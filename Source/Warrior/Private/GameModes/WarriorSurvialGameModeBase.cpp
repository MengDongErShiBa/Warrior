// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/WarriorSurvialGameModeBase.h"

void AWarriorSurvialGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	checkf(EnemyWaveSpawnerDataTable, TEXT("Forgot to assign a valid data table in survial game mode blueprint"));

	SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState::WaitSpawnNewWave);

	// 得到生成总波次
	TotalWavesToSpawn = EnemyWaveSpawnerDataTable->GetRowNames().Num();
}

void AWarriorSurvialGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::WaitSpawnNewWave)
	{
		// 生成新波次
		TimePassedSinceStart += DeltaSeconds;

		if (TimePassedSinceStart >= SpawnNewWaveWaitTime)
		{
			TimePassedSinceStart = 0.f;

			// 进入生成状态
			SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState::SpawningNewWave);
		}
	}

	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::SpawningNewWave)
	{
		// 新波次生成中
		TimePassedSinceStart += DeltaSeconds;

		if (TimePassedSinceStart >= SpawnEnemiesDelayTime)
		{
			// TODO: Handle spawn new enemies

			TimePassedSinceStart = 0.f;
			SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState::InProgress);
		}
	}

	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::WaveCompleted)
	{
		TimePassedSinceStart += DeltaSeconds;

		if (TimePassedSinceStart >= WaveCompletedWaitTime)
		{
			TimePassedSinceStart = 0.f;

			// 增加当前波次
			CurrentWaveCount++;

			if (HasFinishedAllWaves())
			{
				// 所有波次已经处理完毕
				SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState::AllWavesDone);
			}
			else
			{
				// 继续等待生成新波次
				SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState::WaitSpawnNewWave);
			}
		}
	}
}

void AWarriorSurvialGameModeBase::SetCurrentSurvialGameModeState(EWarriorSurvialGameModeState InModeState)
{
	CurrentSurvialGameModeState = InModeState;

	OnSurvialGameModeStateChanged.Broadcast(InModeState);
}

bool AWarriorSurvialGameModeBase::HasFinishedAllWaves() const
{
	return CurrentWaveCount > TotalWavesToSpawn;
}
