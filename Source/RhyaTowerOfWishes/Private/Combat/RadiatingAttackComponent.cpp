#include "Combat/RadiatingAttackComponent.h"
#include "Combat/CombatUtils.h"
#include "Combat/TelegraphActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

URadiatingAttackComponent::URadiatingAttackComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool URadiatingAttackComponent::StartRadiatingAttack(FVector Center, int32 RingCount)
{
    CancelRadiatingAttack();
    const auto Positive = [](float Value) { return FMath::IsFinite(Value) && Value > 0.f; };
    const FRadiatingAttackSettings& S = AttackSettings;
    if (!GetWorld() || !IsValid(GetOwner()) || Center.ContainsNaN() || RingCount <= 0
        || !Positive(S.RingWidth) || !Positive(S.RingInterval) || !Positive(S.TelegraphDuration)
        || !Positive(S.ImpactLifetime) || !Positive(S.DamageHalfHeight)
        || !FMath::IsFinite(S.Damage) || S.Damage < 0.f || !S.TelegraphMaterial
        || !FMath::IsFinite(S.RingWidth * static_cast<float>(RingCount)))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: invalid radiating attack settings (a ring material is required)"), *GetName());
        return false;
    }
    if (!Rhya::SnapToFloor(*GetWorld(), Center, GetOwner(), AttackCenter))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: no floor under radiating attack center"), *GetName());
        return false;
    }

    ActiveSettings = AttackSettings;
    TotalRings = RingCount;
    NextRing = 0;
    SpawnNextRing();
    if (NextRing < TotalRings)
    {
        GetWorld()->GetTimerManager().SetTimer(RingTimer, this,
            &URadiatingAttackComponent::SpawnNextRing, ActiveSettings.RingInterval, true);
    }
    return TotalRings > 0;
}

void URadiatingAttackComponent::SpawnNextRing()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || Owner->IsActorBeingDestroyed() || NextRing >= TotalRings)
    {
        CancelRadiatingAttack();
        return;
    }

    const FTransform Transform(AttackCenter);
    APawn* Instigator = Cast<APawn>(Owner);
    if (!Instigator) Instigator = Owner->GetInstigator();
    ATelegraphActor* Warning = GetWorld()->SpawnActorDeferred<ATelegraphActor>(
        ATelegraphActor::StaticClass(), Transform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Warning)
    {
        CancelRadiatingAttack();
        return;
    }
    Warning->ConfigureRadiatingPulse(ActiveSettings,
        static_cast<float>(NextRing) * ActiveSettings.RingWidth,
        static_cast<float>(NextRing + 1) * ActiveSettings.RingWidth);
    PendingWarnings.RemoveAll([](const TWeakObjectPtr<ATelegraphActor>& Item) { return !Item.IsValid(); });
    PendingWarnings.Add(Warning);
    Warning->FinishSpawning(Transform);
    ++NextRing;
    if (NextRing >= TotalRings)
    {
        GetWorld()->GetTimerManager().ClearTimer(RingTimer);
    }
}

void URadiatingAttackComponent::CancelRadiatingAttack()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RingTimer);
    for (const TWeakObjectPtr<ATelegraphActor>& Warning : PendingWarnings)
    {
        if (Warning.IsValid()) Warning->Destroy();
    }
    PendingWarnings.Reset();
    TotalRings = 0;
    NextRing = 0;
}

void URadiatingAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelRadiatingAttack();
    Super::EndPlay(EndPlayReason);
}
