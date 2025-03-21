#include "..\Public\AssetsToolActions.h"

#include "AssetSelection.h"
#include "AssetsToolCheckActions.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "FileHelpers.h"
#include "IDesktopPlatform.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AudiokineticTools/Private/Factories/AkAssetTypeActions.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Interfaces/IPluginManager.h"
#include "Settings/ContentBrowserSettings.h"
#include "SlateWidget/SAttachPathsDialog.h"
#include "SlateWidget/SScreeningAssetsDialog.h"
#include "Widgets/Notifications/SNotificationList.h"
#define LOCTEXT_NAMESPACE "FExtendEditorCommands"

UAssetsToolActions::UAssetsToolActions()
{
	DefaultTypePaths.Add(UStaticMesh::StaticClass(), "Mesh");
	DefaultTypePaths.Add(UTexture::StaticClass(), "Texture");
	DefaultTypePaths.Add(USkeletalMesh::StaticClass(), "Mesh");
	DefaultTypePaths.Add(UMaterial::StaticClass(), "Material");
	DefaultTypePaths.Add(USoundWave::StaticClass(), "SoundWave");
	DefaultTypePaths.Add(UObject::StaticClass(), "Default");

	CustomTypePathContainer = NewObject<UTypePathContainer>();
	CustomTypePathContainer->TypePaths.Append(DefaultTypePaths);
}

void UAssetsToolActions::BatchMovement()
{
}

void UAssetsToolActions::ScreeningAssets()
{

#pragma region 用户主动杀死进程结束引用修复的Case
{
	TArray<FString> ReferencesList;
	const FString DefaultListPath = UAssetsToolCheckActions::NativeGetDefaultAssetListPath();

	// 检查是否存在待处理的引用列表
	if (UAssetsToolCheckActions::NativeHaveTheListOfReferencesNeedProcess(ReferencesList, DefaultListPath))
	{
		const FText DialogTitle = LOCTEXT("LegacyReferencesTitle", "发现未处理的引用列表");
		const FText DialogMessage = LOCTEXT("LegacyReferencesMessage", 
			"发现未处理的引用记录（共 {0} 项），是否立即修复？\n\n"
			"[Yes] 启动引用修复流程\n"
			"[No] 将删除本地记录文件（建议通过版本控制回滚资产）\n\n"
			"⚠️ 重要提醒：\n"
			"1. 若之前启动过新进程修复，请确认该进程已完成\n"
			"2. 未完成的修复操作可能导致数据冲突或损坏\n"
			"3. 继续使用错误引用列表将导致迁移失败或引用链断裂");

		TArray<FFormatArgumentValue> Args;
		Args.Add(FFormatArgumentValue(ReferencesList.Num())); // 正确包装整型

		const EAppReturnType::Type UserChoice = FMessageDialog::Open(
			EAppMsgType::YesNoCancel,
			FText::Format(DialogMessage, Args),
			&DialogTitle
		);

		switch (UserChoice)
		{
		case EAppReturnType::Yes:
			{
				// 调用独立处理函数
				auto [bSuccess, ProcessedCount, bFileDeleted] = UAssetsToolCheckActions::NativeProcessLegacyReferences(ReferencesList, DefaultListPath);

				// 显示通知
				FNotificationInfo ResultInfo(FText::Format(
					LOCTEXT("FixupResult", "处理完成\n资产数量: {0}\n文件清理: {1}"),
					FText::AsNumber(ProcessedCount),
					FText::FromString(bFileDeleted ? TEXT("✓") : TEXT("✗"))
				));

				ResultInfo.Hyperlink = FSimpleDelegate::CreateLambda([DefaultListPath](){
					FPlatformProcess::ExploreFolder(*FPaths::GetPath(DefaultListPath));
				});

				if (!bSuccess)
				{
					ResultInfo.Text = FText::Format(
						LOCTEXT("FixupPartial", "部分失败!\n成功处理 {0}/{1} 个资产"),
						FText::AsNumber(ProcessedCount),
						FText::AsNumber(ReferencesList.Num())
					);
					ResultInfo.ExpireDuration = 10.0f;
				}

				FSlateNotificationManager::Get().AddNotification(ResultInfo);
			}
			break;
    
		case EAppReturnType::No:
			// 用户选择删除本地文件
				if (UAssetsToolCheckActions::NativeDeleteAssetListFile(DefaultListPath))
				{
					// 显示操作指引通知
					FNotificationInfo Info(LOCTEXT("DeleteSuccessMessage", 
						"已删除本地记录文件，请执行以下操作：\n"
						"1. 在版本控制中回滚相关资产\n"
						"2. 重新生成引用列表\n"
						"3. 再次运行本工具"));
					Info.ExpireDuration = 10.0f;
					FSlateNotificationManager::Get().AddNotification(Info);
				}
			break;

		case EAppReturnType::Cancel:
		default:
			// 用户取消操作，直接return，防止继续操作导致更大的错误。
			UE_LOG(LogTemp, Warning, TEXT("用户取消引用处理操作"));
			return;
		}
	}
}
#pragma endregion 
	
	TArray<FAssetData> TempSelectedAssets;
	AssetSelectionUtils::GetSelectedAssets(TempSelectedAssets);

	// 这个一定要清空
	DataToBeProcessed.Reset();

	// 用户可能选择文件夹操作，也可能选择资产操作
	// 或者批量操作，所以这里要两者兼顾，使用完一轮后释放即可
	SelectedAssets.Append(TempSelectedAssets);
	
	TArray<FString> SelectedFolders = FolderPathsSelected;

	TArray<FAssetData> FolderAssets = GetAllAssetsInFolders(SelectedFolders);

	// 合并结果
	TSet<FAssetData> AllAssetsSet(SelectedAssets);
	AllAssetsSet.Append(FolderAssets);
	TArray<FAssetData> AllAssets = AllAssetsSet.Array();

	TArray<FName> PackageNames;
	PackageNames.Reserve(AllAssets.Num());
	for (int32 AssetIdx = 0; AssetIdx < AllAssets.Num(); ++AssetIdx)
	{
		PackageNames.Add(AllAssets[AssetIdx].PackageName);
	}

	FolderPathsSelected.Reset();
	SelectedAssets.Reset();

	PerformMigratePackages(PackageNames, FString());
}

