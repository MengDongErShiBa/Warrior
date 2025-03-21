#include "AssetsToolCheckActions.h"

#include "AssetToolsModule.h"
#include "CollectionManagerModule.h"
#include "EngineAnalytics.h"
#include "FileHelpers.h"
#include "ICollectionManager.h"
#include "ISourceControlModule.h"
#include "ObjectTools.h"
#include "SourceControlHelpers.h"
#include "Algo/Unique.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Windows/WindowsPlatformApplicationMisc.h"
#include "Wwise/WwiseFileHandlerBase.h"
#include "SourceControlOperations.h"
#include "Developer/AssetTools/Private/AssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Misc/RedirectCollector.h"
#include "UObject/SavePackage.h"
#define LOCTEXT_NAMESPACE "FExtendEditorCommands"

struct CustomFRedirectorRefs
{
	TStrongObjectPtr<const UObjectRedirector> Redirector;
	FName RedirectorPackageName;
	TArray<FName> ReferencingPackageNames;

	// Referencing packages which could not be updated to remove references to this redirector
	TArray<FName> LockedReferencerPackageNames;
	TArray<FName> FailedReferencerPackageNames;

	// Possible failure reason unrelated to above lists of package names.
	// If this is not empty we may not delete the redirector - we may still be able to remove some references to it.
	TArray<FText> OtherFailures;

	bool bSCCError = false; // Failure to check the redirector itself out of source control etc

	explicit CustomFRedirectorRefs(FName PackageName)
		: Redirector(nullptr)
		, RedirectorPackageName(PackageName)
	{
	}

	explicit CustomFRedirectorRefs(const UObjectRedirector* InRedirector)
		: Redirector(InRedirector)
		, RedirectorPackageName(InRedirector->GetOutermost()->GetFName())
	{
	}
};

namespace CustomAssetRenameManagerImpl
{
	// Same as CheckSubPath.IsEmpty() || SubPath == CheckSubPath || SubPath.StartsWith(CheckSubPath + TEXT("."))
	// but with early outs and without having to concatenate a string for comparison
	static bool IsSubPath(const FString& SubPath, const FString& CheckSubPath)
	{
		const int32 CheckSubPathLen = CheckSubPath.Len();
		if (CheckSubPathLen == 0)
		{
			return true;
		}

		const int32 SubPathLen = SubPath.Len();
		if (SubPathLen == CheckSubPathLen)
		{
			if (SubPathLen)
			{
				// Checking the last character first should skip most string compare since lots of paths might have the same beginning
				return (*SubPath)[SubPathLen - 1] == (*CheckSubPath)[SubPathLen - 1] && SubPath == CheckSubPath;
			}
			else
			{
				// Both strings are empty
				return true;
			}
		}
		else
		{
			//Checking for the . at the exact position first should eliminate most of the StartsWith comparison.
			return SubPathLen > CheckSubPathLen && (*SubPath)[CheckSubPathLen] == TEXT('.') && SubPath.StartsWith(CheckSubPath);
		}
	}
}

struct FCustomSoftObjectPathRenameSerializer : public FArchiveUObject
{
	void StartSerializingObject(UObject* InCurrentObject)
	{
		CurrentObject = InCurrentObject;
		bFoundReference = false;
	}
	bool HasFoundReference() const
	{
		return bFoundReference;
	}

	FCustomSoftObjectPathRenameSerializer(const TMap<FSoftObjectPath, FSoftObjectPath>& InRedirectorMap,
		bool bInCheckOnly,
		TMap<FSoftObjectPath, TSet<FWeakObjectPtr>>* InCachedObjectPaths,
		const FName InPackageName = NAME_None)
		: RedirectorMap(InRedirectorMap)
		, CachedObjectPaths(InCachedObjectPaths)
		, CurrentObject(nullptr)
		, PackageName(InPackageName)
		, bSearchOnly(bInCheckOnly)
		, bFoundReference(false)
	{
		if (InCachedObjectPaths)
		{
			DirtyDelegateHandle = UPackage::PackageMarkedDirtyEvent.AddRaw(this, &FCustomSoftObjectPathRenameSerializer::OnMarkPackageDirty);
		}

		this->ArIsObjectReferenceCollector = true;
		this->ArIsModifyingWeakAndStrongReferences = true;

		// Mark it as saving to correctly process all references
		this->SetIsSaving(true);
	}

	virtual ~FCustomSoftObjectPathRenameSerializer()
	{
		UPackage::PackageMarkedDirtyEvent.Remove(DirtyDelegateHandle);
	}

	virtual bool ShouldSkipProperty(const FProperty* InProperty) const override
	{
		if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_IsPlainOldData))
		{
			return true;
		}

		FFieldClass* PropertyClass = InProperty->GetClass();
		if (PropertyClass->GetCastFlags() & (CASTCLASS_FBoolProperty | CASTCLASS_FNameProperty | CASTCLASS_FStrProperty | CASTCLASS_FTextProperty | CASTCLASS_FMulticastDelegateProperty))
		{
			return true;
		}

		if (PropertyClass->GetCastFlags() & (CASTCLASS_FArrayProperty | CASTCLASS_FMapProperty | CASTCLASS_FSetProperty))
		{
			if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(InProperty))
			{
				return ShouldSkipProperty(ArrayProperty->Inner);
			}
			else if (const FMapProperty* MapProperty = CastField<FMapProperty>(InProperty))
			{
				return ShouldSkipProperty(MapProperty->KeyProp) && ShouldSkipProperty(MapProperty->ValueProp);
			}
			else if (const FSetProperty* SetProperty = CastField<FSetProperty>(InProperty))
			{
				return ShouldSkipProperty(SetProperty->ElementProp);
			}
		}

		return false;
	}

	FArchive& operator<<(FSoftObjectPath& Value)
	{
		using namespace CustomAssetRenameManagerImpl;

		// Ignore untracked references if just doing a search only. We still want to fix them up if they happen to be there
		if (bSearchOnly)
		{
			FSoftObjectPathThreadContext& ThreadContext = FSoftObjectPathThreadContext::Get();
			FName ReferencingPackageName, ReferencingPropertyName;
			ESoftObjectPathCollectType CollectType = ESoftObjectPathCollectType::AlwaysCollect;
			ESoftObjectPathSerializeType SerializeType = ESoftObjectPathSerializeType::AlwaysSerialize;

			ThreadContext.GetSerializationOptions(ReferencingPackageName, ReferencingPropertyName, CollectType, SerializeType, this);

			if (CollectType == ESoftObjectPathCollectType::NeverCollect || CollectType == ESoftObjectPathCollectType::NonPackage)
			{
				return *this;
			}
		}

		if (CachedObjectPaths)
		{
			TSet<FWeakObjectPtr>* ObjectSet = &CachedObjectPaths->FindOrAdd(Value);
			ObjectSet->Add(CurrentObject);
		}

		const FString& SubPath = Value.GetSubPathString();
		for (const TPair<FSoftObjectPath, FSoftObjectPath>& Pair : RedirectorMap)
		{
			if (Pair.Key.GetAssetPath() == Value.GetAssetPath())
			{
				// Same asset, fix sub path. Asset will be fixed by normal serializePath call below
				const FString& CheckSubPath = Pair.Key.GetSubPathString();

				if (IsSubPath(SubPath, CheckSubPath))
				{
					bFoundReference = true;

					if (!bSearchOnly)
					{
						if (CurrentObject)
						{
							check(!CachedObjectPaths); // Modify can invalidate the object paths map, not allowed to be modifying and using the cache at the same time
							CurrentObject->Modify(true);
						}

						FString NewSubPath(SubPath);
						NewSubPath.ReplaceInline(*CheckSubPath, *Pair.Value.GetSubPathString());
						Value = FSoftObjectPath(Pair.Value.GetAssetPath(), NewSubPath);
					}
					break;
				}
			}
		}

		return *this;
	}

	void OnMarkPackageDirty(UPackage* Pkg, bool bWasDirty)
	{
		UPackage::PackageMarkedDirtyEvent.Remove(DirtyDelegateHandle);

		if (CachedObjectPaths && Pkg && Pkg->GetFName() == PackageName)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("Performance: Package unexpectedly modified during serialization by FSoftObjectPathRenameSerializer: %s"), *Pkg->GetFullName());
		}
	}

