#pragma once
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"


class UAssetsToolActions;
/**
 * Implements the Editor_Jy_Validation module.
 */
class FAssetsToolModule
	: public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	
private:
	UPROPERTY()
	UAssetsToolActions* AssetActions;
};


IMPLEMENT_MODULE(FAssetsToolModule, AssetsTool);
