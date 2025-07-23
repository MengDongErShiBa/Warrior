#include "AbilitySystem/AbilityActor/RingGustActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void ARingGustActor::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 避免与自己碰撞
	if (OtherActor == GetOwner()) return;

	// 获取目标ASC
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (TargetASC)
	{
		// 应用伤害效果
		FGameplayEffectSpecHandle DamageEffectSpec = // 创建伤害效果;
		TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpec.Data.Get());
        
		// 应用击退效果（假设有一个用于击退的GameplayEffect）
		if (KnockbackEffect)
		{
			FGameplayEffectSpecHandle KnockbackSpec = MakeOutgoingGameplayEffectSpec(KnockbackEffect);
			// 设置击退方向
			FVector Direction = OtherActor->GetActorLocation() - GetActorLocation();
			Direction.Normalize();
			// 设置击退力度
			UGameplayEffectExecutionKnockback::SetKnockbackForce(KnockbackSpec, Direction * KnockbackForce);
			TargetASC->ApplyGameplayEffectSpecToSelf(*KnockbackSpec.Data.Get());
		}

		// 如果应该减速，且目标有特定的标签（比如敌人），则应用减速效果
		if (bShouldApplySlow && TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy"))))
		{
			FGameplayEffectSpecHandle SlowEffectSpec = MakeOutgoingGameplayEffectSpec(SlowEffect);
			TargetASC->ApplyGameplayEffectSpecToSelf(*SlowEffectSpec.Data.Get());
		}
	}

	// 环境交互
	IEnvironmentInteractable* Environment = Cast<IEnvironmentInteractable>(OtherActor);
	if (Environment)
	{
		Environment->Execute_Interact(OtherActor, this);
	}
}
