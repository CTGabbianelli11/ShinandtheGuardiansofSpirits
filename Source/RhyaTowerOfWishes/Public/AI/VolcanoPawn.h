#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VolcanoPawn.generated.h"

class UAC_LobbedProjectile;
class UBehaviorTree;

/**
 * Invisible pawn parked at a volcano crater that lobs telegraphed lava bombs at the player while
 * eruptions are on. A pawn rather than a plain actor because the eruption brain runs in an
 * AIController, and controllers can only possess pawns. Encounter logic drives it solely through
 * StartEruptions/StopEruptions - it never reads boss or fight state itself.
 */
UCLASS()
class RHYATOWEROFWISHES_API AVolcanoPawn : public APawn
{
    GENERATED_BODY()

public:
    AVolcanoPawn();

    // Run by AVolcanoAIController on possession.
    UPROPERTY(EditAnywhere, Category = "Volcano")
    UBehaviorTree* BehaviorTreeAsset = nullptr;

    // Erupt from level start without waiting for a StartEruptions() call.
    UPROPERTY(EditAnywhere, Category = "Volcano")
    bool bAutoStart = false;

    // 2D distance from the pawn the player must be within for eruptions to fire.
    UPROPERTY(EditAnywhere, Category = "Volcano", meta = (ClampMin = "0.0"))
    float ArenaRadius = 4000.f;

    // Owns the projectile/telegraph/strike classes and the arc (FlightTime, LaunchOffset).
    UPROPERTY(VisibleAnywhere, Category = "Volcano")
    UAC_LobbedProjectile* LobComponent = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Volcano")
    void StartEruptions();

    UFUNCTION(BlueprintCallable, Category = "Volcano")
    void StopEruptions();

    UFUNCTION(BlueprintPure, Category = "Volcano")
    bool AreEruptionsActive() const { return bEruptionsActive; }

protected:
    virtual void BeginPlay() override;

private:
    // Source of truth for the on/off state. The behavior tree never owns this; a sense service
    // mirrors it into the blackboard each interval, so Start/Stop are safe to call at any time,
    // including before the controller has possessed the pawn.
    bool bEruptionsActive = false;
};
