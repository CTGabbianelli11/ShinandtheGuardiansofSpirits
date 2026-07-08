#include "Components/AC_LobbedProjectile.h"
#include "Combat/TelegraphActor.h"
#include "Combat/StrikeActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"

UAC_LobbedProjectile::UAC_LobbedProjectile()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAC_LobbedProjectile::ThrowAt(FVector TargetLocation)
{
    AActor* Owner = GetOwner();
    if (!ensureMsgf(Owner, TEXT("%s: ThrowAt called with no owner"), *GetName()))
    {
        return;
    }
    if (!ensureMsgf(ProjectileClass && TelegraphClass && StrikeClass,
        TEXT("%s: ThrowAt missing ProjectileClass/TelegraphClass/StrikeClass"), *GetName()))
    {
        return;
    }

    UWorld* World = GetWorld();
    const FVector LaunchPoint = Owner->GetActorTransform().TransformPositionNoScale(LaunchOffset);

    // Telegraph: spawn deferred so Configure() lands before BeginPlay sizes the decal off Radius.
    ATelegraphActor* Telegraph = World->SpawnActorDeferred<ATelegraphActor>(TelegraphClass, FTransform(TargetLocation));
    if (ensureMsgf(Telegraph, TEXT("%s: failed to spawn TelegraphClass"), *GetName()))
    {
        Telegraph->Configure(Radius, FlightTime, StrikeClass);
        Telegraph->FinishSpawning(FTransform(TargetLocation));
    }

    // Projectile: cosmetic only. Its velocity is solved so it arrives at TargetLocation at
    // t = FlightTime, matching the telegraph's wind-up exactly.
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Owner->GetInstigator();
    AActor* Projectile = World->SpawnActor<AActor>(ProjectileClass, FTransform(LaunchPoint), SpawnParams);
    if (!ensureMsgf(Projectile, TEXT("%s: failed to spawn ProjectileClass"), *GetName()))
    {
        return;
    }

    UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
    if (ensureMsgf(Movement, TEXT("%s: %s has no UProjectileMovementComponent"), *GetName(), *ProjectileClass->GetName()))
    {
        // GetGravityZ() already folds in ProjectileGravityScale, so this can never drift out of
        // sync with how the component actually falls.
        const FVector Gravity(0.f, 0.f, Movement->GetGravityZ());
        Movement->Velocity = (TargetLocation - LaunchPoint) / FlightTime - 0.5f * Gravity * FlightTime;
    }

    Projectile->SetLifeSpan(FlightTime);
}