void UAssetsToolActions::CopyStructure()
{
    // 确保选择了有效目录
	if (FolderPathsSelected.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("未选择任何目录，操作取消"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("请至少选择一个目录"));
		return;
	}

	FString SelectedPath = FolderPathsSelected[0];
	if (FolderPathsSelected.Num() > 1)
	{
		// 构建确认对话框
		FText Title = LOCTEXT("Tiptop", "多选提示");
		FString Message = FString::Printf(TEXT("当前选择了 %d 个目录，默认使用第一个：\n%s\n是否继续操作？"), 
			FolderPathsSelected.Num(), *SelectedPath);
        
		// 显示模态对话框
		EAppReturnType::Type Result = FMessageDialog::Open(
			EAppMsgType::OkCancel,
			FText::FromString(Message),
			&Title
		);

		// 处理用户选择
		if (Result != EAppReturnType::Ok)
		{
			UE_LOG(LogTemp, Display, TEXT("用户取消多选操作"));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("多选确认通过，使用第一个目录: %s"), *SelectedPath);
	}

    // 获取源目录绝对路径
    FString SourcePath = FPaths::ConvertRelativePathToFull(FolderPathsSelected[0]);
	SourcePath = UAssetsToolCheckActions::ConvertGamePathToPhysical(SourcePath);
    UE_LOG(LogTemp, Log, TEXT("源目录路径: %s"), *SourcePath);

	// 获取目标路径
    FString TargetPath = GetDestinationFolder();

    TArray<FString> RelativePaths;
    CollectSubdirectoriesRecursively(SourcePath, SourcePath, RelativePaths);

    if (TargetPath.IsEmpty())
    {
        return;
    }

    TargetPath = FPaths::ConvertRelativePathToFull(TargetPath);
    UE_LOG(LogTemp, Display, TEXT("目标路径: %s"), *TargetPath);

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    int32 SuccessCount = 0;

    for (const FString& RelPath : RelativePaths)
    {
        const FString FullTargetPath = FPaths::Combine(TargetPath, RelPath);

        if (PlatformFile.CreateDirectoryTree(*FullTargetPath))
        {
            ++SuccessCount;
            UE_LOG(LogTemp, Log, TEXT("成功创建: %s"), *FullTargetPath);
        	
        	// 创建.gitkeep文件  不然空文件无法提交
        	const FString KeepFilePath = FPaths::Combine(FullTargetPath, TEXT(".gitkeep"));
        
        	if (!FPaths::FileExists(KeepFilePath))
        	{
        		if (FFileHelper::SaveStringToFile(TEXT(""), *KeepFilePath))
        		{
        			UE_LOG(LogTemp, Log, TEXT("已创建.gitkeep文件: %s"), *KeepFilePath);
        		}
        		else
        		{
        			UE_LOG(LogTemp, Log, TEXT("创建.gitkeep失败: %s"), *KeepFilePath);
        		}
        	}
        	else
        	{
        		UE_LOG(LogTemp, Verbose, TEXT(".gitkeep文件已存在: %s"), *KeepFilePath);
        	}
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("创建失败: %s"), 
                *FullTargetPath);
        }
    }

    FString Result = FString::Printf(TEXT("成功创建 %d/%d 个目录"), SuccessCount, RelativePaths.Num());
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Result));
}

void UAssetsToolActions::CollectSubdirectoriesRecursively(
    const FString& RootPath,
    const FString& CurrentPath,
    TArray<FString>& OutRelativePaths)
{
    TArray<FString> SubDirs;
    IFileManager::Get().FindFiles(SubDirs, *(CurrentPath / "*"), false, true);

    for (const FString& DirName : SubDirs)
    {
        if (DirName.Equals("Core", ESearchCase::IgnoreCase)) continue;

        const FString FullPath = FPaths::Combine(CurrentPath, DirName);

        FString AdjustedPath = FullPath;
        // 移除掉RootPath
        AdjustedPath.RemoveFromStart(RootPath);
        FPaths::CollapseRelativeDirectories(AdjustedPath);

        if (!AdjustedPath.IsEmpty())
        {
            OutRelativePaths.Add(AdjustedPath);
            UE_LOG(LogTemp, Log, TEXT("收集到相对路径: %s"), *AdjustedPath);
        }

        // 递归处理子目录
        CollectSubdirectoriesRecursively(RootPath, FullPath, OutRelativePaths);
    }
}

