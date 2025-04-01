#pragma once


#include "WarriorEnumType.generated.h"

UENUM()
enum class EWarriorConfirmType : uint8
{
	Yes,
	No
};

UENUM()
enum class EWarriorValidType : uint8
{
	Valid,
	InValid,
};

UENUM()
enum class EWarriorSuccessType : uint8
{
	Successful,
	Failed,
};

UENUM()
enum class EWarriorCountDownActionInput : uint8
{
	Start,
	Cancel
};

UENUM()
enum class EWarriorCountDownActionOutPut : uint8
{
	Updated,
	Completed,
	Cancelled
};

UENUM(BlueprintType)
enum class EWarriorGameDifficulty : uint8
{
	Easy UMETA(DisplayName = "简单"),
	Normal UMETA(DisplayName = "普通"),
	Hard UMETA(DisplayName = "困难"),
	VeryHard UMETA(DisplayName = "地狱"),
};