#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_LavaVolley.generated.h"

class AVolcanoPawn;

// Per-run scratch state. BT node objects are shared by every agent running the tree; anything
// that varies per run lives in this block, sized by GetInstanceMemorySize.
struct FBTLavaVolleyMemory
{
    FVector VolleyCenter;
    int32 BombsRemaining;
    float TimeUntilNext;
};

/**
 * Throws BombCount lava bombs at the blackboard's volley center, one every StaggerSeconds, each
 * with its own scatter. Latent across ticks. No AbortTask override on purpose: ThrowAt is
 * fire-and-forget and this task holds no timers, delegates, or actor references, so an abort just
 * stops the ticking - unthrown bombs never happen and in-flight ones own their own lifecycle.
 */
UCLASS()
class RHYATOWEROFWISHES_API UBTTask_LavaVolley : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_LavaVolley();

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;

    UPROPERTY(EditAnywhere, Category = "Volley", meta = (ClampMin = "1"))
    int32 BombCount = 3;

    UPROPERTY(EditAnywhere, Category = "Volley", meta = (ClampMin = "0.0"))
    float StaggerSeconds = 0.4f;

    // Per-bomb 2D scatter around the volley center; keeps a volley from stacking on one point.
    UPROPERTY(EditAnywhere, Category = "Volley", meta = (ClampMin = "0.0"))
    float BombScatter = 250.f;

    // Vector: volley center written by BTTask_PickLobTarget. Read once at task start so the
    // volley stays committed to one area while the player moves.
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

private:
    void ThrowOne(const AVolcanoPawn& Volcano, const FVector& Center) const;
};
