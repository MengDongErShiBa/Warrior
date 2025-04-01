// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WarriorTypes/WarriorEnumType.h"
#include "WarriorGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWarriorGameModeBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameSettings")
	EWarriorGameDifficulty CurrentGameDifficulty;

public:
	FORCEINLINE EWarriorGameDifficulty GetCurrentGameDifficulty() const { return CurrentGameDifficulty; }
};
