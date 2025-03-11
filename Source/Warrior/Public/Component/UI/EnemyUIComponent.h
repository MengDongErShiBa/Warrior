// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/UI/PawnUIComponent.h"
#include "EnemyUIComponent.generated.h"

class UWarriorWidgetBase;
/**
 * 
 */
UCLASS()
class WARRIOR_API UEnemyUIComponent : public UPawnUIComponent
{
	GENERATED_BODY()

public:
	/**
	 * 注册小部件
	 * @param InWidgetToRegister 
	 */
	UFUNCTION(BlueprintCallable)
	void RegisterEnemyDrawnWidget(UWarriorWidgetBase* InWidgetToRegister);

	/**
	 * 删除UI
	 */
	UFUNCTION(BlueprintCallable)
	void RemoveEnemyDrawnWidgetsIfAny();

private:
	TArray<UWarriorWidgetBase*> EnemyDrawnWidgets;
};
