#include "Combat/WallAttackComponent.h"
#include "Combat/CombatUtils.h"
#include "Combat/WallStrike.h"
#include "Combat/WallTelegraph.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

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
    if (!ensureMsgf(SlideSpeed > 0.f && TelegraphDuration > 0.f, TEXT("%s: DoWallAttack with non-positive SlideSpeed/TelegraphDuration"), *GetName()))
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
    if (!ensureMsgf(World, TEXT("%s: DoWallAttack on a component with no world"), *GetName()))
    {
        return;
    }
    AActor* Owner = GetOwner();
    APawn* Instigator = Owner ? Owner->GetInstigator() : nullptr;

    FVector FloorPoint;
    if (!ensureMsgf(Rhya::SnapToFloor(*World, Start, Owner, FloorPoint), TEXT("%s: DoWallAttack found no floor under %s"), *GetName(), *Start.ToString()))
    {
        return;
    }
    const FRotator Rot = Dir.Rotation();

    AWallStrike* Wall = AWallStrike::SpawnConfigured(World, WallClass, FTransform(Rot, FloorPoint), TelegraphDuration, SlideSpeed, Distance, Owner, Instigator);
    if (!Wall)
    {
        return;
    }

    // The danger rect is the area the wall BODY covers
    const FVector Extent = Wall->GetCollisionExtent();
    AWallTelegraph* Telegraph = AWallTelegraph::SpawnConfigured(
        World,
        TelegraphClass,
        FTransform(Rot, FloorPoint + Dir * (Distance * 0.5f)),
        static_cast<float>(Extent.Y) * 2.f,
        Distance + static_cast<float>(Extent.X) * 2.f,
        TelegraphDuration,
        Owner,
        Instigator);
    if (!Telegraph)
    {
        // No warning means no attack.
        Wall->Destroy();
    }
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugWallAttack(
    TEXT("Rhya.Debug.WallAttack"),
    TEXT("Triggers DoWallAttack on the first UWallAttackComponent in the world, sliding forward from its owner. Arg: distance (default 1500)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        World = Rhya::FindGameWorld(World);
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.WallAttack: no running game world"));
            return;
        }

        UWallAttackComponent* Component = Rhya::FindFirstComponent<UWallAttackComponent>(World);
        if (!Component || !Component->GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.WallAttack: no owned UWallAttackComponent in world"));
            return;
        }

        const float Dist = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1500.f;
        AActor* Owner = Component->GetOwner();
        const FVector Fwd = Owner->GetActorForwardVector();
        Component->DoWallAttack(Owner->GetActorLocation() + Fwd * 150.f, Fwd, Dist);
    }),
    ECVF_Cheat);