FString UAssetsToolActions::GetDestinationFolder() const
{
	FString DestinationFolder;
	// Choose a destination folder
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (ensure(DesktopPlatform))
	{
		const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

		const FString Title = LOCTEXT("MigrateToFolderTitle", "选择目标文件夹").ToString();
		bool bFolderAccepted = false;
		while (!bFolderAccepted)
		{
			const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
				ParentWindowWindowHandle,
				Title,
				FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
				DestinationFolder
			);

			if (!bFolderSelected)
			{
				// User canceled, return
				return "";
			}

			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, DestinationFolder);
			FPaths::NormalizeFilename(DestinationFolder);
			if (!DestinationFolder.EndsWith(TEXT("/")))
			{
				DestinationFolder += TEXT("/");
			}
			
			const FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
			FString FullDestinationPath = FPaths::ConvertRelativePathToFull(DestinationFolder);

			// 检查是否在项目 Content 目录下
			if (!FPaths::IsUnderDirectory(FullDestinationPath, ProjectContentDir))
			{
				const FText Message = FText::Format(
					LOCTEXT("MigratePackages_InvalidContentPath", "路径必须位于项目 Content 目录下！\n当前路径：{0}\n有效路径示例：{1}"),
					FText::FromString(FullDestinationPath),
					FText::FromString(ProjectContentDir + "SubFolder/")
				);

				EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::OkCancel, Message);
				if (Response == EAppReturnType::Cancel)
				{
					return "";
				}
				bFolderAccepted = false;
				continue;
			}

			// Verify that it is a content folder
			if (!DestinationFolder.Compare(TEXT("/Content/")))
			{
				// The user chose a non-content folder. Let them know they cannot do that.
				const FText Message = FText::Format(LOCTEXT("MigratePackages_NonContentFolder", "{0} 必须选择Content下的目录，其他目录无效"), FText::FromString(DestinationFolder));
				EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::OkCancel, Message);
				if (Response == EAppReturnType::Cancel)
				{
					return "";
				}

				continue;
			}
			bFolderAccepted = true;
		}
	}
	else
	{
		// Not on a platform that supports desktop functionality
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoDesktopPlatform", "Error: This platform does not support a file dialog."));
		return "";
	}

	return DestinationFolder;
}

void UAssetsToolActions::InitContentBrowserExtend()
{
	FContentBrowserModule& ContentBrowserModule = 
	FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// 获取所有可拓展上下文
	TArray<FContentBrowserMenuExtender_SelectedPaths>& ContentBrowserModuleMenuExtenders = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
	FContentBrowserMenuExtender_SelectedPaths CustomCBMenuDelegate_Paths;

	TWeakPtr<UAssetsToolActions*> WeakPtrThis = MakeShared<UAssetsToolActions*>(this);

	CustomCBMenuDelegate_Paths.BindLambda(
		[WeakThis = TWeakObjectPtr<UAssetsToolActions>(this)](const TArray<FString>& InFolderPaths) -> TSharedRef<FExtender>
	{
		// 创建菜单扩展器（TSharedRef 无需判空）
		TSharedRef<FExtender> MenuExtender = MakeShared<FExtender>();

		// 安全访问对象数据
		if (UAssetsToolActions* ValidThis = WeakThis.Get())
		{
			ValidThis->FolderPathsSelected = InFolderPaths;
	        
			MenuExtender->AddMenuExtension(
				FName("Delete"),
				EExtensionHook::After,
				nullptr, // 无需命令列表时直接传空指针
				FMenuExtensionDelegate::CreateLambda(
					[WeakThis](FMenuBuilder& MenuBuilder)
					{
						if (UAssetsToolActions* ValidInnerThis = WeakThis.Get())
						{
							MenuBuilder.AddMenuEntry(
								LOCTEXT("Screening assets", "资产筛选"),
								LOCTEXT("Screening assets", "筛选当前选中的资产"),
								FSlateIcon(),
								FUIAction(
									FExecuteAction::CreateUObject(
										ValidInnerThis, 
										&UAssetsToolActions::ScreeningAssets
									),
									// 安全绑定可执行性检查
									FCanExecuteAction::CreateLambda([WeakThis]() -> bool {
										return WeakThis.IsValid();
									})
								)
							);
							
							MenuBuilder.AddMenuEntry(
								LOCTEXT("Copy Structure", "拷贝结构"),
								LOCTEXT("Copy Structure", "拷贝当前选中目录下的文件结构到新目录（不包含顶级目录以及Core：例如JY，JY/Core）"),
								FSlateIcon(),
								FUIAction(
									FExecuteAction::CreateUObject(
										ValidInnerThis, 
										&UAssetsToolActions::CopyStructure
									),
									// 检查是否可执行Lambda
									FCanExecuteAction::CreateLambda([WeakThis]() -> bool {
										return WeakThis.IsValid();
									})
								)
							);
						}
					}
				)
			);
		}

		return MenuExtender;
	});
	ContentBrowserModuleMenuExtenders.Add(CustomCBMenuDelegate_Paths);

	// 资产拓展
	TArray<FContentBrowserMenuExtender_SelectedAssets>& ContentBrowserModuleSelectedAssets = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	FContentBrowserMenuExtender_SelectedAssets CustomCBMenuDelegate_Assets;
	CustomCBMenuDelegate_Assets.BindLambda(
	[WeakThis = TWeakObjectPtr<UAssetsToolActions>(this)](const TArray<FAssetData>& Assets) -> TSharedRef<FExtender>
	{
		TSharedRef<FExtender> MenuExtender = MakeShared<FExtender>();

		if (UAssetsToolActions* ValidThis = WeakThis.Get())
		{
			if (!Assets.IsEmpty())
			{
				ValidThis->SelectedAssets = Assets;

				MenuExtender->AddMenuExtension(
					FName("Delete"),
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateLambda(
						[WeakThis](FMenuBuilder& MenuBuilder)
						{
							if (UAssetsToolActions* ValidInnerThis = WeakThis.Get())
							{
								MenuBuilder.AddMenuEntry(
									LOCTEXT("Screening assets", "资产筛选"),
									LOCTEXT("Screening assets", "筛选当前选中的资产"),
									FSlateIcon(),
									FUIAction(
										FExecuteAction::CreateUObject(
											ValidInnerThis, 
											&UAssetsToolActions::ScreeningAssets
										),
										FCanExecuteAction::CreateLambda([WeakThis] {
											return WeakThis.IsValid();
										})
									)
								);
							}
						}
					)
				);
			}
		}
	    
		return MenuExtender;
	});
	
	ContentBrowserModuleSelectedAssets.Add(CustomCBMenuDelegate_Assets);
}

