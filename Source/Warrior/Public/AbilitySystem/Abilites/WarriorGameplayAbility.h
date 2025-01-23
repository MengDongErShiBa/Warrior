// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WarriorGameplayAbility.generated.h"

// 激活策略
UENUM(BlueprintType)
enum class EWarriorAbilityActivationPolicy : uint8
{
	OnTrigger UMETA(DisplayName = "触发"),
	OnGiven UMETA(DisplayName = "立即执行")
};

/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

protected:

	//~ Begin UGameplayAbility Interface
	// 给予能力，类似BeginPlayer的时机
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	// 能力结束时
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface

	UPROPERTY(EditDefaultsOnly, Category = "WarriorAbility")
	EWarriorAbilityActivationPolicy AbilityActivationPolicy = EWarriorAbilityActivationPolicy::OnTrigger;
};
