#include "Combat/HomingAttackComponent.h"
#include "Combat/CombatUtils.h"
#include "Combat/HomingStrike.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

void UHomingAttackComponent::FireAt(AActor* Target)
{
    if (!ensureMsgf(ProjectileClass, TEXT("%s: FireAt missing ProjectileClass"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(Target, TEXT("%s: FireAt with no target"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(Speed > 0.f && LifeSeconds > 0.f && Count > 0, TEXT("%s: FireAt with non-positive Speed/LifeSeconds/Count"), *GetName()))
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
    FVector Aim = Target->GetActorLocation() - Muzzle;
    if (!ensureMsgf(Aim.Normalize(), TEXT("%s: FireAt target coincides with the muzzle"), *GetName()))
    {
        return;
    }

    const FRotator AimRotation = Aim.Rotation();
    const float SpacingDeg = Count > 1 ? FanSpreadDegrees / (Count - 1) : 0.f;
    const float StartDeg = Count > 1 ? -FanSpreadDegrees * 0.5f : 0.f;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        FRotator Heading = AimRotation;
        Heading.Yaw += StartDeg + SpacingDeg * Index;
        AHomingStrike::SpawnConfigured(World, ProjectileClass, FTransform(Heading, Muzzle), Speed, LifeSeconds, TurnRate, Target, Owner, Instigator);
    }
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugHomingAttack(
    TEXT("Rhya.Debug.HomingAttack"),
    TEXT("Fires the first UHomingAttackComponent in the world at the player pawn. Arg: turn rate override in deg/s (default: the component's TurnRate)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        World = Rhya::FindGameWorld(World);
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.HomingAttack: no running game world"));
            return;
        }

        UHomingAttackComponent* Component = Rhya::FindFirstComponent<UHomingAttackComponent>(World);
        if (!Component || !Component->GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.HomingAttack: no owned UHomingAttackComponent in world"));
            return;
        }

        APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
        if (!Player)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.HomingAttack: no player pawn"));
            return;
        }

        const float SavedTurnRate = Component->TurnRate;
        if (Args.Num() > 0)
        {
            Component->TurnRate = FCString::Atof(*Args[0]);
        }
        Component->FireAt(Player);
        Component->TurnRate = SavedTurnRate;
    }),
    ECVF_Cheat);