private:
	const TMap<FSoftObjectPath, FSoftObjectPath>& RedirectorMap;
	TMap<FSoftObjectPath, TSet<FWeakObjectPtr>>* CachedObjectPaths;
	FDelegateHandle DirtyDelegateHandle;
	UObject* CurrentObject;
	FName PackageName;
	bool bSearchOnly;
	bool bFoundReference;

};

bool UAssetsToolCheckActions::NativeSaveTheAssetListLocally(const TArray<FString>& AssetPackageNames, const FString TargetPath)
{
	const FString RootSaveDir = FPaths::ProjectSavedDir();
    
	FString FinalPath = NativeCheckPathValid(TargetPath) ? TargetPath : FPaths::Combine(RootSaveDir, TEXT("AssetsTool"), TEXT("AListOfReferencesToBeProcessed.txt"));;

	// 确保目录存在
	const FString Directory = FPaths::GetPath(FinalPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.CreateDirectoryTree(*Directory))
	{
		UE_LOG(LogTemp, Error, TEXT("无法创建目录: %s"), *Directory);
		return false;
	}

	// 转换数据格式（每个元素换行）
	FString TextContent;
	for (const FString& PackageName : AssetPackageNames)
	{
		TextContent += PackageName + LINE_TERMINATOR;
	}

	// 保存到Save
	if (FFileHelper::SaveStringToFile(TextContent, *FinalPath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogTemp, Log, TEXT("资产列表已保存至: %s"), *FinalPath);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("文件保存失败: %s"), *FinalPath);
		return false;
	}
}

bool UAssetsToolCheckActions::NativeHaveTheListOfReferencesNeedProcess(TArray<FString>& OutAssetPackageNames, const FString TargetPath)
{
	OutAssetPackageNames.Reset();

	const FString FinalPath = [&]() -> FString {
		const FString ValidationRoot = FPaths::ProjectSavedDir();
        
		const FString DefaultPath = NativeGetDefaultAssetListPath();

		return NativeCheckPathValid(TargetPath, ValidationRoot) ? TargetPath : DefaultPath;
	}();

	// 检查文件是否有效
	IPlatformFile& FileSystem = FPlatformFileManager::Get().GetPlatformFile();
	if(!FileSystem.FileExists(*FinalPath))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("资产列表文件不存在: %s"), *FinalPath);
#endif
		return false;
	}

	TArray<FString> RawLines;
	if(!FFileHelper::LoadFileToStringArray(RawLines, *FinalPath))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("文件读取失败: %s"), *FinalPath);
#endif
		return false;
	}

	OutAssetPackageNames.Reserve(RawLines.Num());
	for(FString& Line : RawLines)
	{
		// 去除两端空白字符
		Line.TrimStartAndEndInline();
        
		// 有效性验证
		if(!Line.IsEmpty() && FPackageName::IsValidLongPackageName(Line))
		{
			OutAssetPackageNames.Add(MoveTemp(Line));
		}
	}

	// 检查数据是否有效
	const bool bHasValidData = !OutAssetPackageNames.IsEmpty();
#if !UE_BUILD_SHIPPING
	if(!bHasValidData)
	{
		UE_LOG(LogTemp, Warning, TEXT("文件无有效数据: %s"), *FinalPath);
	}
#endif

	return bHasValidData;
}

bool UAssetsToolCheckActions::NativeDeleteAssetListFile(const FString& TargetPath)
{
    const FString FinalPath = [&]() -> FString {
        const FString ValidationRoot = FPaths::ProjectSavedDir();
        const FString DefaultPath = NativeGetDefaultAssetListPath();
        return NativeCheckPathValid(TargetPath, ValidationRoot) ? TargetPath : DefaultPath;
    }();

    IPlatformFile& FileSystem = FPlatformFileManager::Get().GetPlatformFile();
    
    if(!NativeCheckPathValid(FinalPath))
    {
        UE_LOG(LogTemp, Error, TEXT("非法删除路径: %s"), *FinalPath);
        return false;
    }

    if(!FileSystem.FileExists(*FinalPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("文件不存在: %s"), *FinalPath);
        return true; // 认为删除成功
    }

    // 防止误删非资产列表文件
    const FString ExpectedFileName = TEXT("AListOfReferencesToBeProcessed.txt");
    if(FPaths::GetCleanFilename(FinalPath) != ExpectedFileName)
    {
        UE_LOG(LogTemp, Error, TEXT("禁止删除非资产列表文件: %s"), *FinalPath);
        return false;
    }

    // 执行删除操作
    bool bDeleteResult = false;
    try
    {
        bDeleteResult = FileSystem.DeleteFile(*FinalPath);
    }
    catch(const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("文件删除异常: %s"), UTF8_TO_TCHAR(e.what()));
        return false;
    }

    // 检查结果
    if(bDeleteResult)
    {
        UE_LOG(LogTemp, Log, TEXT("成功删除资产列表文件: %s"), *FinalPath);
        return true;
    }
    else
    {
        const uint32 LastError = FPlatformMisc::GetLastError();
        TCHAR ErrorBuffer[1024];
        FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 1024, LastError);
        
        UE_LOG(LogTemp, Error, TEXT("文件删除失败: %s [错误码:%u 信息:%s]"), 
            *FinalPath, LastError, ErrorBuffer);
        return false;
    }
}

