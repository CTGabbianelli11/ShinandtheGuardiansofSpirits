#pragma once

#include "CoreMinimal.h"
#include "Combat/ProjectileStrike.h"
#include "HomingStrike.generated.h"

/**
 * A homing projectile: flies like AProjectileStrike but steers toward HomingTarget every tick,
 * turning no faster than TurnRate 
 */
UCLASS()
class RHYATOWEROFWISHES_API AHomingStrike : public AProjectileStrike
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;

    // Spawns Class at Transform (whose rotation is the initial heading) chasing Target with the
    // flight parameters already configured before FinishSpawning. Returns nullptr (after an
    // ensure) on bad input or a failed spawn.
    static AHomingStrike* SpawnConfigured(UWorld* World, TSubclassOf<AHomingStrike> Class, const FTransform& Transform, float InSpeed, float InLifeSeconds, float InTurnRate, AActor* Target, AActor* Owner, APawn* Instigator);

protected:
    virtual void OnPawnHit(AActor* OtherActor, const FHitResult& SweepResult) override;
    virtual void OnFlightEnded() override;

    // Max steering rate in degrees/second. Turning circle radius = Speed / TurnRate-in-radians.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Strike", meta = (ClampMin = "0.0"))
    float TurnRate = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Strike", meta = (ClampMin = "1.0"))
    float ExplosionRadius = 250.f;

    UPROPERTY()
    TWeakObjectPtr<AActor> HomingTarget;

    // Fired at the detonation point just before the actor vanishes; spawn blast FX/sound here.
    UFUNCTION(BlueprintImplementableEvent, Category = "Homing Strike")
    void OnDetonated();

private:
    void Detonate();
};
