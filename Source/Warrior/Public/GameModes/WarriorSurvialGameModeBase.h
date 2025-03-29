// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/WarriorGameModeBase.h"
#include "WarriorSurvialGameModeBase.generated.h"

UENUM(Blueprintable)
enum class EWarriorSurvialGameModeState : uint8
{
	WaitSpawnNewWave UMETA(DisplayName = "等待新波次"),
	SpawningNewWave UMETA(DisplayName = "产生新波次"),
	InProgress UMETA(DisplayName = "进行中"),
	WaveCompleted UMETA(DisplayName = "波次完成"),
	AllWavesDone UMETA(DisplayName = "所有波次完成"),
	PlayerDied UMETA(DisplayName = "玩家死亡")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvialGameModeStateChanged, EWarriorSurvialGameModeState, CurrentState);

/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorSurvialGameModeBase : public AWarriorGameModeBase
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:

	void SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState InModeState);
	
	UPROPERTY()
	EWarriorSurvialGameModeState CurrentSurvialGameModeState;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSurvialGameModeStateChanged OnSurvialGameModeStateChanged;
};
