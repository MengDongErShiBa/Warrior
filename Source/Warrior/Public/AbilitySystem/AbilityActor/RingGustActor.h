#pragma once

#include "RingGustActor.generated.h"

class USphereComponent;

UCLASS(Blueprintable)
class ARingGustActor : public AActor
{
	GENERATED_BODY()
public:
	ARingGustActor();

	void Initialize(float InRingRadius, float InBaseDamage);

	void SetAdditionalEffects(bool bPlayFullVFX, bool bPlayApplySlow);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionComponent;

	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplyEffectToTarget(AActor* Target);

	float RingRadius;
	float BaseDamage;
	bool bPlayFullVFX;
	bool bPlayApplySlow;

	// 流逝
	FTimerHandle ExpandTimer;
	float CurrentRadius;
};
