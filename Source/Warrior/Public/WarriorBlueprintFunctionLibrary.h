// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WarriorTypes/WarriorStructTypes.h"
#include "WarriorBlueprintFunctionLibrary.generated.h"

struct FGameplayEffectSpecHandle;
class UPawnCombatComponent;
struct FGameplayTag;
class UWarriorAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 获取ASC组件
	 * @param InActor 
	 * @return 
	 */
	static UWarriorAbilitySystemComponent* NativeGetWarriorASCFromActor(AActor* InActor);

	/**
	 * 添加Tag，如果Tag不存在
	 * @param InActor 
	 * @param TagToAdd 
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static void AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd);

	/**
	 * 如果可以找到Tag则删除Actor对应的Tag
	 * @param InActor 
	 * @param TagToRemove  
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static void RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove);

	/**
	 * 是否拥有标签
	 * @param InActor 
	 * @param TagToCheck 
	 * @return 
	 */
	static bool NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck);

	/**
	 * 是否包含某个标签
	 * @param InActor 
	 * @param TagToCheck 
	 * @param OutConfirmType 返回类型 
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary", meta = (DisplayName = "Does Actor Have Tag", ExpandEnumAsExecs = "OutConfirmType"))
	static void BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EWarriorConfirmType& OutConfirmType);

	/**
	 * 获取战斗组件
	 * @param InActor 
	 * @return 
	 */
	static UPawnCombatComponent* NativeGetPawnCombatComponentFromActor(AActor* InActor);
	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary", meta = (DisplayName = "Get Pawn Combat Component From Actor",  ExpandEnumAsExecs = "OutValidType"))
	static UPawnCombatComponent* BP_GetPawnCombatComponentFromActor(AActor* InActor, EWarriorValidType& OutValidType);

	/**
	 * 目标是否为敌人
	 * @param QueryPawn 查询Pawn
	 * @param TargetPawn 目标Pawn
	 * @return 
	 */
	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static bool IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn);

	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary", meta = (CompactNodeTitle = "Get Value At Level"))
	static float GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel = 1.f);

	/**
	 * 受击方向计算
	 * @param InAttacker 攻击者 
	 * @param InVictim 防御者
	 * @param OutAngleDifference 输出方向 
	 * @return 
	 */
	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static FGameplayTag ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim, float& OutAngleDifference);

	/**
	 * 是否为有效阻挡
	 * @param InAttack 攻击者 
	 * @param InDefender 防御者
	 * @return 
	 */
	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static bool IsValidBlock(AActor* InAttack, AActor* InDefender);

	/**
	 * 将游戏效果规范句柄应用于目标演员
	 * @param InInstigator 
	 * @param InTargetActor 
	 * @param InSpecHandle 
	 * @return 
	 */
	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static bool ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor, const FGameplayEffectSpecHandle& InSpecHandle);
};
