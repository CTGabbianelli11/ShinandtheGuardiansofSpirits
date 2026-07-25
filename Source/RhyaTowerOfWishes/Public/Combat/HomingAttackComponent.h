#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HomingAttackComponent.generated.h"

class AHomingStrike;

/**
 * Fires a volley of homing projectiles at a target actor.
 *
 * Like UProjectileAttackComponent this owns no telegraph or wind-up: the firer's wind-up montage
 * is the warning, and keeping the timing on the caller keeps the muzzle synced to its pose.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UHomingAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // The projectile to fire; carries its own SphereRadius/Damage/ExplosionRadius and visuals.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack")
    TSubclassOf<AHomingStrike> ProjectileClass;

    // Flight speed in units/second, constant for the whole flight; steering never changes it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack", meta = (ClampMin = "1.0"))
    float Speed = 1200.f;

    // Seconds a projectile chases before detonating in place.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack", meta = (ClampMin = "0.1"))
    float LifeSeconds = 6.f;

    // Max steering rate in degrees/second, passed to each projectile. Tune against Speed:
    // the turning circle radius is Speed / TurnRate-in-radians.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack", meta = (ClampMin = "0.0"))
    float TurnRate = 90.f;

    // Projectiles per FireAt call, launched simultaneously.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack", meta = (ClampMin = "1"))
    int32 Count = 3;

    // Total horizontal arc the volley's initial headings fan across, centered on the target.
    // Zero stacks the volley on one path; spread lets each projectile's arc read separately.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack", meta = (ClampMin = "0.0", ClampMax = "360.0"))
    float FanSpreadDegrees = 90.f;

    // Spawn point relative to the owner (X forward, Z up). Keep it outside the owner's own capsule
    // so a projectile never spawns already overlapping its firer.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing Attack")
    FVector MuzzleOffset = FVector(80.f, 0.f, 100.f);

    // Fires the volley at Target, which every projectile chases until it detonates.
    UFUNCTION(BlueprintCallable, Category = "Homing Attack")
    void FireAt(AActor* Target);
};
