#pragma once
#include "Commandlets/Commandlet.h"

#include "FixupRedirectorsCommandlet.generated.h"

/**
 * 修复引用列表命令
 */
UCLASS()
class UFixupRedirectorCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
};
