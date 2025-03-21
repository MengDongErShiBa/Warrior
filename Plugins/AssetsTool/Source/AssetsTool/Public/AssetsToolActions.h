#pragma once
#include "AssetActionUtility.h"
#include "SlateWidget/SScreeningAssetsDialog.h"
#include "AssetsToolActions.generated.h"

/**
 * 暂时设计为支持所有资产...在代码逻辑中自己处理先
 */

struct FAssetRenameData;

UCLASS(BlueprintType, meta = (DisplayName = "资产检查工具"))
class UAssetsToolActions : public UObject
{
	GENERATED_BODY()
public:

	UAssetsToolActions();
	
	/**
	 * 批量移动
	 */
	UFUNCTION(CallInEditor, Category = "JyValidation", meta = (DisplayName = "批量移动", ToolTip = "移动选中资产"))
	void BatchMovement();

	/**
	 * 资产筛选
	 */
	UFUNCTION(CallInEditor, Category = "JyValidation", meta = (DisplayName = "资产筛选", ToolTip = "筛选选中的所有资产"))
	void ScreeningAssets();

	/**
	 * 拷贝数据结构
	 */
	void CopyStructure();

	/**
	 * 递归拿文件路径
	 * @param RootPath 根目录
	 * @param CurrentPath 当前目录
	 * @param OutRelativePaths Out列表
	 */
	void CollectSubdirectoriesRecursively(const FString& RootPath, const FString& CurrentPath, TArray<FString>& OutRelativePaths);

	FString GetDestinationFolder() const;

	UFUNCTION()
	void InitContentBrowserExtend();

	void PerformMigratePackages(TArray<FName> PackageNamesToMigrate, const FString DestinationPath) const;
	void RecursiveGetDependencies(const FName& PackageName, TSet<FName>& AllDependencies, TSet<FString>& ExternalObjectsPaths, TSet<FName>& ExcludedDependencies, const TFunction<bool(FName)>& ShouldExcludeFromDependenciesSearch) const;
	void MigratePackages_ReportConfirmed(TSharedPtr<TArray<ReportPackageData>> PackageDataToMigrate, const FString DestinationPath) const;
	void ResetPaths() const;

	TArray<FAssetData> GetAllAssetsInFolders(const TArray<FString>& FolderPaths);

	void AssemblyRenameData(FAssetRenameData& Data, const FAssetData Asset, const FString DestinationFolder) const;

	void ActiveTriggerMarkDirty() const;

	/**
	 * 获取最终目录
	 * @return 
	 */
	bool GetReallyPath(UClass* Type, FString& OutPath) const;

	// 选中的文件路径
	mutable TArray<FString> FolderPathsSelected;

	// 选中的资产
	mutable TArray<FAssetData> SelectedAssets;

	// 迁移完毕后还需要处理的资产
	mutable TArray<FAssetData> DataToBeProcessed;

	UPROPERTY()
	mutable TMap<UClass*, FString> DefaultTypePaths;
	UPROPERTY()
	mutable UTypePathContainer* CustomTypePathContainer;
};
