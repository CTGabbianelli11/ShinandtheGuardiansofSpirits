#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PillarFieldComponent.generated.h"

class ATelegraphActor;
class AStrikeActor;

/**
 * Holds a set of component-local pillar locations and, on DoPillarAttack(N), floor-snaps a random
 * N of them and fires the shipped telegraph->strike pipeline at each: a deferred ATelegraphActor
 * warns, then spawns StrikeClass when its wind-up completes. PillarPoints is authored via the
 * editor-module component visualizer (numeric entry in Details also works).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UPillarFieldComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    // Preset strike locations in component-local space. The visualizer edits these directly, so no
    // MakeEditWidget meta - that would add a redundant per-element engine widget.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Field")
    TArray<FVector> PillarPoints;

    // Ground warning spawned per chosen pillar; owns the reaction-window timing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Field")
    TSubclassOf<ATelegraphActor> TelegraphClass;

    // Spawned by each telegraph when its fill completes - the erupting pillar/damage.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Field")
    TSubclassOf<AStrikeActor> StrikeClass;

    // Danger radius passed to every telegraph and strike.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Field", meta = (ClampMin = "0.0"))
    float Radius = 120.f;

    // Wind-up length in seconds for each telegraph.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Field", meta = (ClampMin = "0.01"))
    float TelegraphDuration = 1.0f;

    // Max extra random delay (seconds) before an individual pillar's telegraph spawns. 0 = all pillars fire simultaneously.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Field", meta = (ClampMin = "0.0"))
    float StaggerWindow = 0.f;

    UFUNCTION(BlueprintCallable, Category = "Pillar Field")
    void DoPillarAttack(int32 NumPillars);

private:
    // FloorPoint is passed by value: this is a FTimerDelegate payload, which must outlive the stack.
    void SpawnTelegraphAt(FVector FloorPoint);
};
