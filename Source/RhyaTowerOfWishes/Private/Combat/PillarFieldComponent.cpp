#include "Combat/PillarFieldComponent.h"
#include "Combat/CombatUtils.h"
#include "Combat/TelegraphActor.h"
#include "Combat/StrikeActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Algo/RandomShuffle.h"
#include "HAL/IConsoleManager.h"

void UPillarFieldComponent::DoPillarAttack(int32 NumPillars)
{
    if (!ensureMsgf(TelegraphClass && StrikeClass, TEXT("%s: DoPillarAttack missing TelegraphClass/StrikeClass"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(PillarPoints.Num() > 0, TEXT("%s: DoPillarAttack with no PillarPoints authored"), *GetName()))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!ensureMsgf(World, TEXT("%s: DoPillarAttack on a component with no world"), *GetName()))
    {
        return;
    }

    TArray<FVector> Shuffled = PillarPoints;
    Algo::RandomShuffle(Shuffled);
    const int32 Count = FMath::Min(NumPillars, Shuffled.Num());

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector WorldPoint = GetComponentTransform().TransformPosition(Shuffled[Index]);

        FVector FloorPoint;
        if (!ensureMsgf(Rhya::SnapToFloor(*World, WorldPoint, GetOwner(), FloorPoint),
                TEXT("%s: pillar %d found no floor under %s"), *GetName(), Index, *WorldPoint.ToString()))
        {
            continue;
        }

        if (StaggerWindow > 0.f)
        {
            FTimerHandle Handle;
            World->GetTimerManager().SetTimer(
                Handle,
                FTimerDelegate::CreateUObject(this, &UPillarFieldComponent::SpawnTelegraphAt, FloorPoint),
                FMath::FRandRange(KINDA_SMALL_NUMBER, StaggerWindow),
                false);
        }
        else
        {
            SpawnTelegraphAt(FloorPoint);
        }
    }
}

void UPillarFieldComponent::SpawnTelegraphAt(FVector FloorPoint)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    ATelegraphActor* Telegraph = World->SpawnActorDeferred<ATelegraphActor>(
        TelegraphClass,
        FTransform(FloorPoint),
        Owner,
        Owner ? Owner->GetInstigator() : nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (ensureMsgf(Telegraph, TEXT("%s: failed to spawn TelegraphClass"), *GetName()))
    {
        Telegraph->Configure(Radius, TelegraphDuration, StrikeClass);
        Telegraph->FinishSpawning(FTransform(FloorPoint));
    }
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugPillarAttack(
    TEXT("Rhya.Debug.PillarAttack"),
    TEXT("Triggers DoPillarAttack on the first UPillarFieldComponent in the world. Arg: pillar count (default 3)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        World = Rhya::FindGameWorld(World);
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.PillarAttack: no running game world"));
            return;
        }

        UPillarFieldComponent* Component = Rhya::FindFirstComponent<UPillarFieldComponent>(World);
        if (!Component)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.PillarAttack: no UPillarFieldComponent in world"));
            return;
        }

        const int32 N = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 3;
        Component->DoPillarAttack(N);
    }),
    ECVF_Cheat);
