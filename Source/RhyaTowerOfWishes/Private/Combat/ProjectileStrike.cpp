#include "Combat/ProjectileStrike.h"
#include "Combat/CollisionChannels.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/HitInterface.h"
#include "Kismet/GameplayStatics.h"

AProjectileStrike::AProjectileStrike()
{
    PrimaryActorTick.bCanEverTick = true;

    // The sphere must be the root: a swept move only sweeps the root component; children teleport.
    Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    SetRootComponent(Sphere);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    // Overlap pawns and damage blocking world-static bodies (including enemy meshes); overlap
    // swinging weapon hitboxes (deflection hook).
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Sphere->SetCollisionResponseToChannel(ECC_WeaponHitbox, ECR_Overlap);
    Sphere->SetGenerateOverlapEvents(true);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Sphere);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileStrike::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    Sphere->SetSphereRadius(SphereRadius);
}

void AProjectileStrike::BeginPlay()
{
    Super::BeginPlay();

    Sphere->OnComponentBeginOverlap.AddDynamic(this, &AProjectileStrike::OnSphereBeginOverlap);
    Sphere->OnComponentHit.AddDynamic(this, &AProjectileStrike::OnSphereHit);

    // Re-assert after serialized data lands: BPs saved before the WeaponHitbox channel existed
    // carry a response array without it, which silently overrides the constructor's Overlap.
    Sphere->SetCollisionResponseToChannel(ECC_WeaponHitbox, ECR_Overlap);

    // The firer sits at the muzzle; never let the projectile's sweep catch on its own owner.
    if (AActor* OwnerActor = GetOwner())
    {
        Sphere->IgnoreActorWhenMoving(OwnerActor, true);
    }
    if (APawn* Inst = GetInstigator())
    {
        Sphere->IgnoreActorWhenMoving(Inst, true);
    }

    // Leak guard in case the range check never fires (e.g. a stalled tick).
    SetLifeSpan(MaxRange / Speed + 1.f);
}

void AProjectileStrike::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bSpent)
    {
        return;
    }

    const float Step = FMath::Min(Speed * DeltaSeconds, MaxRange - Traveled);
    // bSweep=true so a pawn (or wall) crossed between ticks still registers at high speed.
    FHitResult Hit;
    AddActorWorldOffset(GetActorForwardVector() * Step, true, &Hit);
    Traveled += Step;

    // Either collision callback may have spent us during the sweep.
    if (bSpent)
    {
        return;
    }
    if (Hit.bBlockingHit)
    {
        HandleImpact(Hit.GetActor(), Hit);
        return;
    }
    if (Traveled >= MaxRange)
    {
        EndFlight();
    }
}

void AProjectileStrike::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bSpent || !OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        return;
    }
    // Route by the toucher's object type: a weapon hitbox is a deflection, anything else a hit.
    // Weapon touches don't spend the projectile - a deflected shot keeps flying.
    if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WeaponHitbox)
    {
        OnWeaponHit(OtherActor, SweepResult);
        return;
    }
    HandleImpact(OtherActor, SweepResult);
}

void AProjectileStrike::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    HandleImpact(OtherActor, Hit);
}

void AProjectileStrike::HandleImpact(AActor* OtherActor, const FHitResult& Hit)
{
    if (bSpent)
    {
        return;
    }
    if (!IsValid(OtherActor))
    {
        // A blocking obstruction still ends the flight without a valid damage recipient.
        if (Hit.bBlockingHit)
        {
            EndFlight();
        }
        return;
    }
    if (OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        return;
    }
    bSpent = true;
    OnImpact(OtherActor, Hit);
}

void AProjectileStrike::OnWeaponHit(AActor* WeaponActor, const FHitResult& SweepResult)
{
}

void AProjectileStrike::RestartFlight()
{
    Traveled = 0.f;
    SetLifeSpan(MaxRange / Speed + 1.f);
}

void AProjectileStrike::OnImpact(AActor* OtherActor, const FHitResult& SweepResult)
{
    AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
    // Point damage carries the flight direction, so block verdicts face the projectile, not its firer.
    UGameplayStatics::ApplyPointDamage(OtherActor, Damage, GetActorForwardVector(), SweepResult, InstigatorController, this, nullptr);
    // Resolve damage before the reaction, including fully blocked hits.
    if (IsValid(OtherActor) && !OtherActor->IsActorBeingDestroyed() && OtherActor->Implements<UHitInterface>())
    {
        const bool bHasSweepHit = SweepResult.GetComponent() != nullptr;
        const FVector ImpactPoint = bHasSweepHit ? FVector(SweepResult.ImpactPoint) : OtherActor->GetActorLocation();
        const FVector ImpactNormal = bHasSweepHit && !SweepResult.ImpactNormal.IsNearlyZero()
            ? FVector(SweepResult.ImpactNormal) : -GetActorForwardVector();
        IHitInterface::Execute_GetHit(OtherActor, ImpactPoint, ImpactNormal, Damage);
    }
    Destroy();
}

void AProjectileStrike::EndFlight()
{
    if (bSpent)
    {
        return;
    }

    bSpent = true;
    OnFlightEnded();
}

void AProjectileStrike::OnFlightEnded()
{
    Destroy();
}

AProjectileStrike* AProjectileStrike::SpawnConfigured(UWorld* World, TSubclassOf<AProjectileStrike> Class, const FTransform& Transform, float InSpeed, float InMaxRange, AActor* Owner, APawn* Instigator)
{
    if (!ensureMsgf(World && Class, TEXT("AProjectileStrike::SpawnConfigured: missing World or Class")))
    {
        return nullptr;
    }
    if (!ensureMsgf(InSpeed > 0.f && InMaxRange > 0.f, TEXT("AProjectileStrike::SpawnConfigured: non-positive flight parameters")))
    {
        return nullptr;
    }

    AProjectileStrike* Projectile = World->SpawnActorDeferred<AProjectileStrike>(Class, Transform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ensureMsgf(Projectile, TEXT("AProjectileStrike::SpawnConfigured: failed to spawn %s"), *Class->GetName()))
    {
        return nullptr;
    }

    Projectile->Speed = InSpeed;
    Projectile->MaxRange = InMaxRange;
    Projectile->FinishSpawning(Transform);
    return Projectile;
}
