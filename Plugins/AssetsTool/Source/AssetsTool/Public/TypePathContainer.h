#pragma once
#include "TypePathContainer.generated.h"


UCLASS()
class UTypePathContainer : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<UClass*, FString> TypePaths;
};

USTRUCT()
struct FTypePathData
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UTypePathContainer> TypePathsContainer;

	UPROPERTY()
	TMap<UClass*, FString> TypeChinese;

	// 数据访问接口
	TMap<UClass*, FString>& GetTypePaths() const
	{
		if(TypePathsContainer.IsValid())
		{
			return TypePathsContainer->TypePaths;
		}
		static TMap<UClass*, FString> EmptyMap;
		return EmptyMap;
	}
};