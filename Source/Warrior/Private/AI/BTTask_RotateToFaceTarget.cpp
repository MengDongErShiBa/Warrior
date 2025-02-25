// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_RotateToFaceTarget.h"

#include "BehaviorTree/BlackboardData.h"

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
