// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "WarriorProjectileBase.generated.h"

class UNiagaraComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UBoxComponent;

UENUM(BlueprintType, DisplayName ="伤害类型")
enum class EProjectileDamagePolicy : uint8
{
	OnHit UMETA(DisplayName = "命中"),
	OnBeginOverlap UMETA(DisplayName = "开始重叠"),
};

UCLASS()
class WARRIOR_API AWarriorProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWarriorProjectileBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	UBoxComponent* ProjectileCollisionBox;

	/**
	 * niagara奶瓜效果
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	UNiagaraComponent* ProjectileNiagaraComponent;

	/**
	 * 弹丸移动组件
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovementComponent;

	/**
	 * 伤害类型
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	EProjectileDamagePolicy ProjectileDamagePolicy = EProjectileDamagePolicy::OnHit;
};
