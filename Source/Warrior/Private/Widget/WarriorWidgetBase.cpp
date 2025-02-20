// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/WarriorWidgetBase.h"

#include "Component/UI/EnemyUIComponent.h"
#include "Component/UI/HeroUIComponent.h"
#include "Interfaces/PawnUIInterface.h"

void UWarriorWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IPawnUIInterface* PawnUIInterface = Cast<IPawnUIInterface>(GetOwningPlayerPawn()))
	{
		if (UHeroUIComponent* HeroUIComponent = PawnUIInterface->GetHeroUIComponent())
		{
			BP_OnOwningHeroUIComponentInitialized(HeroUIComponent);
		}
	}

}

void UWarriorWidgetBase::InitEnemyCreateWidget(AActor* OwningEnemyActor)
{
	if (IPawnUIInterface* PawnUIInterface = Cast<IPawnUIInterface>(OwningEnemyActor))
	{
		UEnemyUIComponent* EnemyUIComponent = PawnUIInterface->GetEnemyUIComponent();

		checkf(EnemyUIComponent, TEXT("Faild to extrac an EnemyUIComponent from %s"), *OwningEnemyActor->GetActorNameOrLabel())

		BP_OnOwningEnemyUIComponentInitialized(EnemyUIComponent);
	}
}
