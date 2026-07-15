#include "Combat/WallAttackComponent.h"
#include "Combat/WallStrike.h"
#include "Combat/WallTelegraph.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

void UWallAttackComponent::DoWallAttack(FVector Start, FVector Direction, float Distance)
{
    if (!ensureMsgf(TelegraphClass && WallClass, TEXT("%s: DoWallAttack missing TelegraphClass/WallClass"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(Distance > 0.f, TEXT("%s: DoWallAttack with non-positive Distance"), *GetName()))
    {
        return;
    }

    FVector Dir = Direction;
    Dir.Z = 0.f;
    if (!ensureMsgf(Dir.Normalize(), TEXT("%s: DoWallAttack direction is vertical or zero"), *GetName()))
    {
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    APawn* Instigator = Owner ? Owner->GetInstigator() : nullptr;

    FHitResult Hit;
    const FVector TraceStart = Start + FVector(0.f, 0.f, 500.f);
    const FVector TraceEnd = Start - FVector(0.f, 0.f, 1000.f);
    if (!ensureMsgf(
            World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(WallFloorSnap), false, Owner)),
            TEXT("%s: DoWallAttack found no floor under %s"), *GetName(), *Start.ToString()))
    {
        return;
    }
    const FVector FloorPoint(Hit.ImpactPoint);
    const FRotator Rot = Dir.Rotation();

    AWallStrike* Wall = AWallStrike::SpawnConfigured(World, WallClass, FTransform(Rot, FloorPoint), TelegraphDuration, SlideSpeed, Distance, Owner, Instigator);
    if (!Wall)
    {
        return;
    }

    // The wall's box is centered on its root, so lift it by a half-height to rest on the floor.
    const FVector Extent = Wall->GetCollisionExtent();
    Wall->AddActorWorldOffset(FVector(0.f, 0.f, Extent.Z));

    // The danger rect is the area the wall BODY covers
    AWallTelegraph* Telegraph = World->SpawnActorDeferred<AWallTelegraph>(
        TelegraphClass,
        FTransform(Rot, FloorPoint + Dir * (Distance * 0.5f)),
        Owner,
        Instigator,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (ensureMsgf(Telegraph, TEXT("%s: failed to spawn TelegraphClass"), *GetName()))
    {
        Telegraph->Configure(static_cast<float>(Extent.Y) * 2.f, Distance + static_cast<float>(Extent.X) * 2.f, TelegraphDuration);
        Telegraph->FinishSpawning(FTransform(Rot, FloorPoint + Dir * (Distance * 0.5f)));
    }
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugWallAttack(
    TEXT("Rhya.Debug.WallAttack"),
    TEXT("Triggers DoWallAttack on the first UWallAttackComponent in the world, sliding forward from its owner. Arg: distance (default 1500)."),
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
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.WallAttack: no running game world"));
            return;
        }

        const float Dist = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1500.f;
        for (TObjectIterator<UWallAttackComponent> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsRegistered())
            {
                AActor* Owner = It->GetOwner();
                const FVector Fwd = Owner->GetActorForwardVector();
                It->DoWallAttack(Owner->GetActorLocation() + Fwd * 150.f, Fwd, Dist);
                return;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.WallAttack: no UWallAttackComponent in world"));
    }),
    ECVF_Cheat);
