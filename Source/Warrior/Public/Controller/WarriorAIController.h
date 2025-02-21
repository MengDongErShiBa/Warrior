// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WarriorAIController.generated.h"

struct FAIStimulus;
class UAISenseConfig_Sight;
/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorAIController : public AAIController
{
	GENERATED_BODY()

public:
	// TODO:需要查一查，老师讲说为了覆盖父类的组件，所以要传递这个
	AWarriorAIController(const FObjectInitializer& ObjectInitializer);

	// ~ Begin IGenericTeamAgentInterface Interface
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	// ~ End IGenericTeamAgentInterface Interface

protected:
	// 感知组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAIPerceptionComponent* EnemyPerceptionComponent;

	// 视力
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAISenseConfig_Sight* AISenseConfig_Sight;
	
	virtual void BeginPlay() override;

	/**
	 * 感知更新
	 * @param Actor 
	 * @param Stimulus 
	 */
	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	// 绕行人群避免配置
	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config")
	bool bEnableDetourCrowdAvoidance = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config", meta = (EditCondition = "bEnableDetourCrowdAvoidance", UIMin = "1", UIMax = "4"))
	int32 DetourCrowdAvoidanceQuality = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config", meta = (EditCondition = "bEnableDetourCrowdAvoidance"))
	float CollisionQueryRange = 600.f;
};
