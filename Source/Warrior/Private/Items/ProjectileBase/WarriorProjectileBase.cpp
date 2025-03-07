// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ProjectileBase/WarriorProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "WarriorBlueprintFunctionLibrary.h"
#include "WarriorDebugHelper.h"
#include "WarriorGameplayTags.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AWarriorProjectileBase::AWarriorProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProjectileCollisionBox"));
	SetRootComponent(ProjectileCollisionBox);
	// 设置碰撞
	ProjectileCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	// 监听事件
	ProjectileCollisionBox->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnProjectileHit);
	ProjectileCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnProjectileBeginOverlap);

	ProjectileNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileNiagaraComponent"));
	ProjectileNiagaraComponent->SetupAttachment(GetRootComponent());

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 700.f;
	ProjectileMovementComponent->MaxSpeed = 900.f;
	ProjectileMovementComponent->Velocity = FVector(1.f, 0.f, 0.f);
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;

	// 四秒后自动销毁，调用Destroy
	InitialLifeSpan = 4.f;
}

// Called when the game starts or when spawned
void AWarriorProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileDamagePolicy == EProjectileDamagePolicy::OnBeginOverlap)
	{
		// 如果是重叠政策，则改为响应为重叠
		ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
	
}

void AWarriorProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	BP_OnSpawnProjectileHitFX(Hit.ImpactPoint);
	
	APawn* HitPawn = Cast<APawn>(OtherActor);

	if (!HitPawn || !UWarriorBlueprintFunctionLibrary::IsTargetPawnHostile(GetInstigator(), HitPawn))
	{
		// 并没有击中敌人
		Destroy();
		return;
	}

	bool bIsValidBlock = false;

	// 检查玩家是否在抵挡
	const bool bIsPlayerBlocking = UWarriorBlueprintFunctionLibrary::NativeDoesActorHaveTag(HitPawn, WarriorGameplayTags::Player_Status_Blocking);

	if (bIsPlayerBlocking)
	{
		bIsValidBlock = UWarriorBlueprintFunctionLibrary::IsValidBlock(this, HitPawn);
	}

	FGameplayEventData Data;
	Data.Instigator = this;
	Data.Target = HitPawn;

	if (bIsValidBlock)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			HitPawn,
			WarriorGameplayTags::Player_Event_SuccessfulBlock,
			Data
			);
		Debug::Print(TEXT("阻挡了"));
	}
	else
	{
		// 应用伤害
		// Apply projectile damage
		HandleApplyProjectileDamage(HitPawn, Data);
		Debug::Print(TEXT("命中了"));
	}

	Destroy();
}

void AWarriorProjectileBase::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void AWarriorProjectileBase::HandleApplyProjectileDamage(APawn* InHitPawn, const FGameplayEventData& InPayload)
{
	checkf(ProjectileDamageEffectSpecHandle.IsValid(), TEXT("Forgot to assign a valid spec handle to the projectileDamageEffectSpecHandle: %s"), *GetActorNameOrLabel());
	const bool bWasApplied = UWarriorBlueprintFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(GetInstigator(), InHitPawn, ProjectileDamageEffectSpecHandle);

	// 应用成功
	if (bWasApplied)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			InHitPawn,
			WarriorGameplayTags::Shared_Event_HitReact,
			InPayload
		);
	}
	
}
