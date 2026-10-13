#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileStrike.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * A straight-line projectile: on spawn it faces its travel direction and each tick sweeps
 * forward at Speed. The first pawn overlap or blocking impact applies damage and GetHit once,
 * then destroys the projectile. A miss expires after MaxRange.
 *
 * The sphere collision is the root so the swept move sweeps it (children teleport); the mesh is a
 * cosmetic child sized independently, so the visible shot can read larger than the fair hitbox.
 * Damage is point damage carrying the travel direction, so a blocking player's verdict faces the
 * projectile rather than its firer.
 */
UCLASS()
class RHYATOWEROFWISHES_API AProjectileStrike : public AActor
{
    GENERATED_BODY()

public:
    AProjectileStrike();
    virtual void Tick(float DeltaSeconds) override;

    // Spawns with flight parameters configured before BeginPlay; rotation sets travel direction.
    static AProjectileStrike* SpawnConfigured(UWorld* World, TSubclassOf<AProjectileStrike> Class, const FTransform& Transform, float InSpeed, float InMaxRange, AActor* Owner, APawn* Instigator);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    // Target caught by an overlap or blocking impact.
    virtual void OnImpact(AActor* OtherActor, const FHitResult& SweepResult);

    // Marks the projectile spent before invoking its flight-ending behaviour; safe to call twice.
    void EndFlight();

    // Called by EndFlight after the projectile is spent.
    virtual void OnFlightEnded();

    // A swinging weapon hitbox (ECC_WeaponHitbox) touched the projectile, before any pawn did.
    // Base: nothing — the projectile flies on through. Override to deflect.
    virtual void OnWeaponHit(AActor* WeaponActor, const FHitResult& SweepResult);

    // Grants a fresh MaxRange of flight from wherever the projectile is now (a deflection rearms
    // both the range check and the lifespan leak guard).
    void RestartFlight();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile Strike")
    USphereComponent* Sphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile Strike")
    UStaticMeshComponent* Mesh;

    // Radius of the damage hitbox. Set independently of the mesh so the visual can read larger.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Strike", meta = (ClampMin = "1.0"))
    float SphereRadius = 25.f;

    // Flight speed in units/second along the forward axis.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Strike", meta = (ClampMin = "1.0"))
    float Speed = 2000.f;

    // How far the projectile flies before destroying itself on a miss.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Strike", meta = (ClampMin = "1.0"))
    float MaxRange = 3000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Strike", meta = (ClampMin = "0.0"))
    float Damage = 20.f;

private:
    void HandleImpact(AActor* OtherActor, const FHitResult& Hit);

    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    float Traveled = 0.f;

    // Shared by overlap callbacks, blocking-hit callbacks, and the sweep result.
    bool bSpent = false;
};