void UAssetsToolActions::PerformMigratePackages(TArray<FName> PackageNamesToMigrate, const FString DestinationPath) const
{
	// Form a full list of packages to move by including the dependencies of the supplied packages
	TSet<FName> AllPackageNamesToMove;
	TSet<FString> ExternalObjectsPaths;
	TSet<FName> ExcludedDependencies;
	{
		FScopedSlowTask SlowTask(static_cast<float>(PackageNamesToMigrate.Num()), LOCTEXT( "MigratePackages_GatheringDependencies", "Gathering Dependencies..." ) );
		SlowTask.MakeDialog();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TFunction<bool (FName)> ShouldExcludePackage = [this](FName PackageName) { return false; };

		for ( auto PackageIt = PackageNamesToMigrate.CreateConstIterator(); PackageIt; ++PackageIt )
		{
			const FName PackageName = *PackageIt;

			// 获取资产数据
			TArray<FAssetData> AssetDatas;
			if (AssetRegistry.GetAssetsByPackageName(PackageName, AssetDatas))
			{
				if (AssetDatas.IsEmpty()) continue;

				for (auto AssetData : AssetDatas)
				{
					// 检查是否为 UWorld 类型
					if (AssetData.AssetClassPath == UWorld::StaticClass()->GetClassPathName())
					{
						SlowTask.EnterProgressFrame();
						if ( !AllPackageNamesToMove.Contains(*PackageIt) )
						{
							// 如果是关卡的话需要去掉自身，关卡不参与Copy
							// AllPackageNamesToMove.Add(*PackageIt);
							// 关卡虽然不参与移动，但是需要后处理引用列表
							DataToBeProcessed.Add(AssetData);
							RecursiveGetDependencies(*PackageIt, AllPackageNamesToMove, ExternalObjectsPaths, ExcludedDependencies, ShouldExcludePackage);
						}
					}
					else
					{
						SlowTask.EnterProgressFrame();
						if ( !AllPackageNamesToMove.Contains(*PackageIt) )
						{
							AllPackageNamesToMove.Add(*PackageIt);
						}
					}
				}
			};
		}
	}

	// Fetch the enabled plugins and their mount points
	TMap<FName, EPluginLoadedFrom> EnabledPluginToLoadedFrom;
	TArray<TSharedRef<IPlugin>> EnabledPlugins = IPluginManager::Get().GetEnabledPluginsWithContent();
	for (const TSharedRef<IPlugin>& EnabledPlugin : EnabledPlugins)
	{
		EnabledPluginToLoadedFrom.Add(FName(EnabledPlugin->GetMountedAssetPath()), EnabledPlugin->GetLoadedFrom());
	}

	// Find assets in non-Project Plugins
	TSet<FName> ShouldMigratePackage;
	const UContentBrowserSettings* Settings = GetDefault<UContentBrowserSettings>();
	if (Settings != nullptr)
	{
		bool bShouldShowEngineContent = Settings->GetDisplayEngineFolder();
		{
			// This is the new list to prompt for migration
			TSet<FName> FilteredPackageNamesToMove;

			for (const FName& PackageName : AllPackageNamesToMove)
			{
				FName PackageMountPoint = FPackageName::GetPackageMountPoint(PackageName.ToString(), false);
				EPluginLoadedFrom* Found = EnabledPluginToLoadedFrom.Find(PackageMountPoint);

				bool bShouldMigratePackage = true;
				if (Found)
				{
					// plugin content, decide if it's appropriate to migrate
					switch (*Found)
					{
					case EPluginLoadedFrom::Engine:
						if (!bShouldShowEngineContent)
						{
							continue;
						}
						bShouldMigratePackage = false;
						break;

					case EPluginLoadedFrom::Project:
						bShouldMigratePackage = true;
						break;
				 
					default:
						bShouldMigratePackage = false;
						break;
					}
				}
				else
				{
					// this is not plugin content
					if (PackageName.ToString().StartsWith(TEXT("/Engine")))
					{
						// Engine content
						if (!bShouldShowEngineContent)
						{
							continue;
						}
						bShouldMigratePackage = false;
					}
					else
					{
						// Game content
						bShouldMigratePackage = true;
					}
				}

				FilteredPackageNamesToMove.Add(PackageName);

				if (bShouldMigratePackage)
				{
					ShouldMigratePackage.Add(PackageName);
				}
			}

			AllPackageNamesToMove = FilteredPackageNamesToMove;
		}
	}

	// Confirm that there is at least one package to move 
	if (AllPackageNamesToMove.Num() == 0)
	{
		if (!FApp::IsUnattended())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MigratePackages_NoFilesToMove", "No files were found to move"));
		}
		return;
	}

	TSharedPtr<TArray<ReportPackageData>> ReportPackages = MakeShareable(new TArray<ReportPackageData>);
	for (auto PackageIt = AllPackageNamesToMove.CreateConstIterator(); PackageIt; ++PackageIt)
	{
		// 检查此目录是否为临时资产
		const FString PackagePath = *PackageIt->ToString();
		const FString PackageDir = FPaths::GetPath(PackagePath);

		// 检查是否在 TempRes 目录中   这段代码不用试试了
		// TArray<FString> PathSegments;
		// PackageDir.ParseIntoArray(PathSegments, TEXT("/"));
		// bool bIsInTempRes = PathSegments.Contains(TEXT("TempRes"));
		//
		// if (!bIsInTempRes)
		// {
		// 	continue;
		// }
		
		bool bShouldMigratePackage = ShouldMigratePackage.Find(*PackageIt) != nullptr;
		ReportPackages.Get()->Add({ (*PackageIt).ToString(), bShouldMigratePackage });
	}

	if (ReportPackages->IsEmpty())
	{
		if (!FApp::IsUnattended())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MigratePackages_NoFilesToMove", "没有找到临时文件。"));
		}
		return;
	}

	const FText ReportMessage = LOCTEXT("MigratePackagesReportTitle", "以下为资产列表，点击确认后可选择迁移至目标目录。");
	SScreeningAssetsDialog::FOnReportConfirmed OnReportConfirmed = SScreeningAssetsDialog::FOnReportConfirmed::CreateUObject(this, &ThisClass::MigratePackages_ReportConfirmed, ReportPackages, DestinationPath);
	SScreeningAssetsDialog::FOnReportConfirmed OnAttachConfirmed = SScreeningAssetsDialog::FOnReportConfirmed::CreateUObject(this, &ThisClass::ResetPaths);
	SScreeningAssetsDialog::OpenPackageReportDialog(ReportMessage, *ReportPackages.Get(), OnReportConfirmed, OnAttachConfirmed, CustomTypePathContainer);
}

