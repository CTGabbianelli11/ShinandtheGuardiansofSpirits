#include "Combat/PillarFieldComponent.h"
#include "Combat/TelegraphActor.h"
#include "Combat/StrikeActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Algo/RandomShuffle.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

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

    TArray<FVector> Shuffled = PillarPoints;
    Algo::RandomShuffle(Shuffled);
    const int32 Count = FMath::Min(NumPillars, Shuffled.Num());

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector WorldPoint = GetComponentTransform().TransformPosition(Shuffled[Index]);

        FHitResult Hit;
        const FVector Start = WorldPoint + FVector(0.f, 0.f, 500.f);
        const FVector End = WorldPoint - FVector(0.f, 0.f, 1000.f);
        if (!ensureMsgf(
                World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(PillarFloorSnap), false, GetOwner())),
                TEXT("%s: pillar %d found no floor under %s"), *GetName(), Index, *WorldPoint.ToString()))
        {
            continue;
        }

        // ImpactPoint is FVector_NetQuantize; take a plain FVector so the timer-delegate payload
        // type matches SpawnTelegraphAt's FVector parameter.
        const FVector FloorPoint(Hit.ImpactPoint);
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
        if (!World || !World->IsGameWorld())
        {
            World = nullptr;
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.World() && Context.World()->IsGameWorld())
                {
                    World = Context.World();
                    break;
                }
            }
        }
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.PillarAttack: no running game world"));
            return;
        }

        const int32 N = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 3;
        for (TObjectIterator<UPillarFieldComponent> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsRegistered())
            {
                It->DoPillarAttack(N);
                return;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.PillarAttack: no UPillarFieldComponent in world"));
    }),
    ECVF_Cheat);
