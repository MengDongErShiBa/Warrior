// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_OrientToTargetActor.generated.h"

/**
 * 
 */
UCLASS()
class WARRIOR_API UBTService_OrientToTargetActor : public UBTService
{
	GENERATED_BODY()


	UBTService_OrientToTargetActor();

	// ~ Begin UBTNode Interface

	// 通过资产初始化，可以拿到BlackBoard
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	// 指定静态描述，节点下方的描述
	virtual FString GetStaticDescription() const override;
	// ~ End UBTNode Interface

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
	
	UPROPERTY(EditAnywhere, Category = "Target")
	FBlackboardKeySelector InTargetActorKey;

	/**
	 * 旋转插值速度
	 */
	UPROPERTY(EditAnywhere, Category = "Target")
	float RotationInterpSpeed;
};
