// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilites/WarriorEnemyGameplayAbility.h"

#include "WarriorGameplayTags.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "Characters/WarriorEnemyCharacter.h"

AWarriorEnemyCharacter* UWarriorEnemyGameplayAbility::GetEnemyCharacterFromActorInfo()
{
	if (!CachedWarriorEnemyCharacter.IsValid())
	{
		CachedWarriorEnemyCharacter = Cast<AWarriorEnemyCharacter>(CurrentActorInfo->AvatarActor);
	}

	return CachedWarriorEnemyCharacter.IsValid() ? CachedWarriorEnemyCharacter.Get() : nullptr;
}

UEnemyCombatComponent* UWarriorEnemyGameplayAbility::GetEnemyCombatComponentFromActorInfo()
{
	return GetEnemyCharacterFromActorInfo()->GetEnemyCombatComponent();
}

FGameplayEffectSpecHandle UWarriorEnemyGameplayAbility::MakeEnemyDamageEffectSpecHandle(
	TSubclassOf<UGameplayEffect> EffectClass, const FScalableFloat& InDamageScalableFloat)
{
	check(EffectClass);
	
	// 指定上下文信息
	FGameplayEffectContextHandle ContextHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	ContextHandle.SetAbility(this);
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
	
	// 构建
	FGameplayEffectSpecHandle EffectSpecHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
		EffectClass,
		GetAbilityLevel(),
		ContextHandle
	);
	
	EffectSpecHandle.Data->SetSetByCallerMagnitude(
		WarriorGameplayTags::Shared_SetByCaller_BaseDamage,
		InDamageScalableFloat.GetValueAtLevel(GetAbilityLevel())
	);

	return EffectSpecHandle;
}
