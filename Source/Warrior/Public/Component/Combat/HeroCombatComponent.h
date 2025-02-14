// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/Combat/PawnCombatComponent.h"
#include "HeroCombatComponent.generated.h"

class AWarriorHeroWeapon;
/**
 * 
 */
UCLASS()
class WARRIOR_API UHeroCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	// 获取Hero当前携带的武器
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AWarriorHeroWeapon* GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const;
	/**
	 * 命中Actor
	 * @param HitActor 命中Actor
	 */
	virtual void OnHitTargetActor(AActor* HitActor) override;

	/**
	 * 武器脱离命中目标
	 * @param InteractedActor 
	 */
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor) override;
};
