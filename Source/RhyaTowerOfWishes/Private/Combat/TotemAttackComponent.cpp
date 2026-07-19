#include "Combat/TotemAttackComponent.h"
#include "Combat/CombatUtils.h"
#include "Combat/Totem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"

void UTotemAttackComponent::DoTotemAttack()
{
    if (!ensureMsgf(TotemClass, TEXT("%s: DoTotemAttack missing TotemClass"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(TotemPoints.Num() > 0, TEXT("%s: DoTotemAttack with no TotemPoints authored"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(!bPhaseActive, TEXT("%s: DoTotemAttack while a phase is active - CancelTotemPhase() first"), *GetName()))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!ensureMsgf(World, TEXT("%s: DoTotemAttack on a component with no world"), *GetName()))
    {
        return;
    }

    for (int32 Index = 0; Index < TotemPoints.Num(); ++Index)
    {
        const FVector WorldPoint = GetComponentTransform().TransformPosition(TotemPoints[Index]);

        FVector FloorPoint;
        if (!ensureMsgf(Rhya::SnapToFloor(*World, WorldPoint, GetOwner(), FloorPoint),
                TEXT("%s: totem %d found no floor under %s"), *GetName(), Index, *WorldPoint.ToString()))
        {
            continue;
        }

        AActor* Owner = GetOwner();
        ATotem* Totem = ATotem::SpawnConfigured(World, TotemClass, FTransform(FloorPoint), Owner, Owner ? Owner->GetInstigator() : nullptr);
        if (Totem)
        {
            RegisterTotem(Totem);
        }
    }

    if (!ensureMsgf(AliveTotems.Num() > 0, TEXT("%s: DoTotemAttack spawned no totems - phase not started"), *GetName()))
    {
        return;
    }

    bPhaseActive = true;
    World->GetTimerManager().SetTimer(DeadlineHandle, this, &UTotemAttackComponent::HandleDeadlineExpired, TimeLimit, false);
    OnPhaseStarted.Broadcast(TimeLimit);
}

void UTotemAttackComponent::CancelTotemPhase()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeadlineHandle);
    }
    bPhaseActive = false;

    // Empty the roster first and unbind before destroying: cancel is a silent teardown, and
    // HandleTotemDestroyed must not treat it as kills.
    TArray<ATotem*> Doomed = MoveTemp(AliveTotems);
    for (ATotem* Totem : Doomed)
    {
        if (Totem)
        {
            Totem->OnDestroyed.RemoveDynamic(this, &UTotemAttackComponent::HandleTotemDestroyed);
            Totem->Destroy();
        }
    }
}

void UTotemAttackComponent::BeginPlay()
{
    Super::BeginPlay();

    // Lifetime-scoped, phase-independent: the readout exists as long as the component does.
    GetWorld()->GetTimerManager().SetTimer(StateDrawHandle, this, &UTotemAttackComponent::DrawDebugState, 1.f, true, 0.f);
}

void UTotemAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (EndPlayReason == EEndPlayReason::Destroyed)
    {
        // Owner died mid-phase: take the totems along so no orphaned objectives linger.
        CancelTotemPhase();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeadlineHandle);
        World->GetTimerManager().ClearTimer(StateDrawHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void UTotemAttackComponent::HandleTotemDestroyed(AActor* DestroyedActor)
{
    // Runs even after the phase resolves (stragglers killed post-expiry) so the roster stays truthful.
    AliveTotems.Remove(Cast<ATotem>(DestroyedActor));

    if (!bPhaseActive || AliveTotems.Num() > 0)
    {
        return;
    }

    FTimerManager& Timers = GetWorld()->GetTimerManager();
    const float Remaining = Timers.GetTimerRemaining(DeadlineHandle);
    Timers.ClearTimer(DeadlineHandle);
    bPhaseActive = false;
    OnPhaseCleared.Broadcast(Remaining);
}

void UTotemAttackComponent::HandleDeadlineExpired()
{
    bPhaseActive = false;
    OnPhaseExpired.Broadcast(AliveTotems.Num());
}

void UTotemAttackComponent::RegisterTotem(ATotem* Totem)
{
    Totem->OnDestroyed.AddDynamic(this, &UTotemAttackComponent::HandleTotemDestroyed);
    AliveTotems.Add(Totem);
}

// Immediate-mode readout: derived fresh from live state on every redraw, so unlike an
// event-driven display it cannot desync from the component - a stuck bPhaseActive or stale
// roster becomes visible in-world. Poll to display, never to decide.
void UTotemAttackComponent::DrawDebugState()
{
    static IConsoleVariable* const CombatDebugCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Rhya.Debug.Combat"));
    if (!ensureMsgf(CombatDebugCVar, TEXT("Rhya.Debug.Combat CVar not registered")))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World || CombatDebugCVar->GetInt() == 0)
    {
        return;
    }

    FString Line;
    FColor Color;
    if (bPhaseActive)
    {
        const int32 Seconds = FMath::CeilToInt32(World->GetTimerManager().GetTimerRemaining(DeadlineHandle));
        Line = FString::Printf(TEXT("TOTEMS %d/%d - %ds"), AliveTotems.Num(), TotemPoints.Num(), Seconds);
        Color = FColor::Orange;
    }
    else if (AliveTotems.Num() > 0)
    {
        Line = FString::Printf(TEXT("TOTEMS idle - %d standing"), AliveTotems.Num());
        Color = FColor::Silver;
    }
    else
    {
        Line = TEXT("TOTEMS idle");
        Color = FColor::Silver;
    }

    // Lifetime slightly over the redraw interval so the text never blinks out between draws.
    // Null base actor + absolute location: the TestBaseActor form of DrawDebugString has never
    // rendered in this project, while this form (see DrawCombatText) provably does.
    DrawDebugString(World, GetComponentLocation() + FVector(0.f, 0.f, 140.f), Line, nullptr, Color, 1.05f, true, 2.f);
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugTotemAttack(
    TEXT("Rhya.Debug.TotemAttack"),
    TEXT("Triggers DoTotemAttack on the first UTotemAttackComponent in the world. Arg: time limit override in seconds."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        World = Rhya::FindGameWorld(World);
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.TotemAttack: no running game world"));
            return;
        }

        UTotemAttackComponent* Component = Rhya::FindFirstComponent<UTotemAttackComponent>(World);
        if (!Component)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.TotemAttack: no UTotemAttackComponent in world"));
            return;
        }

        if (Args.Num() > 0)
        {
            Component->TimeLimit = FCString::Atof(*Args[0]);
        }
        Component->DoTotemAttack();
    }),
    ECVF_Cheat);
