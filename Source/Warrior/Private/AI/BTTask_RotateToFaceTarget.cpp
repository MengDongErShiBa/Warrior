// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_RotateToFaceTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_RotateToFaceTarget::UBTTask_RotateToFaceTarget()
{
	// Native Rotate to Face Target Actor
	NodeName = TEXT("自身旋转朝向目标演员");

	AnglePrecision = 10.f;
	RotationInterpSpeed = 5.f;

	// 开启Tick
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	bCreateNodeInstance = false;

	INIT_TASK_NODE_NOTIFY_FLAGS();

	InTargetToFaceKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetToFaceKey), AActor::StaticClass());
}

void UBTTask_RotateToFaceTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		InTargetToFaceKey.ResolveSelectedKey(*BBAsset);
	}
}

uint16 UBTTask_RotateToFaceTarget::GetInstanceMemorySize() const
{
	return sizeof(FRotateToFaceTargetMemory);
}

FString UBTTask_RotateToFaceTarget::GetStaticDescription() const
{
	const FString KeyDescrString = InTargetToFaceKey.SelectedKeyName.ToString();

	// smoothly rotates to face 
	return FString::Printf(TEXT("平滑旋转面朝 %s，旋转精度为 %s"), *KeyDescrString, *FString::SanitizeFloat(AnglePrecision));
}

EBTNodeResult::Type UBTTask_RotateToFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UObject* ActorObject = OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetToFaceKey.SelectedKeyName);
	AActor* TargetActor = Cast<AActor>(ActorObject);

	APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();

	FRotateToFaceTargetMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetMemory>(NodeMemory);
	check(Memory);

	Memory->OwningPawn = OwningPawn;
	Memory->TargetActor = TargetActor;

	if (!Memory->IsValid())
	{
		return EBTNodeResult::Failed;	
	}

	if (HasReachedAnglePrecision(OwningPawn, TargetActor))
	{
		Memory->Reset();
		return EBTNodeResult::Succeeded;
	}

	// 正在执行
	return EBTNodeResult::InProgress;
}

void UBTTask_RotateToFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FRotateToFaceTargetMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetMemory>(NodeMemory);

	if (!Memory->IsValid())
	{
		// 完成Task,失败
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}

	if (HasReachedAnglePrecision(Memory->OwningPawn.Get(), Memory->TargetActor.Get()))
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		// 旋转朝向
		const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Memory->OwningPawn->GetActorLocation(), Memory->TargetActor->GetActorLocation());
		const FRotator TargetRot = FMath::RInterpTo(Memory->OwningPawn->GetActorRotation(), LookAtRot, DeltaSeconds, RotationInterpSpeed);
		Memory->OwningPawn->SetActorRotation(TargetRot);
	}
	
}

bool UBTTask_RotateToFaceTarget::HasReachedAnglePrecision(APawn* QueryPawn, AActor* TargetActor)
{
	if (!QueryPawn || !TargetActor) return false;

	// 获取当前朝向向量和目标方向向量
	const FVector OwnerForward = QueryPawn->GetActorForwardVector();
	const FVector OwnerToTargetDir = (TargetActor->GetActorLocation() - QueryPawn->GetActorLocation()).GetSafeNormal();

	// 计算夹角（0~180度）
	const float DotResult = FVector::DotProduct(OwnerForward, OwnerToTargetDir);
	const float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));

	return FMath::Abs(AngleDiff) <= AnglePrecision;
}
