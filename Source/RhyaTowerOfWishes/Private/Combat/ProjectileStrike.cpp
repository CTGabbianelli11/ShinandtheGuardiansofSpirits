#include "Combat/ProjectileStrike.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
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
    // Overlap pawns (damage), block world statics (stop and destroy on level geometry).
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
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

    // The overlap handler may have spent us mid-sweep; don't also destroy on the wall behind the pawn.
    if (bSpent)
    {
        return;
    }
    if (Hit.bBlockingHit)
    {
        OnFlightEnded();
        return;
    }
    if (Traveled >= MaxRange)
    {
        OnFlightEnded();
    }
}

void AProjectileStrike::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bSpent || !OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        return;
    }
    bSpent = true;
    OnPawnHit(OtherActor, SweepResult);
}

void AProjectileStrike::OnPawnHit(AActor* OtherActor, const FHitResult& SweepResult)
{
    AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
    // Point damage carries the flight direction, so block verdicts face the projectile, not its firer.
    UGameplayStatics::ApplyPointDamage(OtherActor, Damage, GetActorForwardVector(), SweepResult, InstigatorController, this, nullptr);
    Destroy();
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
