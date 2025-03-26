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