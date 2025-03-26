// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WarriorEnumType.h"

/**
 * 自定义冷却节点
 */
class FWarriorCountDownAction : public FPendingLatentAction
{
public:
	FWarriorCountDownAction(float InTotalCountTime, float InUpdateInterval, float& InOutRemainingTime, EWarriorCountDownActionOutPut& InCountDownOutput, const FLatentActionInfo& LatentActionInfo)
	: bNeedToCancel(false)
	, TotalCountDownTime(InTotalCountTime)
	, UpdateInterval(InUpdateInterval)
	, OutRemainingTime(InOutRemainingTime)
	, CountDownOutput(InCountDownOutput)
	, ExecutionFunction(LatentActionInfo.ExecutionFunction)
	, OutputLink(LatentActionInfo.Linkage)
	, CallbackTarget(LatentActionInfo.CallbackTarget)
	, ELapsedInterval(0.f)
	, ElapsedTimeSinceStart(0.f)
	{
		
	}
	
private:
	bool bNeedToCancel;
	float TotalCountDownTime;
	float UpdateInterval;
	float& OutRemainingTime;
	EWarriorCountDownActionOutPut& CountDownOutput;
	// 执行函数
	FName ExecutionFunction;
	//
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	// 运行时间间隔
	float ELapsedInterval;
	// 开始后经过的时长
	float ElapsedTimeSinceStart;
};