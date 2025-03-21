// Copyright Epic Games, Inc. All Rights Reserved.


#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "IDesktopPlatform.h"
#include "..\..\Public\SlateWidget\SScreeningAssetsDialog.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SWindow.h"
#include "Layout/WidgetPath.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Input/STextEntryPopup.h"

#define LOCTEXT_NAMESPACE "PackageReportDialog"

struct FCompareFPackageReportNodeByName
{
	FORCEINLINE bool operator()( TSharedPtr<FPackageReportNode> A, TSharedPtr<FPackageReportNode> B ) const
	{
		return A->NodeName < B->NodeName;
	}
};

FPackageReportNode::FPackageReportNode()
	: CheckedState(ECheckBoxState::Undetermined)
	, bShouldMigratePackage(nullptr)
	, bIsFolder(false)
	, Parent(nullptr)
{}

FPackageReportNode::FPackageReportNode(const FString& InNodeName, bool InIsFolder)
	: NodeName(InNodeName)
	, CheckedState(ECheckBoxState::Undetermined)
	, bShouldMigratePackage(nullptr)
	, bIsFolder(InIsFolder)
	, Parent(nullptr)
{}

void FPackageReportNode::AddPackage(const FString& PackageName, bool* bInShouldMigratePackage)
{
	TArray<FString> PathElements;
	PackageName.ParseIntoArray(PathElements, TEXT("/"), /*InCullEmpty=*/true);

	(void)AddPackage_Recursive(PathElements, bInShouldMigratePackage);
}

void FPackageReportNode::ExpandChildrenRecursively(const TSharedRef<PackageReportTree>& Treeview)
{
	for ( auto ChildIt = Children.CreateConstIterator(); ChildIt; ++ChildIt )
	{
		Treeview->SetItemExpansion(*ChildIt, (*ChildIt)->CheckedState != ECheckBoxState::Unchecked);
		(*ChildIt)->ExpandChildrenRecursively(Treeview);
	}
}