bool UAssetsToolCheckActions::NativeFixupAssetReferences(const TArray<FAssetData>& AssetList, bool bShowProgress)
{
    if (AssetList.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("资产列表为空，无需修复"));
        return false;
    }

#if WITH_EDITOR
    // 初始化进度条
    TSharedPtr<FScopedSlowTask> Progress;
    if (bShowProgress && GIsEditor)
    {
        Progress = MakeShareable(new FScopedSlowTask(AssetList.Num(), LOCTEXT("FixupReferences", "修复资产引用...")));
        Progress->MakeDialog(true);
    }
#endif

    // 获取必要模块
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    TSet<UPackage*> ModifiedPackages;
    bool bAllSuccess = true;

    for (int32 i = 0; i < AssetList.Num(); ++i)
    {
        // 保持响应
        FPlatformApplicationMisc::PumpMessages(true);

        const FAssetData& AssetData = AssetList[i];
        
#if WITH_EDITOR
        // 更新进度
        if (Progress.IsValid())
        {
            Progress->EnterProgressFrame(1, FText::Format(
                LOCTEXT("ProcessingAsset", "正在处理: {0} ({1}/{2})"),
                FText::FromName(AssetData.AssetName),
                FText::AsNumber(i + 1),
                FText::AsNumber(AssetList.Num())
            ));

            if (Progress->ShouldCancel())
            {
                UE_LOG(LogTemp, Warning, TEXT("用户取消修复操作"));
                bAllSuccess = false;
                break;
            }
        }
#endif

        // 加载资产
        UObject* Asset = AssetData.GetAsset();
        if (!Asset)
        {
            UE_LOG(LogTemp, Error, TEXT("无法加载资产: %s"), *AssetData.GetObjectPathString());
            bAllSuccess = false;
            continue;
        }

        // 收集重定向器
        TArray<UObjectRedirector*> Redirectors;
        TArray<FName> Dependencies;
        AssetRegistry.GetDependencies(AssetData.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);

        for (const FName& PackageName : Dependencies)
        {
            TArray<FAssetData> PackageAssets;
            AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets);

            for (const FAssetData& PackageAsset : PackageAssets)
            {
                if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(PackageAsset.GetAsset()))
                {
                    Redirectors.Add(Redirector);
                }
            }
        }

    	// // 检查占用，解除后重新收集重定向器
    	// TArray<UObjectRedirector*> SafeRedirectors;
    	// for (UObjectRedirector* Redirector : Redirectors)
    	// {
    	// 	UPackage* Package = Redirector->GetOutermost();
     //
    	// 	// 检测并解除锁定
    	// 	if (NativeIsFileLocked(FPackageName::LongPackageNameToFilename(Package->GetName())))
    	// 	{
    	// 		NativeForceUnlockPackage(Package);
    	// 		ResetLoaders(Redirector);
	    //
    	// 		UE_LOG(LogTemp, Warning, TEXT("强制解除文件锁定: %s"), *Package->GetName());
    	// 	}
	    //
    	// 	if (Package->IsFullyLoaded())
    	// 	{
    	// 		SafeRedirectors.Add(Redirector);
    	// 	}
    	// 	else
    	// 	{
    	// 		UE_LOG(LogTemp, Error, TEXT("无法处理未完全加载的包: %s"), *Package->GetName());
    	// 	}
    	// }
    	//
    	// CollectGarbage(RF_NoFlags, true);

        if (Redirectors.Num() > 0)
        {
            AssetTools.FixupReferencers(Redirectors, bShowProgress);
			
            // 标记脏数据
            UPackage* Package = Asset->GetOutermost();
            if (Package && !Package->IsDirty())
            {
                Package->MarkPackageDirty();
                ModifiedPackages.Add(Package);
            }
        }
    }

    // 保存
    if (ModifiedPackages.Num() > 0)
    {
        TArray<UPackage*> PackagesToSave = ModifiedPackages.Array();
        const bool bSaveSuccess = FEditorFileUtils::SaveDirtyPackages(
            false,  
            true,  
            true,   
            false,
            false  
        );

#if WITH_EDITOR
        if (GIsEditor)
        {
            FNotificationInfo Info(FText::Format(
                LOCTEXT("SaveResult", "已保存 {0} 个修改的资源包"),
                FText::AsNumber(bSaveSuccess ? PackagesToSave.Num() : 0)
            ));
            Info.ExpireDuration = 5.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
#endif
    }

    return bAllSuccess;
}

