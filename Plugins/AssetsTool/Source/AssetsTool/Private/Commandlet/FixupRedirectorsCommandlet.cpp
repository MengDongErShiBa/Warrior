#include "Commandlet/FixupRedirectorsCommandlet.h"

#include "AssetsToolCheckActions.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "StandaloneRenderer.h"
#define LOCTEXT_NAMESPACE "UFixupRedirectorCommandlet"
int32 UFixupRedirectorCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Log, TEXT("[UFixupRedirectorCommandlet::Main] 准备修复引用列表重定向器"));
	
	FString AssetListPath;
	if (!FParse::Value(*Params, TEXT("-AssetList="), AssetListPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Missing -AssetList parameter"));
		return 1;
	}

	// 加载资产列表
	TArray<FString> AssetPaths;
	if (!FFileHelper::LoadFileToStringArray(AssetPaths, *AssetListPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load asset list: %s"), *AssetListPath);
		return 2;
	}

	// 转化为FAssetData列表
	TArray<FAssetData> AssetsToFix;
	UAssetsToolCheckActions::NativeConvertPackageNamesToAssetData(AssetPaths, AssetsToFix);

	for (FAssetData ToFix : AssetsToFix)
	{
		UE_LOG(LogTemp, Log, TEXT("找到资产%s/%s"), *ToFix.PackagePath.ToString(), *ToFix.AssetName.ToString());
	}

	// 新进程执行，不需要显示进度，内部已经做了宏处理Slate相关提示
	// 这部分要扣修复引用的代码出来，代码中的修复报告全部默认选择，不弹窗，否则会有必现崩溃
	UAssetsToolCheckActions::NativeCommandletFixupAssetReferences(AssetsToFix);

	UE_LOG(LogTemp, Log, TEXT("修复重定向完成"));

	// 处理完成后直接删除本地文件
	IFileManager::Get().Delete(*AssetListPath);

	return 0;
}

#undef LOCTEXT_NAMESPACE
