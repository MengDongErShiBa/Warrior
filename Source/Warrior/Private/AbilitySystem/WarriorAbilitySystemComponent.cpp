// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/WarriorAbilitySystemComponent.h"

void UWarriorAbilitySystemComponent::OnAbilityInputPressed(const FGameplayTag& InInputTag)
{
	if (!InInputTag.IsValid()) return;

	// 获取所有激活的能力
	// GetActivatableAbilities()
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		// 精确匹配
		if (!AbilitySpec.DynamicAbilityTags.HasTagExact(InInputTag)) continue;

		// 激活Ability
		TryActivateAbility(AbilitySpec.Handle);
	}
}

void UWarriorAbilitySystemComponent::OnAbilityInputReleased(const FGameplayTag& InInputTag)
{
	
}
