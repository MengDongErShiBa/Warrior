#pragma once

#include "AssetsToolCheckActions.generated.h"

UCLASS(BlueprintType)
class UAssetsToolCheckActions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * 保存资产列表到本地
	 * @param AssetPackageNames 需要处理的引用
	 * @param TargetPath 这个路径暂时没有想到怎么配置，暂时写死用Save/AssetsTool/AListOfReferencesToBeProcessed.text
	 * @return 
	 */
	static bool NativeSaveTheAssetListLocally(const TArray<FString>& AssetPackageNames, const FString TargetPath = "");

	/**
	 * 是否存在需要处理的引用列表
	 * @param OutAssetPackageNames 引用列表
	 * @param TargetPath 目标路径，可为空默认
	 * @return 
	 */
	static bool NativeHaveTheListOfReferencesNeedProcess(TArray<FString>& OutAssetPackageNames, const FString TargetPath = "");

	/**
	 * 删除目标路径文件
	 * @param TargetPath 
	 * @return 
	 */
	static bool NativeDeleteAssetListFile(const FString& TargetPath);

	/**
	 * 检查文件路径是否有效
	 * @param TargetPath 目标路径 
	 * @param ValidationRoot Root目录，默认为Save
	 * @return 
	 */
	static FORCEINLINE bool NativeCheckPathValid(const FString& TargetPath, const FString& ValidationRoot = FPaths::ProjectSavedDir())
	{
		return !TargetPath.IsEmpty() 
			&& FPaths::ValidatePath(TargetPath) 
			&& FPaths::IsUnderDirectory(TargetPath, ValidationRoot);
	}
	
	/**
	 * 批量修复资产引用，会处理MarkDirty并保存
	 * @param AssetList 需要处理的资产列表
	 * @param bShowProgress 是否显示进度条
	 * @return 是否全部修复成功
	 */
	static bool NativeFixupAssetReferences(const TArray<FAssetData>& AssetList, bool bShowProgress = true);

	/**
	 * 给Commandlet用的批量修复
	 * @param AssetList 
	 * @return 
	 */
	static bool NativeCommandletFixupAssetReferences(const TArray<FAssetData>& AssetList);
	
	/**
	 * 将包名列表转换为AssetData列表
	 * @param PackageNames 输入包名列表（格式："'/Game/Path/AssetName.AssetName'"）
	 * @param OutAssetDatas 输出资产数据列表
	 * @return 成功转换的资产数量
	 */
	static int32 NativeConvertPackageNamesToAssetData(
		const TArray<FString>& PackageNames,
		TArray<FAssetData>& OutAssetDatas
	);

	/**
	 * 处理遗留引用列表
	 * @param ReferencesList 需要处理的引用路径列表
	 * @param ListPath 引用列表文件路径
	 * @return 元组包含：
	 *         - bool: 是否成功处理
	 *         - int32: 处理的资产数量
	 *         - bool: 是否成功删除文件
	 */
	static TTuple<bool, int32, bool> NativeProcessLegacyReferences(
		const TArray<FString>& ReferencesList,
		const FString& ListPath
	);

	/**
	 * 将AssetData的路径转化为FString
	 * @param DataToBeProcessed 
	 * @return 
	 */
	static TArray<FString> NativeConvertAssetDataToStringPaths(const TArray<FAssetData>& DataToBeProcessed);

	/**
	 * 将资产标记为脏数据并触发一次保存
	 * @param DataToBeProcessed 需要MarkDirty的资产列表
	 */
	static void NativeActiveTriggerMarkDirty(const TArray<FAssetData>& DataToBeProcessed);

	/**
	 * 给Commandlet用的自定义修复，无Slate模块占用
	 * @param Objects 
	 * @param bCheckoutDialogPrompt 
	 */
	static void NativeFixupReferencers(const TArray<UObjectRedirector*>& Objects);

	/**
	 * AssetTool中迁移出来的源码，执行修复用，自动删除修复过程中查找到的无效重定向器
	 * @param Objects 
	 */
	static void ExecuteFixUp(TArray<TWeakObjectPtr<UObjectRedirector>> Objects);
	
	static void RenameReferencingSoftObjectPaths(const TArray<UPackage*> PackagesToCheck, const TMap<FSoftObjectPath, FSoftObjectPath>& AssetRedirectorMap);

	/**
	 * 强制解锁占用,慎用，可以解除，但是需要深入引擎卸载资产，不然会独占导致文件无法保存
	 * @param Package Package信息
	 */
	static void NativeForceUnlockPackage(UPackage* Package);
	
	static bool SafeSavePackage(UPackage* Pkg);

	/**
	 * 文件是否被占用
	 * @param FileName 
	 * @return 
	 */
	static bool NativeIsFileLocked(const FString& FileName);

	/**
	 * 获取默认路径
	 * @return 返回默认路径为Save/AssetsTool/AListOfReferencesToBeProcessed.text
	 */
	static FString NativeGetDefaultAssetListPath();
	

	static FString ConvertGamePathToPhysical(const FString& GamePath);
};