FPackageReportNode::FChildrenState FPackageReportNode::AddPackage_Recursive(TArray<FString>& PathElements, bool* bInShouldMigratePackage)
{
	FChildrenState ChildrenState;
	ChildrenState.bAnyChildIsChecked = false;
	ChildrenState.bAllChildrenAreChecked = true;

	if ( PathElements.Num() > 0 )
	{
		// Pop the bottom element
		FString ChildNodeName = PathElements[0];
		PathElements.RemoveAt(0);

		// Try to find a child which uses this folder name
		TSharedPtr<FPackageReportNode> Child;
		for ( auto ChildIt = Children.CreateConstIterator(); ChildIt; ++ChildIt )
		{
			if (*ChildIt == nullptr) continue;
			
			if ( (*ChildIt)->NodeName == ChildNodeName )
			{
				Child = (*ChildIt);
				break;
			}
		}
		
		// If one was not found, create it
		if ( !Child.IsValid() || Child == nullptr)
		{
			const bool bIsAFolder = (PathElements.Num() > 0);
			int32 ChildIdx = Children.Add( MakeShareable(new FPackageReportNode(ChildNodeName, bIsAFolder)) );
			Child = Children[ChildIdx];
			Child.Get()->Parent = this;
			Children.Sort( FCompareFPackageReportNodeByName() );
		}

		if ( ensure(Child.IsValid()) )
		{
			FChildrenState ChildChildrenState = Child->AddPackage_Recursive(PathElements, bInShouldMigratePackage);
			ChildrenState.bAnyChildIsChecked |= ChildChildrenState.bAnyChildIsChecked;
			ChildrenState.bAllChildrenAreChecked &= ChildChildrenState.bAllChildrenAreChecked;
		}

		CheckedState = ChildrenState.bAllChildrenAreChecked ? ECheckBoxState::Checked : (ChildrenState.bAnyChildIsChecked ? ECheckBoxState::Undetermined : ECheckBoxState::Unchecked);
	}
	else
	{
		CheckedState = *bInShouldMigratePackage ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		ChildrenState.bAnyChildIsChecked = ChildrenState.bAllChildrenAreChecked = CheckedState == ECheckBoxState::Checked;
		bShouldMigratePackage = bInShouldMigratePackage;
	}

	return ChildrenState;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SScreeningAssetsDialog::Construct( const FArguments& InArgs, const FText& InReportMessage, TArray<ReportPackageData>& InPackageNames, const FOnReportConfirmed& InOnReportConfirmed, const FOnResetPathsConfirmed InOnAttachPathsConfirmed)
{
	TypePathData = InArgs._TypePathData;

	TypePathData.TypeChinese.Add(UStaticMesh::StaticClass(), TEXT("静态网格"));
	TypePathData.TypeChinese.Add(UTexture::StaticClass(), TEXT("贴图"));
	TypePathData.TypeChinese.Add(USkeletalMesh::StaticClass(), TEXT("骨骼网格"));
	TypePathData.TypeChinese.Add(UMaterial::StaticClass(), TEXT("材质"));
	TypePathData.TypeChinese.Add(USoundWave::StaticClass(), TEXT("声音"));
	TypePathData.TypeChinese.Add(UObject::StaticClass(), TEXT("其他"));
	
	OnReportConfirmed = InOnReportConfirmed;
	OnResetPathsConfirmed = InOnAttachPathsConfirmed;
	
	FolderOpenBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen");
	FolderClosedBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
	PackageBrush = FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon");

	ConstructNodeTree(InPackageNames);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Docking.Tab.ContentAreaBrush") )
		.Padding(FMargin(4, 8, 4, 4))
		[
			SNew(SVerticalBox)

			// Report Message
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 4)
			[
				SNew(STextBlock)
				.Text(InReportMessage)
				.TextStyle( FAppStyle::Get(), "PackageMigration.DialogTitle" )
			]

			// Tree of packages in the report
			+SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
				.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
				[
					SAssignNew( ReportTreeView, PackageReportTree )
					.TreeItemsSource(&PackageReportRootNode.Children)
					.ItemHeight(18)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow( this, &SScreeningAssetsDialog::GenerateTreeRow )
					.OnGetChildren( this, &SScreeningAssetsDialog::GetChildrenForTree )
				]
			]

			// 折叠区域
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 4)
			[
				SNew(SExpandableArea)
				.InitiallyCollapsed(true)
				.HeaderPadding(FMargin(4.0f))
				.HeaderContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TypePathsHeader", "类型指定路径配置（如选择则放入指定路径，不选择则放入指定路径的默认文件夹）"))
				]
				.BodyContent()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						CreateExpandableAreaContent()
					]
				]
			]
			
			// Ok/Cancel buttons
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0,4,0,0)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
				.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
				.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
				+SUniformGridPanel::Slot(0,0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding( FAppStyle::GetMargin("StandardDialog.ContentPadding") )
					.OnClicked(this, &SScreeningAssetsDialog::ResetPathsClicked)
					.Text(LOCTEXT("ResetPath", "重置为默认路径"))
				]
				+SUniformGridPanel::Slot(1,0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding( FAppStyle::GetMargin("StandardDialog.ContentPadding") )
					.OnClicked(this, &SScreeningAssetsDialog::OkClicked)
					.Text(LOCTEXT("OkButton", "确认"))
				]
				+SUniformGridPanel::Slot(2,0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding( FAppStyle::GetMargin("StandardDialog.ContentPadding") )
					.OnClicked(this, &SScreeningAssetsDialog::CancelClicked)
					.Text(LOCTEXT("CancelButton", "取消"))
				]
			]
		]
	];

	if ( ensure(ReportTreeView.IsValid()) )
	{
		PackageReportRootNode.ExpandChildrenRecursively(ReportTreeView.ToSharedRef());
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SScreeningAssetsDialog::OpenPackageReportDialog(const FText& ReportMessage, TArray<ReportPackageData>& PackageNames, const FOnReportConfirmed& InOnReportConfirmed, const FOnResetPathsConfirmed InOnAttachPathsConfirmed, UTypePathContainer* TypeContainer)
{

	FTypePathData Data;
	Data.TypePathsContainer = TypeContainer;
	
	TSharedRef<SWindow> ReportWindow = SNew(SWindow)
		.Title(LOCTEXT("ReportWindowTitle", "资产筛选"))
		.ClientSize( FVector2D(600, 500) )
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SScreeningAssetsDialog, ReportMessage, PackageNames, InOnReportConfirmed, InOnAttachPathsConfirmed).TypePathData(Data)
		];
		
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	if ( MainFrameModule.GetParentWindow().IsValid() )
	{
		FSlateApplication::Get().AddWindowAsNativeChild(ReportWindow, MainFrameModule.GetParentWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(ReportWindow);
	}
}

