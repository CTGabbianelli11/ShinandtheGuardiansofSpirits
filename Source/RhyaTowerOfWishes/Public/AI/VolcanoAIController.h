#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "VolcanoAIController.generated.h"

/**
 * Possession glue for AVolcanoPawn: starts the pawn's behavior tree and nothing else. All
 * decision logic lives in the tree; all eruption state lives on the pawn.
 */
UCLASS()
class RHYATOWEROFWISHES_API AVolcanoAIController : public AAIController
{
    GENERATED_BODY()

protected:
    virtual void OnPossess(APawn* InPawn) override;
};
