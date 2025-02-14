// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WarriorTypes/WarriorEnumType.h"
#include "WarriorGameplayAbility.generated.h"

class UWarriorAbilitySystemComponent;
class UPawnCombatComponent;
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

	/**
	 * 通过Actor信息获取Pawn战斗组件
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability")
	UPawnCombatComponent* GetPawnCombatComponentFromActorInfo() const;

	/**
	 * 通过Actor信息获取ASC
	 * @return ASC
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability")
	UWarriorAbilitySystemComponent* GetWarriorAbilitySystemComponentFromActorInfo() const;
	
	// 能力激活策略
	UPROPERTY(EditDefaultsOnly, Category = "WarriorAbility")
	EWarriorAbilityActivationPolicy AbilityActivationPolicy = EWarriorAbilityActivationPolicy::OnTrigger;

	/**
	 * 应用GE到目标
	 * @param TargetActor 
	 * @param InSpecHandle 
	 * @return 
	 */
	FActiveGameplayEffectHandle NativeApplyEffectSpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle);

	/**
	 * 应用GE到目标
	 * @param TargetActor 目标Actor
	 * @param InSpecHandle GESpecHandle
	 * @param OutSuccessType 是否成功
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability", meta = (DisplayName = "Apply Gameplay Effect Spec Handle To Target Actor", ExpandEnumAsExecs ="OutSuccessType"))
	FActiveGameplayEffectHandle BP_ApplyEffectSpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle, EWarriorSuccessType& OutSuccessType);
};
