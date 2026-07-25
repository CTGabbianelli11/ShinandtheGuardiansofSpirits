#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileStrike.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * A straight-line projectile: on spawn it faces its travel direction and each tick sweeps
 * forward at Speed, damaging the first pawn it passes through once and destroying itself on that
 * hit, on a blocking world hit, or once it has flown MaxRange without hitting anything.
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

    // Spawns Class at Transform (whose ROTATION is the travel direction) with the flight
    // parameters already configured. Encapsulates the SpawnActorDeferred/FinishSpawning ordering
    // the plain properties depend on - returns nullptr (after an ensure) on bad input or a failed spawn.
    static AProjectileStrike* SpawnConfigured(UWorld* World, TSubclassOf<AProjectileStrike> Class, const FTransform& Transform, float InSpeed, float InMaxRange, AActor* Owner, APawn* Instigator);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    // The pawn the sweep caught. Base: point damage along the flight direction, then vanish.
    virtual void OnPawnHit(AActor* OtherActor, const FHitResult& SweepResult);

    // Flight over without catching a pawn — blocking world hit or MaxRange spent. Base: vanish.
    virtual void OnFlightEnded();

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
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    float Traveled = 0.f;

    // Set the instant the projectile hits a pawn, so a single sweep can't also destroy on a wall.
    bool bSpent = false;
};
