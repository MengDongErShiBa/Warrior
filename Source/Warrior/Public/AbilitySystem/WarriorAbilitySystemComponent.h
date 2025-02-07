// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WarroprTypes/WarriorStructTypes.h"
#include "WarriorAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
	// Ability输入按下
	void OnAbilityInputPressed(const FGameplayTag& InInputTag);

	// Ability输入抬起
	void OnAbilityInputReleased(const FGameplayTag& InInputTag);

	/**
	 * 给予武器能力
	 * @param InDefaultWeaponAbilities 武器能力集合
	 * @param ApplyLevel 需要应用的等级
	 * @param OutGrantedAbilitySpecHandles AbilitySpecHandles
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability", meta = (ApplyLevel = "1"))
	void GrantHeroWeaponAbilities(const TArray<FWarriorHeroAbilitySet>& InDefaultWeaponAbilities, int32 ApplyLevel, TArray<FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles);

	/**
	 * 移除已经给予的武器能力
	 * @param InSpecHandlesToRemove 
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability")
	void RemovedGrantedHeroWeaponAbilities(UPARAM(ref) TArray<FGameplayAbilitySpecHandle>& InSpecHandlesToRemove);
};