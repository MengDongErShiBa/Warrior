// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilites/HeroGameplayAbility_RingGust.h"

#include "WarriorBlueprintFunctionLibrary.h"
#include "WarriorGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"

UHeroGameplayAbility_RingGust::UHeroGameplayAbility_RingGust()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UHeroGameplayAbility_RingGust::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                    const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 检查是否处于冷却中
	if (GetWarriorAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(WarriorGameplayTags::Player_Cooldown_SpecialWeaponAbility_WindCyclone))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
	}

	// 添加蓄力标签
	UWarriorBlueprintFunctionLibrary::AddGameplayTagToActorIfNone(GetAvatarActorFromActorInfo(), WarriorGameplayTags::Player_Status_WindCyclone_Charging);

	// 计时
	CurrentChargeTime = 0.f;
	GetWorld()->GetTimerManager().SetTimer(ChargeTimer, [this]
	{
		CurrentChargeTime = FMath::Clamp(CurrentChargeTime + 0.05f, 0.f, MaxChargeTime);
		// 刷UI
	}, 0.05f, true);

	// 绑定输入释放
	UAbilityTask_WaitInputRelease* WaitInputRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	WaitInputRelease->OnRelease.AddDynamic(this, &ThisClass::OnChargeComplete);
	WaitInputRelease->ReadyForActivation();
}

void UHeroGameplayAbility_RingGust::OnChargeComplete(float TimeHeld)
{
	// 清理定时器
	GetWorld()->GetTimerManager().ClearTimer(ChargeTimer);

	// 移除蓄力标签
	UWarriorBlueprintFunctionLibrary::RemoveGameplayTagFromActorIfFound(GetAvatarActorFromActorInfo(), WarriorGameplayTags::Player_Status_WindCyclone_Charging);

	// 根据蓄力时长描述效果
	float Radius;
	float Damage;
	float Cooldown;
	bool bPlayFullVFX = false;

	if (TimeHeld < 0.5f)
	{
		Radius = 200.f;
		Damage = 30.f;
		Cooldown = 1.0f;
	}
	else if (TimeHeld < 1.0f)
	{
		// 半径350 伤害60 冷却 1秒
		Radius = 350.f;
		Damage = 60.f;
		Cooldown = 1.0f;
	}
	else
	{
		// 半径500 伤害100 冷却 2.5秒
		Radius = 500.f;
		Damage = 100.f;
		Cooldown = 2.5f;
		bPlayFullVFX = true;
	}

	// 检查是否有增益Buff
	if (GetWarriorAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(WarriorGameplayTags::Shared_Buff_WindMastery))
	{
		Damage *= 1.4f;
	}

	// 生成环风
	SpawnGustRing(Radius, Damage, bPlayFullVFX);

	// 使用冷却GE
	FGameplayEffectSpecHandle CooldownEffectSpecHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
		CooldownGameplayEffectClass,
		GetAbilityLevel(),
		GetWarriorAbilitySystemComponentFromActorInfo()->MakeEffectContext()
	);

	// 设置冷却时长
	CooldownEffectSpecHandle.Data->SetDuration(Cooldown, false);
	GetWarriorAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToSelf(*CooldownEffectSpecHandle.Data);
	// 结束技能
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UHeroGameplayAbility_RingGust::SpawnGustRing(float Radius, float Damage, bool bPlayFullVFX)
{
}
