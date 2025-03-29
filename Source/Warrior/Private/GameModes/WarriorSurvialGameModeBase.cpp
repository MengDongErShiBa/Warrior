// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/WarriorSurvialGameModeBase.h"

void AWarriorSurvialGameModeBase::BeginPlay()
{
	Super::BeginPlay();
}

void AWarriorSurvialGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AWarriorSurvialGameModeBase::SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState InModeState)
{
	CurrentSurvialGameModeState = InModeState;

	OnSurvialGameModeStateChanged.Broadcast(InModeState);
}
