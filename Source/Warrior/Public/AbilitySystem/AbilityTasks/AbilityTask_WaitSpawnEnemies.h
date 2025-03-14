// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitSpawnEnemies.generated.h"

class AWarriorEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitSpawnEnemiesDelegate, const TArray<AWarriorEnemyCharacter*>&, SpawnedEnemies);

/**
 * 等待创建敌人（召唤）
 */
UCLASS()
class WARRIOR_API UAbilityTask_WaitSpawnEnemies : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Warrior|AbilityTasks", meta = (DisplayName = "Wait Gameplay Event And Spawn Enemies", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", NumToSpawn = "1", RandomSpawnRadius = "200"))
	static UAbilityTask_WaitSpawnEnemies* WaitSpawnEnemies(UGameplayAbility* OwningAbility,	FGameplayTag EventTag, TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClassToSpawn,
		int32 NumToSpawn, const FVector& SpawnOrigin, float RandomSpawnRadius);

	UPROPERTY(BlueprintAssignable)
	FWaitSpawnEnemiesDelegate OnSpawnFinished;

	UPROPERTY(BlueprintAssignable)
	FWaitSpawnEnemiesDelegate DidNotSpawn;

	//~ Begin UGameplayTask Interface
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	//~ End UGameplayTask Interface

private:
	// 事件Tag
	FGameplayTag CachedEventTag;
	// 怪物类型
	TSoftClassPtr<AWarriorEnemyCharacter> CachedSoftEnemyClassToSpawn;
	// 生成数量
	int32 CachedNumToSpawn;
	// 创建位置
	FVector CachedSpawnOrigin;
	// 创建半径
	float CachedRandomSpawnRadius;
	// 事件Handle
	FDelegateHandle DelegateHandle;

	// 接收到事件的回调
	void OnGameplayEventReceived(const FGameplayEventData* InPayload);

	// 敌人加载成功
	void OnEnemyClassLoaded();
};
