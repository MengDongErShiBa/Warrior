// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/AbilityTask_WaitSpawnEnemies.h"

#include "AbilitySystemComponent.h"
#include "NavigationSystem.h"
#include "WarriorDebugHelper.h"
#include "Characters/WarriorEnemyCharacter.h"
#include "Engine/AssetManager.h"

UAbilityTask_WaitSpawnEnemies* UAbilityTask_WaitSpawnEnemies::WaitSpawnEnemies(UGameplayAbility* OwningAbility,
                                                                               FGameplayTag EventTag, TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClassToSpawn, int32 NumToSpawn,
                                                                               const FVector& SpawnOrigin, float RandomSpawnRadius)
{
	UAbilityTask_WaitSpawnEnemies* Node = NewAbilityTask<UAbilityTask_WaitSpawnEnemies>(OwningAbility);
	Node->CachedEventTag = EventTag;
	Node->CachedSoftEnemyClassToSpawn = SoftEnemyClassToSpawn;
	Node->CachedNumToSpawn = NumToSpawn;
	Node->CachedSpawnOrigin = SpawnOrigin;
	Node->CachedRandomSpawnRadius = RandomSpawnRadius;

	return Node;
}

void UAbilityTask_WaitSpawnEnemies::Activate()
{
	// 如果监听到这个Tag的事件，触发回调
	FGameplayEventMulticastDelegate& Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(CachedEventTag);
	DelegateHandle = Delegate.AddUObject(this, &ThisClass::OnGameplayEventReceived);
}

void UAbilityTask_WaitSpawnEnemies::OnDestroy(bool bInOwnerFinished)
{
	FGameplayEventMulticastDelegate& Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(CachedEventTag);
	Delegate.Remove(DelegateHandle);

	Super::OnDestroy(bInOwnerFinished);
}

void UAbilityTask_WaitSpawnEnemies::OnGameplayEventReceived(const FGameplayEventData* InPayload)
{
	if (ensure(!CachedSoftEnemyClassToSpawn.IsNull()))
	{
		// 异步加载
		UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
			CachedSoftEnemyClassToSpawn.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &ThisClass::OnEnemyClassLoaded)
			);
	}
	else
	{
		// 使用这个可以保证能力在激活状态，还需要查查是不是
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			// 并未生成敌人
			DidNotSpawn.Broadcast(TArray<AWarriorEnemyCharacter*>());
		}

		// 结束任务
		EndTask();
	}
}

void UAbilityTask_WaitSpawnEnemies::OnEnemyClassLoaded()
{
	UClass* LoadedClass = CachedSoftEnemyClassToSpawn.Get();
	UWorld* World = GetWorld();

	// 确保有效
	if (!LoadedClass || !World)
	{
		// 使用这个可以保证能力在激活状态，还需要查查是不是
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			// 并未生成敌人
			DidNotSpawn.Broadcast(TArray<AWarriorEnemyCharacter*>());
		}
		// 结束任务
		EndTask();
		return;
	}

	TArray<AWarriorEnemyCharacter*> SpawnedEnemies;

	// 生成Actor参数
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	for (int32 i = 0; i < CachedNumToSpawn; i++)
	{
		FVector RandomLocation;
		// 通过导航网格体 查询随机点位
		UNavigationSystemV1::K2_GetRandomReachablePointInRadius(this, CachedSpawnOrigin, RandomLocation, CachedRandomSpawnRadius);

		// 抬高Z轴， 如果不做这一步可能会导致敌人卡在地面
		RandomLocation += FVector(0.f, 0.f, 150.f);

		// 旋转不对，因为是生成前的旋转值
		const FRotator SpawnFacingRotation = AbilitySystemComponent->GetAvatarActor()->GetActorForwardVector().ToOrientationRotator();

		// 生成Actor
		AWarriorEnemyCharacter* SpawnedEnemy = World->SpawnActor<AWarriorEnemyCharacter>(LoadedClass, RandomLocation, SpawnFacingRotation, SpawnParameters);
		if (SpawnedEnemy)
		{
			SpawnedEnemies.Add(SpawnedEnemy);
		}
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (!SpawnedEnemies.IsEmpty())
		{
			// 通知生成完毕
			OnSpawnFinished.Broadcast(SpawnedEnemies);
		}
		else
		{
			DidNotSpawn.Broadcast(TArray<AWarriorEnemyCharacter*>());
		}
	}

	// 结束任务
	EndTask();
}
