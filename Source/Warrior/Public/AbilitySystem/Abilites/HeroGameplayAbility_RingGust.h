// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilites/WarriorHeroGameplayAbility.h"
#include "HeroGameplayAbility_RingGust.generated.h"


/**
 * 
 */
UCLASS()
class WARRIOR_API UHeroGameplayAbility_RingGust : public UWarriorHeroGameplayAbility
{
	GENERATED_BODY()

	UHeroGameplayAbility_RingGust();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnChargeComplete(float TimeHeld);

	void SpawnGustRing(float Radius, float Damage, bool bPlayFullVFX);
private:

	TSubclassOf<AActor> RingActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "风环")
	float MaxChargeTime = 1.5f;

	FTimerHandle ChargeTimer;
	float CurrentChargeTime;
};
