// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilites/WarriorGameplayAbility.h"
#include "WarriorHeroGameplayAbility.generated.h"

class UHeroCombatComponent;
class AWarriorHeroController;
class AWarriorHeroCharacter;
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorHeroGameplayAbility : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * 通过ActorInfo获取英雄角色
	 * @return 英雄角色
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Warrior|Ability")
	AWarriorHeroCharacter* GetHeroCharacterFromActorInfo();

	/**
	 * 通过ActorInfo获取英雄控制器
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Warrior|Ability")
	AWarriorHeroController* GetHeroControllerFromActionInfo();

	/**
	 * 通过ActorInfo获取英雄战斗组件
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Warrior|Ability")
	UHeroCombatComponent* GetHeroCombatComponentFromActionInfo();

	/**
	 * 创建Hero伤害效果句柄
	 * @param EffectClass GEClass
	 * @param InWeaponBaseDamage 武器基础伤害
	 * @param InCurrentAttackTypeTag 攻击类型Tag
	 * @param InUsedComboCount 攻击计数
	 * @return 
	 */
	UFUNCTION(BlueprintPure, Category = "Warrior|Ability")
	FGameplayEffectSpecHandle MakeHeroDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, float InWeaponBaseDamage, FGameplayTag InCurrentAttackTypeTag, int32 InUsedComboCount);

private:
	// WeakObjectPtr,弱指针，不会增加持有对象的引用计数
	// 缓存英雄角色
	TWeakObjectPtr<AWarriorHeroCharacter> CachedWarriorHeroCharacter;

	// 缓存英雄控制器
	TWeakObjectPtr<AWarriorHeroController> CachedWarriorHeroController;
};
