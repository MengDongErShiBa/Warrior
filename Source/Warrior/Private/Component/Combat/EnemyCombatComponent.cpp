// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Combat/EnemyCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "WarriorBlueprintFunctionLibrary.h"
#include "WarriorDebugHelper.h"
#include "WarriorGameplayTags.h"

void UEnemyCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor))
	{
		return;
	}

	// 在禁用武器碰撞部分，做出了清理操作
	OverlappedActors.AddUnique(HitActor);

	// TODO: Implement block check 执行阻挡检查
	// 是否有效阻挡
	bool bIsValidBlock = false;

	// 玩家是否正在阻挡
	const bool bIsPlayerBlacking = UWarriorBlueprintFunctionLibrary::NativeDoesActorHaveTag(HitActor, WarriorGameplayTags::Player_Status_Blocking);
	// 攻击是否为不可阻挡
	const bool bIsMyAttackUnBlockAble = false;

	if (bIsPlayerBlacking && !bIsMyAttackUnBlockAble)
	{
		// TODO: check if the block is valid
		bIsValidBlock = UWarriorBlueprintFunctionLibrary::IsValidBlock(GetOwningPawn(), HitActor);
	}

	FGameplayEventData EventData;
	EventData.Instigator = GetOwningPawn();
	EventData.Target = HitActor;

	if (bIsValidBlock)
	{
		// 有效阻挡
	}
	else
	{
		// 应用伤害
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			GetOwningPawn(),
			WarriorGameplayTags::Shared_Event_MeleeHit,
			EventData
		);
	}
	
}