bool UAssetsToolCheckActions::NativeCommandletFixupAssetReferences(const TArray<FAssetData>& AssetList)
{
	 if (AssetList.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("资产列表为空，无需修复"));
        return false;
    }

    // 获取必要模块
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	// 强制同步资产扫描
	AssetRegistry.SearchAllAssets(true);

	// 等待资产注册表完成初始化
	 if (AssetRegistry.IsSearchAsync() || AssetRegistry.IsLoadingAssets())
	 {
		 UE_LOG(LogTemp, Error, TEXT("等待收集资产，最大等待时长5分钟"));
		 FEventRef ScanCompleteEvent(EEventMode::AutoReset);
		 AssetRegistry.OnFilesLoaded().AddLambda([&ScanCompleteEvent](){ ScanCompleteEvent->Trigger(); });
		 ScanCompleteEvent->Wait(1);
		 // AssetRegistry.OnFilesLoaded().RemoveAll(this);
	 }
	
	UE_LOG(LogTemp, Error, TEXT("资产收集完毕，准备修复引用列表。"));

    TSet<UPackage*> ModifiedPackages;
    bool bAllSuccess = true;

    for (int32 i = 0; i < AssetList.Num(); ++i)
    {
        // 保持响应
        FPlatformApplicationMisc::PumpMessages(true);

        const FAssetData& AssetData = AssetList[i];
        
        // 加载资产
        UObject* Asset = AssetData.GetAsset();
        if (!Asset)
        {
            UE_LOG(LogTemp, Error, TEXT("无法加载资产: %s"), *AssetData.GetObjectPathString());
            bAllSuccess = false;
            continue;
        }

    	// 收集重定向器
    	UE_LOG(LogTemp, Log, TEXT("[引用修复] 开始收集资产 %s 的重定向器"), *AssetData.GetObjectPathString());

    	TArray<UObjectRedirector*> Redirectors;
    	TArray<FName> Dependencies;
    	int32 TotalRedirectorsFound = 0;

    	// 获取包依赖
    	AssetRegistry.GetDependencies(AssetData.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
    	UE_LOG(LogTemp, Log, TEXT("├─ 资产 %s 共有 %d 个包依赖"), *AssetData.PackageName.ToString(), Dependencies.Num());

    	for (int32 j = 0; j < Dependencies.Num(); ++j)
    	{
    		const FName& PackageName = Dependencies[j];
    		UE_LOG(LogTemp, Log, TEXT("│  ├─ 处理依赖包 [%d/%d] %s"), 
				j+1, Dependencies.Num(), *PackageName.ToString());

    		// 获取包内资产
    		TArray<FAssetData> PackageAssets;
    		AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets);
    		UE_LOG(LogTemp, Log, TEXT("│  │  发现 %d 个资产"), PackageAssets.Num());

    		// 查找重定向器
    		int32 PackageRedirectors = 0;
    		for (const FAssetData& PackageAsset : PackageAssets)
    		{
    			if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(PackageAsset.GetAsset()))
    			{
    				Redirectors.Add(Redirector);
    				PackageRedirectors++;
            
    				UE_LOG(LogTemp, Log, TEXT("│  │  ├─ 发现重定向器: %s -> %s"), 
						*Redirector->GetPathName(),
						*Redirector->DestinationObject->GetPathName());
    			}
    		}

    		if (PackageRedirectors > 0)
    		{
    			UE_LOG(LogTemp, Log, TEXT("│  └─ 包 %s 包含 %d 个重定向器"), 
					*PackageName.ToString(), PackageRedirectors);
    			TotalRedirectorsFound += PackageRedirectors;
    		}
    	}

        if (Redirectors.Num() > 0)
        {
        	UE_LOG(LogTemp, Warning, TEXT("└─ [完成] 共找到 %d 个重定向器"), TotalRedirectorsFound);
			NativeFixupReferencers(Redirectors);
            // 标记脏数据
            UPackage* Package = Asset->GetOutermost();
        	
        	// if (NativeIsFileLocked(Package->GetName()))
        	// {
        	// 	NativeForceUnlockPackage(Package);
        	// 	UE_LOG(LogTemp, Log, TEXT("▷▷▷ 包被占用，主动释放: %s"), *Package->GetName());
        	// }
        	
            UE_LOG(LogTemp, Log, TEXT("资产%s修复完成："), *Package->GetName());
            if (Package)
            {
                Package->MarkPackageDirty();
                ModifiedPackages.Add(Package);
            	UE_LOG(LogTemp, Log, TEXT("标记为脏数据：%s"), *Package->GetName());
            }
        }
    	UE_LOG(LogTemp, Log, TEXT("当前处理进度: %d/%d"), i, AssetList.Num());
    }

    // 保存
    if (ModifiedPackages.Num() > 0)
    {
    	// 保存计数器
		int32 SuccessCount = 0;
		int32 FailCount = 0;

		for(UPackage* Pkg : ModifiedPackages)
		{
		    if(!Pkg)
		    {
		        UE_LOG(LogTemp, Error, TEXT("[保存失败] 空包指针!"));
		        FailCount++;
		        continue;
		    }

		    const FString PackageName = Pkg->GetName();
		    FString FileName;
		    
		    // 包名有效性检查
		    if(PackageName.IsEmpty())
		    {
		        UE_LOG(LogTemp, Error, TEXT("[保存失败] 无效包名! 内存地址: %p"), Pkg);
		        FailCount++;
		        continue;
		    }

		    // 转换为文件名
		    if(!FPackageName::DoesPackageExist(PackageName, &FileName))
		    {
		        UE_LOG(LogTemp, Warning, TEXT("[路径警告] 包 %s 不存在磁盘文件，将保存到新位置: %s"), 
		            *PackageName, *FileName);
		    }

		    // 记录保存开始
		    UE_LOG(LogTemp, Log, TEXT("▷▷▷ 正在保存包: %s"), *PackageName);
		    UE_LOG(LogTemp, Log, TEXT("   文件路径: %s"), *FileName);

		    // 执行保存
		    const bool bSaveResult = SafeSavePackage(Pkg);
		    
		    // 记录结果详情
		    if(bSaveResult)
		    {
		        SuccessCount++;
		        UE_LOG(LogTemp, Log, TEXT("◉◉◉ 保存成功: %s"), *PackageName);
		        UE_LOG(LogTemp, VeryVerbose, TEXT("   文件大小: %lld 字节"), 
		            IFileManager::Get().FileSize(*FileName));
		    }
		    else
		    {
		        FailCount++;
		        UE_LOG(LogTemp, Error, TEXT("[保存失败] 包: %s"), *PackageName);
		        
		        // 检查文件系统状态
		        if(!IFileManager::Get().FileExists(*FileName))
		        {
		            UE_LOG(LogTemp, Warning, TEXT("   目标文件未创建: %s"), *FileName);
		        }
		    }
		}
    	
    	// 最终统计
    	UE_LOG(LogTemp, Display, TEXT("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
    	UE_LOG(LogTemp, Display, TEXT("保存操作完成"));
    	UE_LOG(LogTemp, Display, TEXT("成功数量: %d"), SuccessCount);
    	UE_LOG(LogTemp, Display, TEXT("失败数量: %d"), FailCount);
    	UE_LOG(LogTemp, Display, TEXT("━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
    }

    return bAllSuccess;
}

int32 UAssetsToolCheckActions::NativeConvertPackageNamesToAssetData(const TArray<FString>& PackageNames, TArray<FAssetData>& OutAssetDatas)
{
	OutAssetDatas.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// 预过滤有效包名
	TArray<FName> ValidatedPackageNames;
	for (const FString& PackageName : PackageNames)
	{
		// 标准化包名格式
		FString NormalizedPackageName = PackageName;
		if (FPackageName::IsValidLongPackageName(NormalizedPackageName))
		{
			ValidatedPackageNames.Add(FName(*FPackageName::ObjectPathToPackageName(NormalizedPackageName)));
		}
	}

	TArray<FAssetData> AllAssetData;
	for (const FName& PackageName : ValidatedPackageNames)
	{
		TArray<FAssetData> PackageAssets;
		// 对每个包名单独查询
		AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets, true);
		AllAssetData.Append(PackageAssets);
	}

	// 过滤有效主资产（排除生成类等）
	TMap<FName, FAssetData> PrimaryAssetMap;
	for (const FAssetData& Asset : AllAssetData)
	{
		// 每个包只保留主资产（非生成类）
		if (!Asset.IsRedirector() && !Asset.AssetClassPath.IsNull())
		{
			PrimaryAssetMap.Add(Asset.PackageName, Asset);
		}
	}

	// 按输入顺序生成输出列表
	int32 ValidCount = 0;
	for (const FName& PackageName : ValidatedPackageNames)
	{
		if (const FAssetData* FoundData = PrimaryAssetMap.Find(PackageName))
		{
			OutAssetDatas.Add(*FoundData);
			ValidCount++;
		}
	}

#if !UE_BUILD_SHIPPING
	if (ValidCount < PackageNames.Num())
	{
		const int32 MissingCount = PackageNames.Num() - ValidCount;
		UE_LOG(LogTemp, Warning, TEXT("未能找到 %d 个资产的元数据"), MissingCount);
	}
#endif

	return ValidCount;
}

TTuple<bool, int32, bool> UAssetsToolCheckActions::NativeProcessLegacyReferences(const TArray<FString>& ReferencesList, const FString& ListPath)
{
	// 转换路径到AssetData
	TArray<FAssetData> AssetDatas;
	const int32 ValidCount = NativeConvertPackageNamesToAssetData(ReferencesList, AssetDatas);

	// 有效性验证
	if (ValidCount == 0)
	{
		return MakeTuple(false, 0, false);
	}

	// 执行引用修复
	const bool bFixSuccess = NativeFixupAssetReferences(AssetDatas, true);

	// 删除本地文件 无论修复是否成功
	const bool bDeleteSuccess = NativeDeleteAssetListFile(ListPath);

	return MakeTuple(bFixSuccess, ValidCount, bDeleteSuccess);
}

TArray<FString> UAssetsToolCheckActions::NativeConvertAssetDataToStringPaths(const TArray<FAssetData>& DataToBeProcessed)
{
	TArray<FString> ResultPaths;
	ResultPaths.Reserve(DataToBeProcessed.Num());

	for (const FAssetData& AssetData : DataToBeProcessed)
	{
		// 验证有效性
		if (AssetData.IsValid() && !AssetData.IsRedirector())
		{
			const FString PackagePath = AssetData.PackageName.ToString();
            
			if (FPackageName::IsValidLongPackageName(PackagePath))
			{
				ResultPaths.Add(PackagePath);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("非法路径被过滤: %s"), *PackagePath);
			}
		}
	}

	// 去重
	ResultPaths.Sort();
	ResultPaths.SetNum(Algo::Unique(ResultPaths));

	return ResultPaths;
}

void UAssetsToolCheckActions::NativeActiveTriggerMarkDirty(const TArray<FAssetData>& DataToBeProcessed)
{
	if (!DataToBeProcessed.IsEmpty())
	{
		TArray<UPackage*> PackagesToSave;

		for (const FAssetData& AssetData : DataToBeProcessed)
		{
			// 获取资产对象（不强制加载）
			UObject* Asset = AssetData.GetAsset();
			if (!Asset)
			{
				UE_LOG(LogTemp, Warning, TEXT("无法加载资产: %s"), *AssetData.GetSoftObjectPath().ToString());
				continue;
			}

			// 获取所属的 Package
			UPackage* Package = Asset->GetOutermost();
			
			if (!Package)
			{
				UE_LOG(LogTemp, Error, TEXT("无效的 Package: %s"), *Asset->GetName());
				continue;
			}

			// 标记 Package 为脏（仅当未标记时）
			if (!Package->IsDirty())
			{
				Package->MarkPackageDirty();
				UE_LOG(LogTemp, Log, TEXT("标记为脏数据: %s"), *Package->GetName());
				PackagesToSave.AddUnique(Package);
			}
		}
		
		// 批量保存脏 Package
		if (PackagesToSave.Num() > 0)
		{
			FEditorFileUtils::SaveDirtyPackages(
				false,
				true,   
				true,
				true, 
				false, 
				false
			);

			UE_LOG(LogTemp, Display, TEXT("成功保存 %d 个资产"), PackagesToSave.Num());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("无需要保存的脏数据"));
		}
	}
}

