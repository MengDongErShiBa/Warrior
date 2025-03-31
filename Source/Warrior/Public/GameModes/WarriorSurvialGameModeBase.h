// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/WarriorGameModeBase.h"
#include "WarriorSurvialGameModeBase.generated.h"

class AWarriorEnemyCharacter;
/**
 * 游戏状态
 */
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

/**
 * 敌人生成信息
 */
USTRUCT(BlueprintType)
struct  FWarriorEnemyWaveSpawnerInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClassToSpawn;
	UPROPERTY(EditAnywhere)
	int32 MinPerSpawnCount = 1;
	UPROPERTY(EditAnywhere)
	int32 MaxPerSpawnCount = 3;
};

USTRUCT(BlueprintType)
struct FWarriorEnemyWaveSpawnerInfoTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<FWarriorEnemyWaveSpawnerInfo> EnemyWaveSpawnerDefinitions;

	/**
	 * 产生此波的敌人总数
	 */
	UPROPERTY(EditAnywhere)
	int32 TotalEnemyToSpawnThisWave = 1;
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
	// 是否完成了所有波次
	bool HasFinishedAllWaves() const;
	// 预加载下一波敌人
	void PreLoadNextWaveEnemies();
	// 获取当前的数据行
	FWarriorEnemyWaveSpawnerInfoTableRow* GetCurrentWaveSpawnerTableRow() const;
	// 尝试生成波次内的敌人
	int32 TrySpawnWaveEnemies();

	/**
	 * 是否支持继续生成敌人
	 * @return 
	 */
	bool ShouldKeepSpawnEnemies() const;
	
	UPROPERTY()
	EWarriorSurvialGameModeState CurrentSurvialGameModeState;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSurvialGameModeStateChanged OnSurvialGameModeStateChanged;

	/**
	 * 生成信息表
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	UDataTable* EnemyWaveSpawnerDataTable;

	/**
	 * 总波次
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	int32 TotalWavesToSpawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWaveCount = 1;

	UPROPERTY()
	int32 CurrentSpawnedEnemiesCounter = 0;

	UPROPERTY()
	int32 TotalSpawnedEnemiesThisWaveCounter = 0;

	UPROPERTY()
	TArray<AActor*> TargetPointsArray;
	
	/**
	 * 已经流逝的时间
	 */
	UPROPERTY()
	float TimePassedSinceStart = 0.f;

	/**
	 * 生成等待时长
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	float SpawnNewWaveWaitTime = 5.f;

	/**
	 * 生成延时等待时长
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	float SpawnEnemiesDelayTime = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	float WaveCompletedWaitTime = 5.f;

	UPROPERTY()
	TMap<TSoftClassPtr<AWarriorEnemyCharacter>, UClass*> PreLoadedEnemyClassMap;
};


