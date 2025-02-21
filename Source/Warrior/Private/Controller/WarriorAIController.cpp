// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/WarriorAIController.h"

#include "WarriorDebugHelper.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AWarriorAIController::AWarriorAIController(const FObjectInitializer& ObjectInitializer)
	// 替换父类中ROV的寻路为Detour Crowd
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>("PathFollowingComponent"))
{
	// 视觉设置
	AISenseConfig_Sight = CreateDefaultSubobject<UAISenseConfig_Sight>("EnemySenseConfig_Sight");
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;
	AISenseConfig_Sight->SightRadius = 5000.f;
	AISenseConfig_Sight->LoseSightRadius = 0.f;
	AISenseConfig_Sight->PeripheralVisionAngleDegrees = 360.f;
	
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("EnemyPerceptionComponent");
	// 添加配置，里面是个数组，可以添加多个
	EnemyPerceptionComponent->ConfigureSense(*AISenseConfig_Sight);
	EnemyPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnEnemyPerceptionUpdated);
}

ETeamAttitude::Type AWarriorAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(PawnToCheck->GetController());

	// 如果目标的ID 不等于自身ID敌方
	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() != GetGenericTeamId())
	{
		return ETeamAttitude::Hostile;
	}

	return ETeamAttitude::Friendly;
}

void AWarriorAIController::BeginPlay()
{
	Super::BeginPlay();

	// 设置团体编号
	SetGenericTeamId(FGenericTeamId(1));
	
	if (UCrowdFollowingComponent* CrowComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		// 讲师自己总结的配置

		// 设置拥挤模拟状态,是否避让
		CrowComp->SetCrowdSimulationState(bEnableDetourCrowdAvoidance ? ECrowdSimulationState::Enabled : ECrowdSimulationState::Disabled);

		// 设置拥挤避让的质量
		switch (DetourCrowdAvoidanceQuality)
		{
			case 1:
				CrowComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Low);
				break;
			case 2:
				CrowComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium);
				break;
			case 3:
				CrowComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
				break;
			case 4:
				CrowComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
				break;
			default:
				break;
		}

		// 角色的避让组设置为 1。避让组用于将角色分组，以便在拥挤环境中进行更好的避让决策。
		CrowComp->SetAvoidanceGroup(1);
		// 角色需要避免的组为 1。这意味着该角色会避免与同一组的其他角色发生碰撞。
		CrowComp->SetGroupsToAvoid(1);
		// 角色的碰撞查询范围，使用 CollisionQueryRange 变量的值。这个范围决定了角色在进行碰撞检测时的有效范围。
		CrowComp->SetCrowdCollisionQueryRange(CollisionQueryRange);
	}

	
}

void AWarriorAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed() && Actor)
	{
		if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
		{
			BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
		}
	}
}
