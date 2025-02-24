// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/WarriorBaseAnimInstance.h"

#include "WarriorBlueprintFunctionLibrary.h"
#include "Characters/WarriorBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UWarriorBaseAnimInstance::DoesOwnerHaveTag(FGameplayTag TagToCheck) const
{
	if (APawn* OwningPawn = TryGetPawnOwner())
	{
		return UWarriorBlueprintFunctionLibrary::NativeDoesActorHaveTag(OwningPawn, TagToCheck);
	}

	return false;
}
