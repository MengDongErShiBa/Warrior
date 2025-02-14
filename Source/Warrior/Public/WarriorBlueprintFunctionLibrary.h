// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WarroprTypes/WarriorEnumType.h"
#include "WarriorBlueprintFunctionLibrary.generated.h"

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
	static void RemoveGameplayFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove);

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
};
