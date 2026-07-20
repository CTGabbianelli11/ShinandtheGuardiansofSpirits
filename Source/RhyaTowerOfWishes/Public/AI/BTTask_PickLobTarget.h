#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_PickLobTarget.generated.h"

/**
 * Picks the next volley's center - the target actor's location plus a uniform 2D scatter, snapped
 * to the floor - and writes it to the blackboard. ThrowAt aims at exactly the point it is given,
 * so the snap happens here, not in the lob component.
 */
UCLASS()
class RHYATOWEROFWISHES_API UBTTask_PickLobTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_PickLobTarget();

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // Max 2D distance from the target the volley center may land. 0 = dead-on.
    UPROPERTY(EditAnywhere, Category = "Lob", meta = (ClampMin = "0.0"))
    float ScatterRadius = 300.f;

    // Object(Actor): who to aim near (in).
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    // Vector: the chosen, floor-snapped volley center (out).
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;
};