void UAssetsToolActions::RecursiveGetDependencies(const FName& PackageName, TSet<FName>& AllDependencies,
                                                             TSet<FString>& OutExternalObjectsPaths, TSet<FName>& ExcludedDependencies, const TFunction<bool(FName)>& ShouldExcludeFromDependenciesSearch) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FName> Dependencies;
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.GetDependencies(PackageName, Dependencies);
	
	for (TArray<FName>::TConstIterator DependsIt = Dependencies.CreateConstIterator(); DependsIt; ++DependsIt)
	{
		FString DependencyName = (*DependsIt).ToString();

		const bool bIsScriptPackage = DependencyName.StartsWith(TEXT("/Script"));

		// The asset registry can give some reference to some deleted assets. We don't want to migrate these.
		const bool bAssetExist = AssetRegistry.GetAssetPackageDataCopy(*DependsIt).IsSet();

		if (!bIsScriptPackage && bAssetExist)
		{
			uint32 DependsHash = GetTypeHash(*DependsIt);
			if (!AllDependencies.ContainsByHash(DependsHash, *DependsIt) && !ExcludedDependencies.ContainsByHash(DependsHash, *DependsIt))
			{
				// Early stop the dependency search
				if (ShouldExcludeFromDependenciesSearch(*DependsIt))
				{
					ExcludedDependencies.AddByHash(DependsHash, *DependsIt);
					continue;
				}

				AllDependencies.AddByHash(DependsHash, *DependsIt);

				RecursiveGetDependencies(*DependsIt, AllDependencies, OutExternalObjectsPaths, ExcludedDependencies, ShouldExcludeFromDependenciesSearch);
			}
		}
	}

	// Handle Specific External Objects use case (only used for the Migrate path for now)
	// todo: revisit how to handle those in a more generic way. Should the FExternalActorAssetDependencyGatherer handle the external objects reference also?
	TArray<FAssetData> Assets;

	// The migration only work on the saved version of the assets so no need to scan the for the in memory only assets. This also greatly improve the performance of the migration when a lot assets are loaded in the editor.
	const bool bOnlyIncludeOnDiskAssets = true;
	if (AssetRegistryModule.Get().GetAssetsByPackageName(PackageName, Assets, bOnlyIncludeOnDiskAssets))
	{
		for (const FAssetData& AssetData : Assets)
		{
			if (AssetData.GetClass() && AssetData.GetClass()->IsChildOf<UWorld>())
			{
				TArray<FString> ExternalObjectsPaths = ULevel::GetExternalObjectsPaths(PackageName.ToString());
				for (const FString& ExternalObjectsPath : ExternalObjectsPaths)
				{
					if (!ExternalObjectsPath.IsEmpty() && !OutExternalObjectsPaths.Contains(ExternalObjectsPath))
					{
						OutExternalObjectsPaths.Add(ExternalObjectsPath);
						AssetRegistryModule.Get().ScanPathsSynchronous({ ExternalObjectsPath }, /*bForceRescan*/true, /*bIgnoreDenyListScanFilters*/true);

						TArray<FAssetData> ExternalObjectAssets;
						AssetRegistryModule.Get().GetAssetsByPath(FName(*ExternalObjectsPath), ExternalObjectAssets, /*bRecursive*/true, bOnlyIncludeOnDiskAssets);

						for (const FAssetData& ExternalObjectAsset : ExternalObjectAssets)
						{
							// We don't expose the early dependency search exit to the external objects/actors since to the users their are same the outer package that own these objects
							AllDependencies.Add(ExternalObjectAsset.PackageName);
							RecursiveGetDependencies(ExternalObjectAsset.PackageName, AllDependencies, OutExternalObjectsPaths, ExcludedDependencies, ShouldExcludeFromDependenciesSearch);
						}
					}
				}
			}
		}
	}
}

