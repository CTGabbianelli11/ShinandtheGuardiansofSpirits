#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_VolcanoSense.generated.h"

/**
 * The volcano tree's only senses. Each interval it mirrors the pawn's on/off switch and the
 * player's presence into the blackboard; the pawn owns the truth, the blackboard is a mirror, so
 * no key ever needs seeding before the tree starts.
 */
UCLASS()
class RHYATOWEROFWISHES_API UBTService_VolcanoSense : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_VolcanoSense();

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // Bool: mirror of AVolcanoPawn::AreEruptionsActive().
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector EruptionsActiveKey;

    // Object(Actor): the player pawn; cleared while no valid player exists.
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    // Bool: player is within the pawn's ArenaRadius (2D).
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector PlayerInRangeKey;
};
