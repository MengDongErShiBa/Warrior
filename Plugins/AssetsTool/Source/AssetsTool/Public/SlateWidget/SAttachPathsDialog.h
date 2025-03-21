#pragma once

// struct FTypePathData
// {
// 	TMap<UClass*, FString>* TypePaths;
// };

class SAttachPathsDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAttachPathsDialog) {}
	// 	: _TypePathData(FTypePathData())
	// {}
	// SLATE_ARGUMENT(FTypePathData, TypePathData)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs);

	/** Opens the dialog in a new window */
	static void OpenAttachPathsDialog(TMap<UClass*, FString>* TypePaths);

	TSharedRef<SWidget> CreateEditablePathRow(UClass* Class, FString& Path);
	
	TSharedRef<SVerticalBox> CreateAllEditablePaths();

	/** Closes the dialog. */
	void CloseDialog();
private:
	/** Handler for when "Ok" is clicked */
	FReply OkClicked();

	/** Handler for when "Cancel" is clicked */
	FReply CancelClicked();

private:
	/** Brushes for the different node states */
	const FSlateBrush* FolderOpenBrush;
	const FSlateBrush* FolderClosedBrush;
	const FSlateBrush* PackageBrush;

	// FTypePathData TypePathData;
};
