// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToFaceTarget.generated.h"

struct FRotateToFaceTargetMemory
{
	TWeakObjectPtr<APawn> OwningPawn;
	TWeakObjectPtr<AActor> TargetActor;

	bool IsValid() const
	{
		return OwningPawn.IsValid() && TargetActor.IsValid();
	}

	void Reset()
	{
		OwningPawn.Reset();
		TargetActor.Reset();
	}
};

/**
 * 
 */
UCLASS()
class WARRIOR_API UBTTask_RotateToFaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

	UBTTask_RotateToFaceTarget();

	// ~ Begin UBTNode Interface
	// 通过黑板初始化
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	// 分配内存大小
	virtual uint16 GetInstanceMemorySize() const override;
	// 描述
	virtual FString GetStaticDescription() const override;
	// ~ End UBTNode Interface

	/**
	 * 角度精度
	 */
	UPROPERTY(EditAnywhere, Category = "Face Target")
	float AnglePrecision;

	/**
	 * 旋转插值速度
	 */
	UPROPERTY(EditAnywhere, Category = "Face Target")
	float RotationInterpSpeed;

	UPROPERTY(EditAnywhere, Category = "Face Target")
	FBlackboardKeySelector InTargetToFaceKey;
};
