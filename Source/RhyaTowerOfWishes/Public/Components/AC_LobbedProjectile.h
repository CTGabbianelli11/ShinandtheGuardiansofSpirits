#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_LobbedProjectile.generated.h"

class ATelegraphActor;
class AStrikeActor;

/**
 * Throws a cosmetic projectile on an arc while a ground telegraph warns the landing spot; when
 * the telegraph's wind-up completes it spawns StrikeClass there. The projectile carries no
 * damage of its own - FlightTime drives both its arc and the telegraph's timer, so they always
 * resolve together.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RHYATOWEROFWISHES_API UAC_LobbedProjectile : public UActorComponent
{
    GENERATED_BODY()

public:
    UAC_LobbedProjectile();

    // Cosmetic actor thrown through the air. Must own a UProjectileMovementComponent; this
    // component sets its Velocity directly and never deals damage through it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob")
    TSubclassOf<AActor> ProjectileClass;

    // Ground warning spawned at the target; owns the reaction-window timing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob")
    TSubclassOf<ATelegraphActor> TelegraphClass;

    // Spawned by the telegraph when its fill completes - the actual damage/blast.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob")
    TSubclassOf<AStrikeActor> StrikeClass;

    // Seconds from throw to landing. Drives the projectile's flight AND the telegraph's wind-up.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob", meta = (ClampMin = "0.05"))
    float FlightTime = 1.5f;

    // keeps the actor around a little longer after its flight but turns off vis and collisions
    // useful, for example, if you want particles from the flight to not get cleaned up right away.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob", meta = (ClampMin = "0.0"))
    float PostFlightTime = 0.f;

    // Danger radius passed to the telegraph.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob", meta = (ClampMin = "0.0"))
    float Radius = 200.f;

    // Launch point, relative to the owner's transform (e.g. raised to hand height).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lob")
    FVector LaunchOffset = FVector(0.f, 0.f, 100.f);

    // Spawns the telegraph at TargetLocation and throws ProjectileClass on an arc timed to land
    // there exactly when the telegraph's wind-up finishes.
    UFUNCTION(BlueprintCallable, Category = "Lob")
    void ThrowAt(FVector TargetLocation);
};