void UAssetsToolActions::ResetPaths() const
{
	CustomTypePathContainer->TypePaths.Empty();
	CustomTypePathContainer->TypePaths.Append(DefaultTypePaths);
}

void UAssetsToolActions::MigratePackages_ReportConfirmed(TSharedPtr<TArray<ReportPackageData>> PackageDataToMigrate,
	const FString DestinationPath) const
{
	FString DestinationFolder = GetDestinationFolder();

	EAppReturnType::Type UserChoice = FMessageDialog::Open(
		EAppMsgType::YesNo, 
		LOCTEXT("MigratePackages_NoFilesToMove", "是否丢弃各类型目录设置？\n（Yes：放弃指定目录，使用默认目录）")
	);

	if (UserChoice == EAppReturnType::Yes)
	{
		UE_LOG(LogTemp, Warning, TEXT("用户选择放弃指定目录，使用默认目录"));
		// 重置路径
		ResetPaths();
	}
	
	UE_LOG(LogTemp, Warning, TEXT("目标目录为： %s"), *DestinationFolder);

	TArray<FAssetRenameData> FAssetRenames;

	// 按照类型分类
	TArray<ReportPackageData> AllReportPackages = *PackageDataToMigrate;
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// 记录错误信息
	TArray<FString> ConversionErrors;
	
	// 分类
	for (const ReportPackageData& Package : AllReportPackages)
	{
		const FName PackageName = FName(Package.Name);

		if (!Package.bShouldMigratePackage)
		{
			// 没有选中，直接放弃
			continue;
		}

		TArray<FAssetData> Assets;
		if (AssetRegistry.GetAssetsByPackageName(PackageName, Assets))
		{
			if (Assets.IsEmpty()) continue;

			FAssetRenameData Data;
			
			for (auto Asset : Assets)
			{
				AssemblyRenameData(Data, Asset, DestinationFolder);

				// 转换旧路径和新路径为虚拟路径
                FString NewVirtualPath;
				
                if (FPackageName::TryConvertFilenameToLongPackageName(Data.NewPackagePath, NewVirtualPath))
                {
                	Data.NewPackagePath = NewVirtualPath;
                }
				else
				{
					UE_LOG(LogTemp, Error, TEXT("虚拟路径转化失败：%s"), *Data.NewPackagePath);
					// 记录具体错误信息
					FString ErrorMsg = FString::Printf(TEXT("资产 [%s] 路径转换失败：%s"), 
						*Asset.AssetName.ToString(),
						*Data.NewPackagePath);
					UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
					ConversionErrors.Add(ErrorMsg);
					continue;
				}
				
				{
					// log
					UE_LOG(LogTemp, Log, TEXT("资产：%s,旧路径为：%s,新路径为：%s"), *Asset.AssetName.ToString(), *Asset.GetSoftObjectPath().ToString(), *Data.NewPackagePath);
				}

				FAssetRenames.Add(Data);
			}
		};
	}

	// 有效资产为0
	if (FAssetRenames.Num() <= 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MigratePackages_NoFilesToMove", "操作成功，并未选中有效资产。"));
		return;
	}

	// 弹出汇总错误提示
	if (ConversionErrors.Num() > 0)
	{
		FString FullErrorMsg = FString::Join(ConversionErrors, TEXT("\n"));
		FullErrorMsg.Append("\n 因为引擎资源列表刷新不及时会误报，可先主动确认是否移动成功。");
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FullErrorMsg));
	}

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// 修复前保存当前状态
	const FString TempSavePath = UAssetsToolCheckActions::NativeGetDefaultAssetListPath();

	bool bRenameSuccess = false;
	try 
	{
	    bRenameSuccess = AssetTools.RenameAssets(FAssetRenames);
	    
		for (const FAssetRenameData& RenameData : FAssetRenames) {
			const FString FullObjectPath = FString::Printf(TEXT("%s/%s"), *RenameData.NewPackagePath, *RenameData.NewName);
    
			FAssetData NewAssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(FullObjectPath));
			if (NewAssetData.IsValid()) {
				DataToBeProcessed.Add(NewAssetData);
			} else {
				UE_LOG(LogTemp, Warning, TEXT("无法找到迁移后的资产: %s"), *FullObjectPath);
			}
		}
	}
	catch (const std::exception& e)
	{
	    UE_LOG(LogTemp, Error, TEXT("资产转移异常: %s"), UTF8_TO_TCHAR(e.what()));
	}

	// 转移结果处理
	if (bRenameSuccess)
	{
	    FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MigrateSuccess", "资产转移成功,准备修复引用"));
	}
	else
	{
	    FNotificationInfo Info(FText::Format(
	        LOCTEXT("BackupKept", "可能有其他原因转移失败的资产，若影响到工作请联系程序排查问题，或本地检查资产引用问题。"),
	        FText::FromString(TempSavePath)
	    ));
	    Info.ExpireDuration = 10.0f;
	    FSlateNotificationManager::Get().AddNotification(Info);
	}

	TArray<FString> CurrentAssetList = UAssetsToolCheckActions::NativeConvertAssetDataToStringPaths(DataToBeProcessed); // 获取当前处理的资产路径列表

	if (!UAssetsToolCheckActions::NativeSaveTheAssetListLocally(CurrentAssetList, TempSavePath))
	{
		UE_LOG(LogTemp, Error, TEXT("无法创建修复备份文件: %s"), *TempSavePath);
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BackupFailed", "创建修复备份失败，操作中止,请联系程序排查问题！"));
		return;
	}

	// 弹窗确认是否启动新的进程进行引用修复
	EAppReturnType::Type UserResponse = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("ConfirmNewProcess", "是否启动新进程进行引用修复？\n(推荐使用新进程避免当前编辑器卡顿，修复期间请不要随意操作正在修复的资产，以免发生其他未知问题。)"));
	
	if (UserResponse == EAppReturnType::Yes)
	{
		EBuildConfiguration CurrentBuildConfig = FApp::GetBuildConfiguration();

		FString EditorPath;
		EditorPath = FPlatformProcess::GenerateApplicationPath(
			TEXT("UnrealEditor"), 
			CurrentBuildConfig
		);

		if (CurrentBuildConfig == EBuildConfiguration::DebugGame)
		{
			EditorPath = EditorPath.Replace(TEXT("-Win64-Shipping"), TEXT("-Win64-DebugGame"));
		}

		// 这里不可用无头模式，AsetTools中有大部分都会使用到Open等Slate模块相关的代码，无头模式会导致崩溃
		FString CommandLine = FString::Printf(
			TEXT("\"%s\" %s -run=FixupRedirector -AssetList=\"%s\" -build=%s -stdout -unattended -NoSlateUI -EnableUnrealEdModule"),
			*FPaths::GetProjectFilePath(),
			*FCommandLine::GetSubprocessCommandline(),
			*TempSavePath,
			LexToString(CurrentBuildConfig) // 动态传递当前配置
		);

		UE_LOG(LogTemp, Log, TEXT("准备启动新进程修复引用启动命令为：%s"), *CommandLine);
		
		bool bLaunched = false;
		
#if PLATFORM_WINDOWS
		bLaunched = FPlatformProcess::CreateProc(
			*EditorPath,
			*CommandLine,
			true,
			false,
			false,
			nullptr,
			0,
			nullptr,
			nullptr,
			nullptr
		).IsValid();
#endif

		if (!bLaunched)
		{
			UE_LOG(LogTemp, Error, TEXT("进程启动失败！路径: %s\n命令行: %s"), 
				*EditorPath, *CommandLine);
			return;
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConfirmNewProcess", "正在拉起修复窗口，修复过程中除正在修复的资产外（切勿对正在修复引用的资产做其他操作，可能会导致引用修复失败。），其他可随意操作。"));
		}
	}
	else
	{
		// 无论成功与否都执行引用修复  这个函数内部已有MarkDirty并保存的逻辑，因此不要下一行
		UAssetsToolCheckActions::NativeFixupAssetReferences(DataToBeProcessed, true);
		// 使用Commandlet专用接口找Bug
		//UAssetsToolCheckActions::NativeCommandletFixupAssetReferences(DataToBeProcessed);
		// 强制标记脏数据并保存
		// UAssetsToolCheckActions::NativeActiveTriggerMarkDirty(DataToBeProcessed);
		
		{
			// 必须执行，只要进过此阶段必须执行删除
			const double StartTime = FPlatformTime::Seconds();
    
			// 异步安全删除
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [TempSavePath]()
			{
				if (FPaths::FileExists(TempSavePath))
				{
					IFileManager& FileManager = IFileManager::Get();
            
					// 最多重试3次
					for (int32 i = 0; i < 3; ++i)
					{
						if (FileManager.Delete(*TempSavePath))
						{
							UE_LOG(LogTemp, Log, TEXT("清理备份文件成功: %s"), *TempSavePath);
							return;
						}
						FPlatformProcess::Sleep(0.5f); // 等待500ms重试
					}
            
					UE_LOG(LogTemp, Warning, TEXT("无法删除备份文件: %s"), *TempSavePath);
				}
			});
		}
	}

	{
		// 重置本次数据
		FolderPathsSelected.Reset();
		SelectedAssets.Reset();
		DataToBeProcessed.Reset();
	}
}