void UAssetsToolCheckActions::NativeFixupReferencers(const TArray<UObjectRedirector*>& Objects)
{
	// Transform array into TWeakObjectPtr array
	TArray<TWeakObjectPtr<UObjectRedirector>> ObjectWeakPtrs;
	for (auto Object : Objects)
	{
		ObjectWeakPtrs.Add(Object);
	}

	if (ObjectWeakPtrs.Num() > 0)
	{
		ExecuteFixUp(ObjectWeakPtrs);
	}
}

void UAssetsToolCheckActions::ExecuteFixUp(TArray<TWeakObjectPtr<UObjectRedirector>> Objects)
{

	TArray<CustomFRedirectorRefs> RedirectorRefsList;
	for (TWeakObjectPtr<UObjectRedirector> Object : Objects)
	{
		if (UObjectRedirector* ObjectRedirector = Object.Get())
		{
			RedirectorRefsList.Emplace(ObjectRedirector);
		}
	}

	if (RedirectorRefsList.Num() == 0)
	{
		return;
	}
	
	// Check if we can delete redirectors - if so we need to perform source control operations on them
	bool bMayDeleteRedirectors = true;

	// Gather all referencing packages for all redirectors that are being fixed.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	bool bAnyRefs = false;
	for (CustomFRedirectorRefs& RedirectorRefs : RedirectorRefsList)
	{
		AssetRegistryModule.Get().GetReferencers(RedirectorRefs.RedirectorPackageName, RedirectorRefs.ReferencingPackageNames);
		bAnyRefs = bAnyRefs || RedirectorRefs.ReferencingPackageNames.Num() != 0;
	}
	
	if (!bAnyRefs && !bMayDeleteRedirectors)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoPackagesToSave", "No referencing assets found to be resaved."));
		return;
	}
	
	// Build back-reference lookup for later
	TMultiMap<FName, CustomFRedirectorRefs*> ReferencingAssetToRedirector;
	for (CustomFRedirectorRefs& RedirectorRefs : RedirectorRefsList)
	{
		for (FName PackageName : RedirectorRefs.ReferencingPackageNames)
		{
			ReferencingAssetToRedirector.Add(PackageName, &RedirectorRefs);	
		}
	}

	// Update Package Status for all selected redirectors if SCC is enabled and we may delete
	if (bMayDeleteRedirectors && ISourceControlModule::Get().IsEnabled())
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		// Update the source control server availability to make sure we can do the rename operation
		SourceControlProvider.Login();
		if (!SourceControlProvider.IsAvailable())
		{
			// We have failed to update source control even though it is enabled. This is critical and we can not continue
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "SourceControl_ServerUnresponsive", "Revision Control is unresponsive. Please check your connection and try again.") );
			return;
		}

		TArray<UPackage*> PackagesToAddToSCCUpdate;
		for (const CustomFRedirectorRefs& RedirectorRefs : RedirectorRefsList)
		{
			PackagesToAddToSCCUpdate.Add(RedirectorRefs.Redirector->GetOutermost());
		}
		SourceControlProvider.Execute(ISourceControlOperation::Create<FUpdateStatus>(), PackagesToAddToSCCUpdate);
	}

	// Load all referencing packages.
	TSet<UPackage*> ReferencingPackagesToSave;
	TSet<UPackage*> LoadedPackages;
	bool bCancel = false;
	{
		FScopedSlowTask SlowTask(static_cast<float>(RedirectorRefsList.Num()), LOCTEXT( "LoadingReferencingPackages", "Loading Referencing Packages..." ) );
		SlowTask.MakeDialog(true);
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

		// Load all packages that reference each redirector, if possible
		for (CustomFRedirectorRefs& RedirectorRefs : RedirectorRefsList)
		{
			SlowTask.EnterProgressFrame(1);
			if (SlowTask.ShouldCancel())
			{
				bCancel = true;
				break;
			}
			if (bMayDeleteRedirectors && ISourceControlModule::Get().IsEnabled())
			{
				FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(RedirectorRefs.Redirector->GetOutermost(), EStateCacheUsage::Use);
				const bool bValidSCCState = !SourceControlState.IsValid() || SourceControlState->IsAdded() || SourceControlState->IsCheckedOut() || SourceControlState->CanCheckout() || !SourceControlState->IsSourceControlled() || SourceControlState->IsIgnored();

				if (!bValidSCCState)
				{
					RedirectorRefs.bSCCError = true;
					// Continue to load the referencers because we may still be able to fix them up 
				}
			}

			// Load all referencers
			for (FName ReferencingPackageName : RedirectorRefs.ReferencingPackageNames)
			{
				FNameBuilder PackageName{ReferencingPackageName};

				// Find the package in memory. If it is not in memory, try to load it
				UPackage* Package = FindPackage(nullptr, *PackageName);
				if (!Package)
				{
					Package = LoadPackage(nullptr, *PackageName, LOAD_None);
					if (Package)
					{
						LoadedPackages.Add(Package);
					}
				}

				if (Package)
				{
					if (Package->HasAnyPackageFlags(PKG_CompiledIn))
					{
						// This is a script reference
						RedirectorRefs.OtherFailures.Add(FText::Format(LOCTEXT("RedirectorFixupFailed_CodeReference", "Redirector is referenced by code. Package: {0}"), FText::FromName(ReferencingPackageName)));
					}
					else
					{
						// If we found a valid package, mark it for save
						ReferencingPackagesToSave.Add(Package);
					}
				}
			}
		}
	}

	ON_SCOPE_EXIT {
		// If any packages were loaded during the fixup process, make sure we unload them here
		if (!LoadedPackages.IsEmpty())
		{
			FText ErrorMessage;
			UPackageTools::UnloadPackages(LoadedPackages.Array(), ErrorMessage, true);
			if (!ErrorMessage.IsEmpty())
			{
				FTextBuilder Builder;
				Builder.AppendLine(LOCTEXT("ErrorsUnloadingPackages", "Errors were encountered unloading packages which were loaded to update redirector references. Some assets may still be loaded. "));
				Builder.AppendLine();
				Builder.AppendLine(ErrorMessage);
				FMessageDialog::Open(EAppMsgType::Ok, Builder.ToText());
			}
		}
	};
	
	if (bCancel)
	{
		return;
	}

	// Add all referencing packages objects that aren't RF_Standalone to the root set to avoid them being GC'd during the following processing
	TArray<TStrongObjectPtr<UObject>> RootedObjects;
	for (UPackage* Package : ReferencingPackagesToSave)
	{
		ForEachObjectWithPackage(Package, [&RootedObjects](UObject* Object)
		{
			RootedObjects.Emplace(Object);
			return true;
		}, false, RF_Standalone, EInternalObjectFlags::RootSet);
	}

	// Reset loaders of assets used in level instances to allow referencing packages to be saved
	TSet<FName> WorldAssetsNeedingLoadersReset;
	for (UPackage* Package : ReferencingPackagesToSave)
	{
		TArray<FAssetData> ReferencingPackageAssets;
		if (AssetRegistryModule.Get().GetAssetsByPackageName(Package->GetFName(), ReferencingPackageAssets, /*bIncludeOnlyOnDiskAssets=*/true))
		{
			for (const FAssetData& Asset : ReferencingPackageAssets)
			{
				if (!Asset.GetOptionalOuterPathName().IsNone())
				{
					WorldAssetsNeedingLoadersReset.Add(FSoftObjectPath(Asset.GetOptionalOuterPathName().ToString()).GetLongPackageFName());
				}
			}
		}
	}
	for (const FName& WorldAsset : WorldAssetsNeedingLoadersReset)
	{
		ULevelInstanceSubsystem::ResetLoadersForWorldAsset(*WorldAsset.ToString());
	}
	
	// Check out all referencing packages, leave redirectors for assets referenced by packages that are not checked out and remove those packages from the save list.
	bool bUserAcceptedCheckout = true; // If source control is disabled, assume checkout was selected
	if (ISourceControlModule::Get().IsEnabled() && ReferencingPackagesToSave.Num() > 0)
	{
		TArray<UPackage*> PackagesCheckedOutOrMadeWritable;
		TArray<UPackage*> PackagesNotNeedingCheckout;
		const bool bErrorIfAlreadyCheckedOut = false;
		const bool bConfirmPackageBranchCheckOutStatus = false;
		FEditorFileUtils::CheckoutPackages(ReferencingPackagesToSave.Array(), &PackagesCheckedOutOrMadeWritable, bErrorIfAlreadyCheckedOut, bConfirmPackageBranchCheckOutStatus);

		if (bUserAcceptedCheckout)
		{
			TSet<UPackage*> PackagesThatCouldNotBeCheckedOut = ReferencingPackagesToSave;
			for (UPackage* Package : PackagesCheckedOutOrMadeWritable)
			{
				PackagesThatCouldNotBeCheckedOut.Remove(Package);
			}
			for (UPackage* Package : PackagesNotNeedingCheckout)
			{
				PackagesThatCouldNotBeCheckedOut.Remove(Package);
			}

			for (UPackage* Package : PackagesThatCouldNotBeCheckedOut)
			{
				FName PackageName = Package->GetFName(); // Key iterator requires we copy this

				// Note which redirectors will still be required because this package could not be checked out 
				for (auto It = ReferencingAssetToRedirector.CreateKeyIterator(PackageName); It; ++It)
				{
					It.Value()->LockedReferencerPackageNames.Add(Package->GetFName());
				}

				// Don't save anything that wasn't checked out 
				ReferencingPackagesToSave.Remove(Package);
			}
		}
	}

	if (bUserAcceptedCheckout)
	{
		// If any referencing packages are left read-only, the checkout failed or SCC was not enabled. Trim them from the save list and leave redirectors.
		for (auto It = ReferencingPackagesToSave.CreateIterator(); It; ++It)
		{
			UPackage* Package = *It;
			if (!ensure(Package))
			{
				It.RemoveCurrent();
				continue;
			}
			
			// If the file is read only
			FString Filename;
			if (FPackageName::DoesPackageExist(Package->GetName(), &Filename) 
			&& IFileManager::Get().IsReadOnly(*Filename))
			{
				// Note which redirectors will still be required because this package was read only
				FName PackageName = Package->GetFName(); // Key iterator requires we copy this
				for (auto RedirectorIt = ReferencingAssetToRedirector.CreateKeyIterator(PackageName); RedirectorIt; ++RedirectorIt)
				{
					RedirectorIt.Value()->LockedReferencerPackageNames.Add(Package->GetFName());
				}

				// Remove the package from the save list
				It.RemoveCurrent();
			}
		}

		// Fix up referencing FSoftObjectPaths
		{
			TSet<UPackage*> PackagesToCheck = ReferencingPackagesToSave;

			TArray<UPackage*> Tmp;
			FEditorFileUtils::GetDirtyWorldPackages(Tmp);
			FEditorFileUtils::GetDirtyContentPackages(Tmp);
			PackagesToCheck.Append(Tmp);

			TMap<FSoftObjectPath, FSoftObjectPath> RedirectorMap;
			for (const CustomFRedirectorRefs& RedirectorRef : RedirectorRefsList)
			{
				const UObjectRedirector* Redirector = RedirectorRef.Redirector.Get();
				FSoftObjectPath OldPath = FSoftObjectPath(Redirector);
				FSoftObjectPath NewPath = FSoftObjectPath(Redirector->DestinationObject);

				RedirectorMap.Add(OldPath, NewPath);
				if (UBlueprint* Blueprint = Cast<UBlueprint>(Redirector->DestinationObject))
				{
					// Add redirect for class and default as well
					RedirectorMap.Add(FString::Printf(TEXT("%s_C"), *OldPath.ToString()), FString::Printf(TEXT("%s_C"), *NewPath.ToString()));
					RedirectorMap.Add(FString::Printf(TEXT("%s.Default__%s_C"), *OldPath.GetLongPackageName(), *OldPath.GetAssetName()), FString::Printf(TEXT("%s.Default__%s_C"), *NewPath.GetLongPackageName(), *NewPath.GetAssetName()));
				}
			}

			RenameReferencingSoftObjectPaths(PackagesToCheck.Array(), RedirectorMap);
		}

		// Save all packages that were referencing any of the assets that were moved without redirectors
		TArray<UPackage*> FailedToSave;
		if (ReferencingPackagesToSave.Num() > 0)
		{
			// Get the list of filenames before calling save because some of the saved packages can get GCed if they are empty packages
			const TArray<FString> Filenames = USourceControlHelpers::PackageFilenames(ReferencingPackagesToSave.Array());

			const bool bCheckDirty = false;
			const bool bPromptToSave = false;
			FEditorFileUtils::PromptForCheckoutAndSave(ReferencingPackagesToSave.Array(), bCheckDirty, bPromptToSave, &FailedToSave);
			for (UPackage* Package : FailedToSave)
			{
				FName PackageName = Package->GetFName(); // Key iterator requires we copy this
				for (auto It = ReferencingAssetToRedirector.CreateKeyIterator(PackageName); It; ++It)	
				{
					It.Value()->FailedReferencerPackageNames.Add(Package->GetFName());
				}
			}

			ISourceControlModule::Get().QueueStatusUpdate(Filenames);
		}

		// Save any collections that were referencing any of the redirectors
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

		// Find all collections that were referenced by any of the redirectors that are potentially going to be removed and attempt to re-save them
		// The redirectors themselves will have already been fixed up, as collections do that once the asset registry has been populated, 
		// however collections lazily re-save redirector fix-up to avoid SCC issues, so we need to force that now
		for (CustomFRedirectorRefs& RedirectorRefs : RedirectorRefsList)
		{
			// Follow each link in the redirector, and notify the collections manager that it is going to be removed - this will force it to re-save any required collections
			for (const UObjectRedirector* Redirector = RedirectorRefs.Redirector.Get(); Redirector; Redirector = Cast<UObjectRedirector>(Redirector->DestinationObject))
			{
				const FSoftObjectPath RedirectorObjectPath = FSoftObjectPath(Redirector);
				FText Error;
				if (!CollectionManagerModule.Get().HandleRedirectorDeleted(RedirectorObjectPath, &Error))
				{
					RedirectorRefs.OtherFailures.Add(FText::Format(LOCTEXT("RedirectorFixupFailed_CollectionsFailedToSave", "Referencing collection(s) failed to save: {0}"), Error));
				}
			}
		}

		// Wait for package referencers to be updated
		// Load the asset registry module
		{
			TArray<FString> AssetPaths;
			for (const CustomFRedirectorRefs& Redirector : RedirectorRefsList)
			{
				AssetPaths.AddUnique(FPackageName::GetLongPackagePath(Redirector.RedirectorPackageName.ToString()) / TEXT("")); // Ensure trailing slash

				for (const auto& Referencer : Redirector.ReferencingPackageNames)
				{
					AssetPaths.AddUnique(FPackageName::GetLongPackagePath(Referencer.ToString()) / TEXT("")); // Ensure trailing slash
				}
			}
			AssetRegistryModule.Get().ScanPathsSynchronous(AssetPaths, true);
		}

		// Show user report and check whether we should delete
		bool bCanDelete = true;
		
		TArray<UObject*> ObjectsToDelete;
		for (CustomFRedirectorRefs& RedirectorRefs : RedirectorRefsList)
		{
			if (RedirectorRefs.OtherFailures.IsEmpty() 
			&&  RedirectorRefs.LockedReferencerPackageNames.IsEmpty()
			&& 	RedirectorRefs.FailedReferencerPackageNames.IsEmpty())
			{
				ensure(RedirectorRefs.Redirector);
				// Add all redirectors found in this package to the redirectors to delete list.
				// All redirectors in this package should be fixed up.
				UPackage* RedirectorPackage = RedirectorRefs.Redirector->GetOutermost();
				UMetaData* PackageMetaData = nullptr;
				bool bContainsAtLeastOneOtherAsset = false;
				ForEachObjectWithOuter(RedirectorPackage, [&PackageMetaData, &ObjectsToDelete, &bContainsAtLeastOneOtherAsset](UObject* Obj)
				{
					if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(Obj))
					{
						Redirector->RemoveFromRoot();
						ObjectsToDelete.Add(Redirector);
					}
					else if (UMetaData* MetaData = Cast<UMetaData>(Obj))
					{
						PackageMetaData = MetaData;
					}
					else
					{
						bContainsAtLeastOneOtherAsset = true;
					}
				});

				if ( !bContainsAtLeastOneOtherAsset )
				{
					RedirectorPackage->RemoveFromRoot();
					ObjectsToDelete.Add(RedirectorPackage);

					// @todo we shouldnt be worrying about metadata objects here, ObjectTools::CleanUpAfterSuccessfulDelete should
					if ( PackageMetaData )
					{
						PackageMetaData->RemoveFromRoot();
						ObjectsToDelete.Add(PackageMetaData);
					}
				}
			}
		}
		
		// Release all redirector references before executing delete operation
		RedirectorRefsList.Empty();

		if ( ObjectsToDelete.Num() > 0 )
		{
			ObjectTools::DeleteObjects(ObjectsToDelete, false);
		}
	}
}

