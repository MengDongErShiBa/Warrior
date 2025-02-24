// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/WarriorCharacterBaseAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Characters/WarriorBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


void UWarriorCharacterBaseAnimInstance::NativeInitializeAnimation()
{
	// 获取拥有者
	OwningCharacter = Cast<AWarriorBaseCharacter>(TryGetPawnOwner());

	if (OwningCharacter)
	{
		OwningMovementComponent = OwningCharacter->GetCharacterMovement();
	}
}

void UWarriorCharacterBaseAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningCharacter || !OwningMovementComponent)
	{
		return;
	}

	GroundSpeed = OwningCharacter->GetVelocity().Size2D();

	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;

	// 计算运动方向
	LocomotionDirection = UKismetAnimationLibrary::CalculateDirection(OwningCharacter->GetVelocity(), OwningCharacter->GetActorRotation());
}
