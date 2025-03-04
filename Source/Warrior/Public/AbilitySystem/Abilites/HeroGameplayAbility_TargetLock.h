// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilites/WarriorHeroGameplayAbility.h"
#include "HeroGameplayAbility_TargetLock.generated.h"

class UWarriorWidgetBase;
/**
 * 
 */
UCLASS()
class WARRIOR_API UHeroGameplayAbility_TargetLock : public UWarriorHeroGameplayAbility
{
	GENERATED_BODY()

protected:
	// ~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	// ~ End UGameplayAbility Interface

	/**
	 * 通过AbilityTask实现Tick
	 * @param DeltaTime 
	 */
	UFUNCTION(BlueprintCallable)
	void OnTargetLockTick(float DeltaTime);

private:
	/**
	 * 尝试锁定目标
	 */
	void TryLockOnTarget();

	/**
	 * 获取可用锁定的Actors
	 */
	void GetAvailableActorsToLock();

	/**
	 * 获取距离最近的Actor
	 * @param InAvailableActors 
	 * @return 
	 */
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);

	/**
	 * 绘制锁敌UI
	 */
	void DrawTargetLockWidget();

	/**
	 * 设置Widget位置
	 */
	void SetTargetLockWidgetPosition();

	/**
	 * 取消锁定能力
	 */
	void CancelTargetLockAbility();

	/**
	 * 清理
	 */
	void CleanUp();

	UPROPERTY(EditDefaultsOnly, Category = "Trace Lock")
	float TraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TArray<TEnumAsByte<EObjectTypeQuery>> BoxTraceChannel;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	bool bShowPersistentDebugShape;
	
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TSubclassOf<UWarriorWidgetBase> TargetLockWidgetClass;

	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;
	
	UPROPERTY()
	AActor* CurrentLockedActor;

	UPROPERTY()
	UWarriorWidgetBase* DrawTargetLockWidgetPtr;

	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;
};