void SScreeningAssetsDialog::CloseDialog()
{
	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());

	if ( Window.IsValid() )
	{
		Window->RequestDestroyWindow();
	}
}

TSharedRef<ITableRow> SScreeningAssetsDialog::GenerateTreeRow( TSharedPtr<FPackageReportNode> TreeItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(TreeItem.IsValid());

	const FSlateBrush* IconBrush = GetNodeIcon(TreeItem);

	return SNew( STableRow< TSharedPtr<FPackageReportNode> >, OwnerTable )
		[
			// Icon
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SScreeningAssetsDialog::CheckBoxStateChanged, TreeItem, OwnerTable)
				.IsChecked(this, &SScreeningAssetsDialog::GetEnabledCheckState, TreeItem)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage).Image(IconBrush)
			]
			// Name
			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(STextBlock).Text(FText::FromString(TreeItem->NodeName))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
}

ECheckBoxState SScreeningAssetsDialog::GetEnabledCheckState(TSharedPtr<FPackageReportNode> TreeItem) const
{
	return TreeItem.Get()->CheckedState;
}

void SScreeningAssetsDialog::SetStateRecursive(TSharedPtr<FPackageReportNode> TreeItem, bool bIsChecked)
{
	if (TreeItem.Get() == nullptr)
	{
		return;
	}

	TreeItem.Get()->CheckedState = bIsChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

	if (TreeItem.Get()->bShouldMigratePackage)
	{
		*(TreeItem.Get()->bShouldMigratePackage) = bIsChecked;
	}

	TArray< TSharedPtr<FPackageReportNode> > Children;
	GetChildrenForTree(TreeItem, Children);
	for (int i = 0; i < Children.Num(); i++)
	{
		if (Children[i].Get() == nullptr)
		{
			continue;
		}

		SetStateRecursive(Children[i], bIsChecked);
	}
}

void SScreeningAssetsDialog::CheckBoxStateChanged(ECheckBoxState InCheckBoxState, TSharedPtr<FPackageReportNode> TreeItem, TSharedRef<STableViewBase> OwnerTable)
{
	SetStateRecursive(TreeItem, InCheckBoxState == ECheckBoxState::Checked);

	FPackageReportNode* CurrentParent = TreeItem->Parent;
	while (CurrentParent != nullptr)
	{
		bool bAnyChildIsChecked = false;
		bool bAllChildrenAreChecked = true;
		for (int i = 0; i < CurrentParent->Children.Num(); i++)
		{
			bAnyChildIsChecked |= CurrentParent->Children[i]->CheckedState != ECheckBoxState::Unchecked;
			bAllChildrenAreChecked &= CurrentParent->Children[i]->CheckedState != ECheckBoxState::Unchecked;
		}

		CurrentParent->CheckedState = bAllChildrenAreChecked ? ECheckBoxState::Checked : (bAnyChildIsChecked ? ECheckBoxState::Undetermined : ECheckBoxState::Unchecked);
		CurrentParent = CurrentParent->Parent;
	}

	OwnerTable.Get().RebuildList();
}

void SScreeningAssetsDialog::GetChildrenForTree( TSharedPtr<FPackageReportNode> TreeItem, TArray< TSharedPtr<FPackageReportNode> >& OutChildren )
{
	OutChildren = TreeItem->Children;
}

void SScreeningAssetsDialog::ConstructNodeTree(TArray<ReportPackageData>& PackageNames)
{
	for (ReportPackageData& Package : PackageNames)
	{
		PackageReportRootNode.AddPackage(Package.Name, &Package.bShouldMigratePackage);
	}
}

const FSlateBrush* SScreeningAssetsDialog::GetNodeIcon(const TSharedPtr<FPackageReportNode>& ReportNode) const
{
	if ( !ReportNode->bIsFolder )
	{
		return PackageBrush;
	}
	else if ( ReportTreeView->IsItemExpanded(ReportNode) )
	{
		return FolderOpenBrush;
	}
	else
	{
		return FolderClosedBrush;
	}
}

