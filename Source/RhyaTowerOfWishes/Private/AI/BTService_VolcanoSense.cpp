#include "AI/BTService_VolcanoSense.h"
#include "AI/VolcanoPawn.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Kismet/GameplayStatics.h"

UBTService_VolcanoSense::UBTService_VolcanoSense()
{
    NodeName = TEXT("Volcano Sense");
    Interval = 0.5f;
    RandomDeviation = 0.1f;

    EruptionsActiveKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_VolcanoSense, EruptionsActiveKey));
    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_VolcanoSense, TargetActorKey), AActor::StaticClass());
    PlayerInRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_VolcanoSense, PlayerInRangeKey));
}

void UBTService_VolcanoSense::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // Selectors bind to keys by name when the asset loads; an unresolved selector makes every
    // write a silent no-op.
    UBlackboardData* BlackboardAsset = GetBlackboardAsset();
    if (ensureMsgf(BlackboardAsset, TEXT("%s: behavior tree %s has no blackboard asset"), *GetName(), *Asset.GetName()))
    {
        EruptionsActiveKey.ResolveSelectedKey(*BlackboardAsset);
        TargetActorKey.ResolveSelectedKey(*BlackboardAsset);
        PlayerInRangeKey.ResolveSelectedKey(*BlackboardAsset);
    }
}

void UBTService_VolcanoSense::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* Controller = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    AVolcanoPawn* Volcano = Controller ? Cast<AVolcanoPawn>(Controller->GetPawn()) : nullptr;
    if (!ensureMsgf(Volcano && Blackboard, TEXT("%s ticking without a possessed AVolcanoPawn and blackboard"), *GetName()))
    {
        return;
    }

    Blackboard->SetValueAsBool(EruptionsActiveKey.SelectedKeyName, Volcano->AreEruptionsActive());

    APawn* Player = UGameplayStatics::GetPlayerPawn(Volcano, 0);
    if (Player && !Player->IsPendingKillPending())
    {
        Blackboard->SetValueAsObject(TargetActorKey.SelectedKeyName, Player);
        const bool bInRange =
            FVector::Dist2D(Volcano->GetActorLocation(), Player->GetActorLocation()) <= Volcano->ArenaRadius;
        Blackboard->SetValueAsBool(PlayerInRangeKey.SelectedKeyName, bInRange);
    }
    else
    {
        Blackboard->ClearValue(TargetActorKey.SelectedKeyName);
        Blackboard->SetValueAsBool(PlayerInRangeKey.SelectedKeyName, false);
    }
}
