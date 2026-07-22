#include "AI/BTTask_LavaVolley.h"
#include "AI/VolcanoPawn.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Combat/CombatUtils.h"
#include "Components/AC_LobbedProjectile.h"
#include "Engine/World.h"

UBTTask_LavaVolley::UBTTask_LavaVolley()
{
    NodeName = TEXT("Lava Volley");
    bNotifyTick = true;

    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_LavaVolley, TargetLocationKey));

    // Convention default; see BTService_VolcanoSense's ctor for the auto-pick rationale.
    TargetLocationKey.SelectedKeyName = TEXT("TargetLocation");
}

void UBTTask_LavaVolley::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    UBlackboardData* BlackboardAsset = GetBlackboardAsset();
    if (ensureMsgf(BlackboardAsset, TEXT("%s: behavior tree %s has no blackboard asset"), *GetName(), *Asset.GetName()))
    {
        TargetLocationKey.ResolveSelectedKey(*BlackboardAsset);
    }
}

uint16 UBTTask_LavaVolley::GetInstanceMemorySize() const
{
    return sizeof(FBTLavaVolleyMemory);
}

EBTNodeResult::Type UBTTask_LavaVolley::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    AAIController* Controller = OwnerComp.GetAIOwner();
    const AVolcanoPawn* Volcano = Controller ? Cast<AVolcanoPawn>(Controller->GetPawn()) : nullptr;
    if (!Blackboard || !Volcano)
    {
        return EBTNodeResult::Failed;
    }
    if (!ensureMsgf(TargetLocationKey.IsSet(),
        TEXT("%s: unbound TargetLocation blackboard key"), *GetName()))
    {
        return EBTNodeResult::Failed;
    }

    FBTLavaVolleyMemory* Memory = CastInstanceNodeMemory<FBTLavaVolleyMemory>(NodeMemory);
    Memory->VolleyCenter = Blackboard->GetValueAsVector(TargetLocationKey.SelectedKeyName);

    ThrowOne(*Volcano, Memory->VolleyCenter);
    if (BombCount <= 1)
    {
        return EBTNodeResult::Succeeded;
    }

    Memory->BombsRemaining = BombCount - 1;
    Memory->TimeUntilNext = StaggerSeconds;
    return EBTNodeResult::InProgress;
}

void UBTTask_LavaVolley::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    FBTLavaVolleyMemory* Memory = CastInstanceNodeMemory<FBTLavaVolleyMemory>(NodeMemory);
    Memory->TimeUntilNext -= DeltaSeconds;
    if (Memory->TimeUntilNext > 0.f)
    {
        return;
    }

    AAIController* Controller = OwnerComp.GetAIOwner();
    const AVolcanoPawn* Volcano = Controller ? Cast<AVolcanoPawn>(Controller->GetPawn()) : nullptr;
    if (!Volcano)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    ThrowOne(*Volcano, Memory->VolleyCenter);
    --Memory->BombsRemaining;
    if (Memory->BombsRemaining <= 0)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    Memory->TimeUntilNext = StaggerSeconds;
}

void UBTTask_LavaVolley::ThrowOne(const AVolcanoPawn& Volcano, const FVector& Center) const
{
    const FVector2D Scatter = FMath::RandPointInCircle(BombScatter);
    const FVector Candidate = Center + FVector(Scatter.X, Scatter.Y, 0.f);

    // Center is already known-good floor; a scatter point that finds none drifted past a ledge.
    FVector FloorPoint;
    if (!Rhya::SnapToFloor(*Volcano.GetWorld(), Candidate, &Volcano, FloorPoint))
    {
        FloorPoint = Center;
    }
    Volcano.LobComponent->ThrowAt(FloorPoint);
}
