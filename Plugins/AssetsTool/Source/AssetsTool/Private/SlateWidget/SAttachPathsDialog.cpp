#include "SlateWidget/SAttachPathsDialog.h"

#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "PackageReportDialog"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAttachPathsDialog::Construct( const FArguments& InArgs)
{
	FolderOpenBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen");
	FolderClosedBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
	PackageBrush = FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon");

	// TypePathData = InArgs._TypePathData;
	
	const FText ReportMessage = LOCTEXT("MigratePackagesReportTitle", "请指定各类型目录名称");

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
				.Text(ReportMessage)
				.TextStyle( FAppStyle::Get(), "PackageMigration.DialogTitle" )
			]

			+SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
				.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						CreateAllEditablePaths()
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
					.OnClicked(this, &SAttachPathsDialog::OkClicked)
					.Text(LOCTEXT("OkButton", "确认"))
				]
				+SUniformGridPanel::Slot(1,0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding( FAppStyle::GetMargin("StandardDialog.ContentPadding") )
					.OnClicked(this, &SAttachPathsDialog::CancelClicked)
					.Text(LOCTEXT("CancelButton", "取消"))
				]
			]
		]
	];

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAttachPathsDialog::OpenAttachPathsDialog(TMap<UClass*, FString>* TypePaths)
{
	// FTypePathData Data;
	// Data.TypePaths = TypePaths;
	
	TSharedRef<SWindow> ReportWindow = SNew(SWindow)
		.Title(LOCTEXT("ReportWindowTitle", "资产筛选"))
		.ClientSize( FVector2D(300, 500) )
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SAttachPathsDialog)
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

TSharedRef<SWidget> SAttachPathsDialog::CreateEditablePathRow(UClass* Class, FString& Path)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Class->GetDisplayNameText())
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(2.0f, 0.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(*Path))
			.OnTextChanged_Lambda([this,Class](const FText& NewText) {
	              // if (&TypePathData.TypePaths)
	              // {
	              //     TypePathData.TypePaths->FindOrAdd(Class) = NewText.ToString();
	              // }
			})
		];
}

TSharedRef<SVerticalBox> SAttachPathsDialog::CreateAllEditablePaths()
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	// for (auto Entry : *TypePathData.TypePaths)
	// {
	// 	Container->AddSlot()
	// 	.AutoHeight()
	// 	.Padding(0.0f, 2.0f)
	// 	[
	// 		CreateEditablePathRow(Entry.Key, Entry.Value)
	// 	];
	// }

	return Container;
}

void SAttachPathsDialog::CloseDialog()
{
	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());

	// for (auto Entry : *TypePathData.TypePaths)
	// {
	// 	UE_LOG(LogTemp, Log, TEXT("UClass:%s, 文件夹名称：%s"), *Entry.Key->GetDisplayNameText().ToString(), *Entry.Value)
	// }

	if ( Window.IsValid() )
	{
		Window->RequestDestroyWindow();
	}
}

FReply SAttachPathsDialog::OkClicked()
{
	CloseDialog();

	return FReply::Handled();
}

FReply SAttachPathsDialog::CancelClicked()
{
	CloseDialog();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
