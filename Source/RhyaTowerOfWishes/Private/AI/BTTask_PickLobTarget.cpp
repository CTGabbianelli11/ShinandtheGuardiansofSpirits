#include "AI/BTTask_PickLobTarget.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Combat/CombatUtils.h"
#include "Engine/World.h"

UBTTask_PickLobTarget::UBTTask_PickLobTarget()
{
    NodeName = TEXT("Pick Lob Target");

    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PickLobTarget, TargetActorKey), AActor::StaticClass());
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PickLobTarget, TargetLocationKey));

    // Convention defaults; see BTService_VolcanoSense's ctor for the auto-pick rationale.
    TargetActorKey.SelectedKeyName = TEXT("TargetActor");
    TargetLocationKey.SelectedKeyName = TEXT("TargetLocation");
}

void UBTTask_PickLobTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    UBlackboardData* BlackboardAsset = GetBlackboardAsset();
    if (ensureMsgf(BlackboardAsset, TEXT("%s: behavior tree %s has no blackboard asset"), *GetName(), *Asset.GetName()))
    {
        TargetActorKey.ResolveSelectedKey(*BlackboardAsset);
        TargetLocationKey.ResolveSelectedKey(*BlackboardAsset);
    }
}

EBTNodeResult::Type UBTTask_PickLobTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    UWorld* World = OwnerComp.GetWorld();
    AAIController* Controller = OwnerComp.GetAIOwner();
    APawn* Volcano = Controller ? Controller->GetPawn() : nullptr;
    if (!Blackboard || !World || !Volcano)
    {
        return EBTNodeResult::Failed;
    }
    if (!ensureMsgf(TargetActorKey.IsSet() && TargetLocationKey.IsSet(),
        TEXT("%s: unbound blackboard key(s) - TargetActor:%d TargetLocation:%d"),
        *GetName(), (int32)TargetActorKey.IsSet(), (int32)TargetLocationKey.IsSet()))
    {
        return EBTNodeResult::Failed;
    }

    const AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
    if (!Target)
    {
        return EBTNodeResult::Failed;
    }

    const FVector TargetLocation = Target->GetActorLocation();
    const FVector2D Scatter = FMath::RandPointInCircle(ScatterRadius);
    const FVector Candidate = TargetLocation + FVector(Scatter.X, Scatter.Y, 0.f);

    // The scattered point can hang past a ledge; the spot under the target is the known-good
    // fallback. No floor there either means there is nothing sane to aim at this loop.
    FVector FloorPoint;
    if (!Rhya::SnapToFloor(*World, Candidate, Volcano, FloorPoint) &&
        !Rhya::SnapToFloor(*World, TargetLocation, Volcano, FloorPoint))
    {
        return EBTNodeResult::Failed;
    }

    Blackboard->SetValueAsVector(TargetLocationKey.SelectedKeyName, FloorPoint);
    return EBTNodeResult::Succeeded;
}