void UAssetsToolCheckActions::RenameReferencingSoftObjectPaths(const TArray<UPackage*> PackagesToCheck,
	const TMap<FSoftObjectPath, FSoftObjectPath>& AssetRedirectorMap)
{
	// Add redirects as needed
	for (const TPair<FSoftObjectPath, FSoftObjectPath>& Pair : AssetRedirectorMap)
	{
		if (Pair.Key.IsAsset())
		{
			GRedirectCollector.AddAssetPathRedirection(Pair.Key.GetWithoutSubPath(), Pair.Value.GetWithoutSubPath());
		}
	}

	FCustomSoftObjectPathRenameSerializer RenameSerializer(AssetRedirectorMap, false, nullptr);

	for (UPackage* Package : PackagesToCheck)
	{
		TArray<UObject*> ObjectsInPackage;
		GetObjectsWithPackage(Package, ObjectsInPackage);

		for (UObject* Object : ObjectsInPackage)
		{
			if (!IsValid(Object))
			{
				continue;
			}

			RenameSerializer.StartSerializingObject(Object);
			Object->Serialize(RenameSerializer);

			if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
			{
				// Serialize may have dirtied the BP bytecode in some way
				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			}
		}
	}
}

void UAssetsToolCheckActions::NativeForceUnlockPackage(UPackage* Package)
{
	if (!Package) return;

	check(IsInGameThread());

	FlushAsyncLoading();
    
	TArray<UObject*> ObjectsInPackage;
	GetObjectsWithPackage(Package, ObjectsInPackage);
	for (UObject* Obj : ObjectsInPackage)
	{
		Obj->RemoveFromRoot();
		Obj->MarkAsGarbage();
	}

	Package->RemoveFromRoot();
	Package->MarkAsGarbage();

	const bool bPerformFullPurge = true;
	CollectGarbage(RF_NoFlags, bPerformFullPurge);
}

