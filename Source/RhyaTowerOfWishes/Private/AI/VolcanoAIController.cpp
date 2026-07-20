#include "AI/VolcanoAIController.h"
#include "AI/VolcanoPawn.h"
#include "BehaviorTree/BehaviorTree.h"

void AVolcanoAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AVolcanoPawn* Volcano = Cast<AVolcanoPawn>(InPawn);
    if (!ensureMsgf(Volcano, TEXT("%s possessed %s, which is not an AVolcanoPawn"),
        *GetName(), InPawn ? *InPawn->GetName() : TEXT("null")))
    {
        return;
    }
    if (!ensureMsgf(Volcano->BehaviorTreeAsset, TEXT("%s: %s has no BehaviorTreeAsset assigned"),
        *GetName(), *Volcano->GetName()))
    {
        return;
    }

    // RunBehaviorTree also creates and initializes the blackboard from the tree's asset. The
    // explicit UseBlackboard-then-seed-keys dance is only needed when keys must be valid before
    // the first tick; every key here is rewritten each interval by BTService_VolcanoSense.
    RunBehaviorTree(Volcano->BehaviorTreeAsset);
}
