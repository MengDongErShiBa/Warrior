// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/WarriorEnemyCharacter.h"

#include "Component/Combat/EnemyCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AWarriorEnemyCharacter::AWarriorEnemyCharacter()
{
	// 何时由AIController接管
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	// 不使用控制器旋转
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	// 使用移动方向来控制旋转
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// 旋转速率
	GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);
	// 最大行走速度
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;

	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>("EnemyCombatComponent");
}
