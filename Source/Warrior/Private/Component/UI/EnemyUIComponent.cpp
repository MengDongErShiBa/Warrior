// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/UI/EnemyUIComponent.h"

#include "Widget/WarriorWidgetBase.h"

void UEnemyUIComponent::RegisterEnemyDrawnWidget(UWarriorWidgetBase* InWidgetToRegister)
{
	EnemyDrawnWidgets.Add(InWidgetToRegister);
}

void UEnemyUIComponent::RemoveEnemyDrawnWidgetsIfAny()
{
	if (EnemyDrawnWidgets.IsEmpty())
	{
		return;
	}

	for (UWarriorWidgetBase* EnemyDrawnWidget : EnemyDrawnWidgets)
	{
		if (EnemyDrawnWidget)
		{
			EnemyDrawnWidget->RemoveFromParent();
		}
	}
}
