// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/WarriorBaseCharacter.h"

#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "AbilitySystem/WarriorAttributeSet.h"

// Sets default values
AWarriorBaseCharacter::AWarriorBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	// 开始游戏时不器用Tick
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Mesh不接受贴花
	GetMesh()->bReceivesDecals = false;

	WarriorAbilitySystemComponent = CreateDefaultSubobject<UWarriorAbilitySystemComponent>(TEXT("WarriorAbilitySystemComponent"));
	WarriorAttributeSet = CreateDefaultSubobject<UWarriorAttributeSet>(TEXT("WarriorAttributeSet"));
	
}

UAbilitySystemComponent* AWarriorBaseCharacter::GetAbilitySystemComponent() const
{
	return GetWarriorAbilitySystemComponent();
}

void AWarriorBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (WarriorAbilitySystemComponent)
	{
		WarriorAbilitySystemComponent->InitAbilityActorInfo(this, this);

		// ensure(!CharacterStartUpData.IsNull());
		ensureMsgf(!CharacterStartUpData.IsNull(),TEXT("Forgot to assign start up data to %s"), *GetName());
	}
}