FReply SScreeningAssetsDialog::ResetPathsClicked()
{
	OnResetPathsConfirmed.ExecuteIfBound();

	RefreshExpandableAreaContent();

	return FReply::Handled();
}

FReply SScreeningAssetsDialog::OkClicked()
{
	CloseDialog();
	OnReportConfirmed.ExecuteIfBound();

	return FReply::Handled();
}

FReply SScreeningAssetsDialog::CancelClicked()
{
	CloseDialog();

	return FReply::Handled();
}

FReply SScreeningAssetsDialog::OnModifyPathClicked(UClass* TargetClass)
{
	FString* TargetPathPtr = TypePathData.GetTypePaths().Find(TargetClass); // 安全查找
	if (!TargetPathPtr)
	{
		return FReply::Handled(); // 键不存在，直接返回
	}

	const FString TargetFolder = SelectedFolder();
	
	if (!TargetFolder.IsEmpty())
	{
		*TargetPathPtr = TargetFolder; // 直接修改 TMap 中的值

		// 刷新Widget
		RefreshExpandableAreaContent();
	}

	return FReply::Handled();
}

FString SScreeningAssetsDialog::SelectedFolder()
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
				return DestinationFolder;
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
					return DestinationFolder;
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
					return DestinationFolder;
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
		return DestinationFolder;
	}

	return DestinationFolder;
}

TSharedRef<SVerticalBox> SScreeningAssetsDialog::CreateExpandableAreaContent()
{
	TSharedRef<SVerticalBox> EntryContainer = SNew(SVerticalBox);

	ExpandableArea = EntryContainer;

	if (TypePathData.GetTypePaths().IsEmpty())
	{
		// 无效数据
		return EntryContainer;
	}
	
	// 遍历所有类型路径，生成条目
	for (auto Entry : TypePathData.GetTypePaths())
	{
		UClass* Class = Entry.Key;
		const FString& Path = Entry.Value;

		FString ClassTypeName = TypePathData.TypeChinese.FindOrAdd(Class).IsEmpty() ? Class->GetDisplayNameText().ToString() : TypePathData.TypeChinese.FindOrAdd(Class);

		EntryContainer->AddSlot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(SHorizontalBox)
			// 类型名称
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ClassTypeName))
			]
			// 路径显示
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4, 0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Path]() { 
					FString VirtualPath;
					if (FPackageName::TryConvertFilenameToLongPackageName(Path, VirtualPath))
					{
						return FText::FromString(VirtualPath); 
					}
					else
					{
						return FText::FromString("/" + Path); 
					}
				})
			]
			// 修改按钮
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("ModifyButton", "更改"))
				.OnClicked_Lambda([this, Class]() { 
					return OnModifyPathClicked(Class); 
				})
			]
		];
	}

	return EntryContainer;
}

void SScreeningAssetsDialog::RefreshExpandableAreaContent()
{
	if (TypePathData.GetTypePaths().IsEmpty())
	{
		// 无效数据
		return;
	}

	// 清空
	ExpandableArea->ClearChildren();
	
	// 再次生成
	for (auto Entry : TypePathData.GetTypePaths())
	{
		UClass* Class = Entry.Key;
		const FString& Path = Entry.Value;

		FString ClassTypeName = TypePathData.TypeChinese.FindOrAdd(Class).IsEmpty() ? Class->GetDisplayNameText().ToString() : TypePathData.TypeChinese.FindOrAdd(Class);

		ExpandableArea->AddSlot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(SHorizontalBox)
			// 类型名称
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ClassTypeName))
			]
			// 路径显示
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4, 0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Path]() {
					FString VirtualPath;
					if (FPackageName::TryConvertFilenameToLongPackageName(Path, VirtualPath))
					{
						return FText::FromString(VirtualPath); 
					}
					else
					{
						return FText::FromString(Path); 
					}
				})
			]
			// 修改按钮
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("ModifyButton", "修改"))
				.OnClicked_Lambda([this, Class]() { 
					return OnModifyPathClicked(Class); 
				})
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
