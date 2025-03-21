
#include "..\Public\AssetsToolModule.h"

#include "AssetsToolActions.h"
#include "Misc/FileHelper.h"
UE_DISABLE_OPTIMIZATION
void FAssetsToolModule::StartupModule()
{
	// 注册入口
	AssetActions = NewObject<UAssetsToolActions>();
	AssetActions->InitContentBrowserExtend();
}
void FAssetsToolModule::ShutdownModule()  
{ 
}
UE_ENABLE_OPTIMIZATION
