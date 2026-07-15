#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WallAttackComponent.generated.h"

class AWallStrike;
class AWallTelegraph;

/**
 * Fires a sliding-wall attack: DoWallAttack floor-snaps Start, spawns WallClass there
 * immediately (dormant, so the player sees the wall during the wind-up) plus a telegraph decal
 * over the full sweep rectangle; after TelegraphDuration the wall arms and slides Distance at
 * SlideSpeed. The telegraph rectangle is measured from the spawned wall's actual footprint, so
 * resizing the wall mesh in its BP keeps the warning and the hitbox in agreement.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UWallAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sweep-area warning spawned over the wall's path; purely visual.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Attack")
    TSubclassOf<AWallTelegraph> TelegraphClass;

    // The wall itself: dormant through the wind-up, then slides and damages.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Attack")
    TSubclassOf<AWallStrike> WallClass;

    // Wind-up length in seconds: drives the telegraph fill AND the wall's dormant phase.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Attack", meta = (ClampMin = "0.01"))
    float TelegraphDuration = 1.0f;

    // Slide speed in units/second. Speed rather than duration so dodge difficulty stays
    // consistent regardless of the distance a caller picks.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Attack", meta = (ClampMin = "1.0"))
    float SlideSpeed = 800.f;

    // Direction is flattened to the ground plane and normalized; Start is floor-snapped.
    UFUNCTION(BlueprintCallable, Category = "Wall Attack")
    void DoWallAttack(FVector Start, FVector Direction, float Distance);
};
