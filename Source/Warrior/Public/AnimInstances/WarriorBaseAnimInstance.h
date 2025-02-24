// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstance.h"
#include "WarriorBaseAnimInstance.generated.h"

class UCharacterMovementComponent;
class AWarriorBaseCharacter;
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	/**
	 * 辅助函数：检查是否拥有某个标签
	 * @return 
	 */
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	bool DoesOwnerHaveTag(FGameplayTag TagToCheck) const;
};