TArray<FAssetData> UAssetsToolActions::GetAllAssetsInFolders(const TArray<FString>& FolderPaths)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.bRecursivePaths = true;

	for (const FString& FolderPath : FolderPaths)
	{
		Filter.PackagePaths.Add(FName(*FolderPath));
	}

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);
	return Assets;
}

void UAssetsToolActions::AssemblyRenameData(FAssetRenameData& Data,  const FAssetData Asset, const FString DestinationFolder) const
{
	Data.Asset = Asset.GetAsset();
	Data.OldObjectPath = Asset.GetSoftObjectPath();
	Data.NewName = Asset.AssetName.ToString();
	FString NewPathBase = DestinationFolder;

	FString OutPath;

	UClass* AssetClass = Asset.GetClass();
				
	// 检查是否为 UWorld 类型
	if (AssetClass && AssetClass->IsChildOf(UStaticMesh::StaticClass()))
	{
		Data.NewPackagePath = GetReallyPath(UStaticMesh::StaticClass(), OutPath) ? Data.NewPackagePath.Append(OutPath) :NewPathBase.Append(OutPath);
	}
	else if (AssetClass && AssetClass->IsChildOf(UTexture::StaticClass()))
	{
		Data.NewPackagePath = GetReallyPath(UTexture::StaticClass(), OutPath) ? Data.NewPackagePath.Append(OutPath) :NewPathBase.Append(OutPath);
	}
	else if (AssetClass && AssetClass->IsChildOf(USkeletalMesh::StaticClass()))
	{
		Data.NewPackagePath = GetReallyPath(USkeletalMesh::StaticClass(), OutPath) ? Data.NewPackagePath.Append(OutPath) :NewPathBase.Append(OutPath);
	}
	else if (AssetClass && AssetClass->IsChildOf(UMaterialInterface::StaticClass()))
	{
		Data.NewPackagePath = GetReallyPath(UMaterial::StaticClass(), OutPath) ? Data.NewPackagePath.Append(OutPath) :NewPathBase.Append(OutPath);
	}
	else if (AssetClass && AssetClass->IsChildOf(USoundWave::StaticClass()))
	{
		Data.NewPackagePath = GetReallyPath(USoundWave::StaticClass(), OutPath) ? Data.NewPackagePath.Append(OutPath) :NewPathBase.Append(OutPath);
	}
	else
	{
		Data.NewPackagePath = GetReallyPath(UObject::StaticClass(), OutPath) ? Data.NewPackagePath.Append(OutPath) :NewPathBase.Append(OutPath);
	}
}

