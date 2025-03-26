// Fill out your copyright notice in the Description page of Project Settings.


#include "WarriorTypes/WarriorCountDownAction.h"

void FWarriorCountDownAction::UpdateOperation(FLatentResponse& Response)
{
	if (bNeedToCancel)
	{
		// 需要取消
		CountDownOutput = EWarriorCountDownActionOutPut::Cancelled;
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		return;
	}

	if (ElapsedTimeSinceStart >= TotalCountDownTime)
	{
		// 经过时长大于等于总时长，完成
		CountDownOutput = EWarriorCountDownActionOutPut::Completed;
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		return;
	}

	if (ELapsedInterval < UpdateInterval)
	{
		ELapsedInterval += Response.ElapsedTime();
	}
	else
	{
		ElapsedTimeSinceStart += UpdateInterval > 0.f ? UpdateInterval : Response.ElapsedTime();

		OutRemainingTime = TotalCountDownTime - ElapsedTimeSinceStart;

		CountDownOutput = EWarriorCountDownActionOutPut::Updated;

		// 更新
		Response.TriggerLink(ExecutionFunction, OutputLink, CallbackTarget);

		ELapsedInterval = 0.f;
	}
}

void FWarriorCountDownAction::CancelAction()
{
	bNeedToCancel = true;
}
