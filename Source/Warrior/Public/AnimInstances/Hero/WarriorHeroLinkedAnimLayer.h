// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstances/WarriorBaseAnimInstance.h"
#include "WarriorHeroLinkedAnimLayer.generated.h"

class UWarriorHeroAnimInstance;
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorHeroLinkedAnimLayer : public UWarriorBaseAnimInstance
{
	GENERATED_BODY()

public:
	/**
	 * 纯函数，蓝图线程安全
	 * 获取Hero动画实例
	 * @return 
	 */
	UFUNCTION(BlueprintPure, meta=(BluerpintThreadSafe))
	UWarriorHeroAnimInstance* GetHeroAnimInstance() const;
};
