// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Component/PawnExtensionComponentBase.h"
#include "PawnCombatComponent.generated.h"

class AWarriorWeaponBase;

UENUM(BlueprintType)
enum class EToggleDamageType : uint8
{
	CurrentEquippedWeapon UMETA(DisplayName = "当前装备武器"),
	LeftHand UMETA(DisplayName = "左手"),
	RightHand UMETA(DisplayName = "右手")
};

/**
 * 
 */
UCLASS()
class WARRIOR_API UPawnCombatComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	/**
	 * 当前正在装备的武器Tag
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Warrior|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	/**
	 * 注册武器
	 * @param InWeaponTagToRegister 可注册的武器Tag
	 * @param InWeaponToRegister 注册武器
	 * @param bRegisterAsEquippedWeapon 是否立即装备
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister, AWarriorWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon = false);

	/**
	 * 根据Tag获取当前携带的武器
	 * @param InWeaponTagToGet 武器Tag
	 * @return 武器
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AWarriorWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	/**
	 * 获取当前装备的武器
	 * @return 武器
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AWarriorWeaponBase* GetCharacterCurrentEquippedWeapon() const;

	/**
	 * 触发武器碰撞
	 * @param bShouldEnable 
	 * @param ToggleDamage 
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	void ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType = EToggleDamageType::CurrentEquippedWeapon);

	/**
	 * 命中Actor
	 * @param HitActor 命中Actor
	 */
	virtual void OnHitTargetActor(AActor* HitActor);

	/**
	 * 武器脱离命中目标
	 * @param InteractedActor 
	 */
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor);

protected:
	// 重叠的Actor
	UPROPERTY()
	TArray<AActor*> OverlappedActors;
private:
	// 角色携带的武器
	TMap<FGameplayTag, AWarriorWeaponBase*> CharacterCarriedWeaponMap;
};
