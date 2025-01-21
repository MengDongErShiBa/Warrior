// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/WarriorBaseCharacter.h"

// Sets default values
AWarriorBaseCharacter::AWarriorBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	// 开始游戏时不器用Tick
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Mesh不接受贴花
	GetMesh()->bReceivesDecals = false;
}
