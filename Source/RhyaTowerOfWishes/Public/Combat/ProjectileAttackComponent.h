#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectileAttackComponent.generated.h"

class AProjectileStrike;

/**
 * Fires a straight-line projectile at a target point: FireAt spawns ProjectileClass at the
 * owner's muzzle, aimed at the target as it was WHEN FIRED (a snapshot, so the shot is dodgeable),
 * and the projectile flies until it hits a pawn, hits world geometry, or reaches MaxRange.
 *
 * This component owns no telegraph or wind-up: the firer's wind-up montage is the warning, and
 * keeping the timing on the caller keeps the muzzle synced to the enemy's pose at the release frame.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UProjectileAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // The projectile to fire; carries its own SphereRadius/Damage and visuals.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Attack")
    TSubclassOf<AProjectileStrike> ProjectileClass;

    // Flight speed in units/second, passed to the spawned projectile.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Attack", meta = (ClampMin = "1.0"))
    float Speed = 2000.f;

    // Range in units before an un-hit projectile destroys itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Attack", meta = (ClampMin = "1.0"))
    float MaxRange = 3000.f;

    // Spawn point relative to the owner (X forward, Z up). Keep it outside the owner's own capsule
    // so the projectile never spawns already overlapping its firer.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Attack")
    FVector MuzzleOffset = FVector(80.f, 0.f, 100.f);

    // Fires one projectile from the muzzle toward TargetLocation, aimed at the snapshot taken now.
    UFUNCTION(BlueprintCallable, Category = "Projectile Attack")
    void FireAt(FVector TargetLocation);
};
