#include "Combat/ProjectileAttackComponent.h"
#include "Combat/CombatUtils.h"
#include "Combat/ProjectileStrike.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

void UProjectileAttackComponent::FireAt(FVector TargetLocation)
{
    if (!ensureMsgf(ProjectileClass, TEXT("%s: FireAt missing ProjectileClass"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(Speed > 0.f && MaxRange > 0.f, TEXT("%s: FireAt with non-positive Speed/MaxRange"), *GetName()))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!ensureMsgf(World, TEXT("%s: FireAt on a component with no world"), *GetName()))
    {
        return;
    }
    AActor* Owner = GetOwner();
    if (!ensureMsgf(Owner, TEXT("%s: FireAt on a component with no owner"), *GetName()))
    {
        return;
    }
    APawn* Instigator = Owner->GetInstigator();

    const FVector Muzzle = Owner->GetActorTransform().TransformPositionNoScale(MuzzleOffset);
    FVector Dir = TargetLocation - Muzzle;
    if (!ensureMsgf(Dir.Normalize(), TEXT("%s: FireAt target coincides with the muzzle"), *GetName()))
    {
        return;
    }

    // The transform's rotation is the travel direction; the projectile reads its forward vector for both movement and damage.
    AProjectileStrike::SpawnConfigured(World, ProjectileClass, FTransform(Dir.Rotation(), Muzzle), Speed, MaxRange, Owner, Instigator);
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugProjectileAttack(
    TEXT("Rhya.Debug.ProjectileAttack"),
    TEXT("Fires the first UProjectileAttackComponent in the world at the player pawn. Arg: speed override (default: the component's Speed)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        World = Rhya::FindGameWorld(World);
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.ProjectileAttack: no running game world"));
            return;
        }

        UProjectileAttackComponent* Component = Rhya::FindFirstComponent<UProjectileAttackComponent>(World);
        if (!Component || !Component->GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.ProjectileAttack: no owned UProjectileAttackComponent in world"));
            return;
        }

        APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
        if (!Player)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.ProjectileAttack: no player pawn"));
            return;
        }

        const float SavedSpeed = Component->Speed;
        if (Args.Num() > 0)
        {
            Component->Speed = FCString::Atof(*Args[0]);
        }
        Component->FireAt(Player->GetActorLocation());
        Component->Speed = SavedSpeed;
    }),
    ECVF_Cheat);
