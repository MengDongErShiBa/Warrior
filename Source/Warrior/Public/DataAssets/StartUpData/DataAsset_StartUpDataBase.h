// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataAsset_StartUpDataBase.generated.h"

class UWarriorAbilitySystemComponent;
class UWarriorGameplayAbility;
/**
 * 
 */
UCLASS()
class WARRIOR_API UDataAsset_StartUpDataBase : public UDataAsset
{
	GENERATED_BODY()

public:
	// 将ability交给ASC
	virtual void GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);

protected:
	// 需要立即被激活的能力
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray<TSubclassOf<UWarriorGameplayAbility>> ActivateOnGivenAbilities;

	// 需要触发的
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray<TSubclassOf<UWarriorGameplayAbility>> ReactiveAbilities;

	// 给予能力
	void GrantAbilities(const TArray<TSubclassOf<UWarriorGameplayAbility>>& InAbilitiesToGive, UWarriorAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);
};