bool UAssetsToolActions::GetReallyPath(UClass* Type, FString& OutPath) const
{
	bool IsCustom = false;
	
	FString DefaultTypePath = DefaultTypePaths.FindOrAdd(Type);
	FString CustomTypePath = CustomTypePathContainer->TypePaths.FindOrAdd(Type);

	if (DefaultTypePaths.IsEmpty())
	{
		OutPath = "Default";

		// 美术要求如果不在指定的几种类型里，则使用类型名称作为文件夹
		OutPath = Type->GetDisplayNameText().ToString();
		
		// 默认路径失效，这个Case基本不存在，不过还是提醒下吧
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Default Path invalid", "默认路径无效，因此数据被放在Default目录下,请联系vvshangwang排查问题。"));
		return false;
	}

	if (CustomTypePath.IsEmpty())
	{
		IsCustom =  false;
	}

	if (DefaultTypePath == CustomTypePath)
	{
		// 使用默认
		IsCustom =  false;
	}
	else
	{
		// 使用自定义
		IsCustom =  true;
	}

	if (CustomTypePath ==  "Default")
	{
		// 美术不需要Default，放在默认类目录就好。
		CustomTypePath = Type->GetDisplayNameText().ToString();
	}
	
	OutPath = IsCustom ? CustomTypePath : DefaultTypePath;
	return IsCustom;
}

#undef LOCTEXT_NAMESPACE