bool UAssetsToolCheckActions::SafeSavePackage(UPackage* Pkg)
{
    if (!Pkg || Pkg->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        UE_LOG(LogTemp, Error, TEXT("包已销毁或无效"));
        return false;
    }

    const FString PackageName = Pkg->GetName();
    FString FileName;
    if (!FPackageName::DoesPackageExist(PackageName, &FileName))
    {
        UE_LOG(LogTemp, Error, TEXT("包文件不存在: %s"), *PackageName);
        return false;
    }
    FileName = FPaths::ConvertRelativePathToFull(FileName);

    // 处理文件锁
    if (NativeIsFileLocked(PackageName))
    {
        NativeForceUnlockPackage(Pkg);
        UE_LOG(LogTemp, Log, TEXT("▷▷▷ 包被占用，主动释放: %s"), *PackageName);

        UPackage* ReloadedPackage = LoadPackage(nullptr, *PackageName, LOAD_None);
        if (!ReloadedPackage)
        {
            UE_LOG(LogTemp, Error, TEXT("重新加载包失败: %s"), *PackageName);
            return false;
        }
        Pkg = ReloadedPackage;
    }

    // 加载依赖项
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FName> Dependencies;
    AssetRegistryModule.Get().GetDependencies(FName(*PackageName), Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
	for (const FName& Dependency : Dependencies)
	{
		// 创建临时包上下文
		UPackage* TempPackage = GetTransientPackage();
        
		// 加载依赖包
		UPackage* LoadedPackage = LoadPackage(
			TempPackage, 
			*Dependency.ToString(), 
			LOAD_None, 
			nullptr, 
			nullptr
		);

		// 验证加载结果
		if (!LoadedPackage || LoadedPackage->IsGarbageEliminationEnabled())
		{
			UE_LOG(LogTemp, Error, TEXT("依赖包加载失败: %s"), *Dependency.ToString());
			continue;
		}
	}

    ResetLoaders(Pkg);
    
    // 执行保存
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Standalone;
    SaveArgs.SaveFlags = SAVE_KeepDirty;
    const bool bSaveResult = GEditor->SavePackage(Pkg, nullptr, *FileName, SaveArgs);

    // 错误处理
    if (!bSaveResult)
    {
        const DWORD LastError = FPlatformMisc::GetLastError();
        TCHAR ErrorBuffer[1024];
        FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 1024, LastError);
        UE_LOG(LogTemp, Error, TEXT("保存失败! 错误码: %d | 描述: %s"), LastError, ErrorBuffer);
        return false;
    }

    return true;
}

bool UAssetsToolCheckActions::NativeIsFileLocked(const FString& FileName)
{
#if PLATFORM_WINDOWS
	HANDLE hFile = CreateFileW(
		*FileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
    
	const bool bLocked = (hFile == INVALID_HANDLE_VALUE);
	if (!bLocked) CloseHandle(hFile);
	return bLocked;
#else
#endif
}

FString UAssetsToolCheckActions::NativeGetDefaultAssetListPath()
{
	return FPaths::Combine(
	   FPaths::ProjectSavedDir(),      
	   TEXT("AssetsTool"),         
	   TEXT("AListOfReferencesToBeProcessed.txt")
   );
}

FString UAssetsToolCheckActions::ConvertGamePathToPhysical(const FString& GamePath)
{
	static const FString ContentDir = FPaths::ProjectContentDir();
    
	// 验证输入路径格式
	if (!GamePath.StartsWith(TEXT("/Game/")))
	{
		UE_LOG(LogTemp, Warning, TEXT("非法虚拟路径格式: %s"), *GamePath);
		return FString();
	}

	// 转换路径
	FString RelativePath = GamePath.RightChop(6); // 移除前6个字符"/Game/"
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(ContentDir, RelativePath)
	);
}

#undef LOCTEXT_NAMESPACE