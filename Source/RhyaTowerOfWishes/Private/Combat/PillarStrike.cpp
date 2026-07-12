#include "Combat/PillarStrike.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

APillarStrike::APillarStrike()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
    // Damage is a one-shot query in ApplyEmergenceDamage; the mesh never touches the physics scene.
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APillarStrike::BeginPlay()
{
    Super::BeginPlay();

    Mesh->SetRelativeLocation(FVector(0.f, 0.f, -RiseDistance));
    ApplyEmergenceDamage();
}

void APillarStrike::ApplyEmergenceDamage()
{
    UWorld* World = GetWorld();

    FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PillarEmergence), false, this);
    if (GetOwner())
    {
        QueryParams.AddIgnoredActor(GetOwner());
    }

    TArray<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), QueryParams);

    AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;

    TSet<AActor*> Damaged;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* HitActor = Overlap.GetActor();
        if (HitActor && !Damaged.Contains(HitActor))
        {
            Damaged.Add(HitActor);
            UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController, this, nullptr);
        }
    }
}

void APillarStrike::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    Elapsed += DeltaSeconds;

    switch (Phase)
    {
    case EPhase::Rising:
    {
        const float Alpha = FMath::Clamp(Elapsed / RiseTime, 0.f, 1.f);
        const float Z = FMath::Lerp(-RiseDistance, 0.f, Alpha);
        Mesh->SetRelativeLocation(FVector(0.f, 0.f, Z));
        if (Elapsed >= RiseTime)
        {
            Phase = EPhase::Holding;
            Elapsed = 0.f;
        }
        break;
    }
    case EPhase::Holding:
    {
        if (Elapsed >= HoldTime)
        {
            Phase = EPhase::Sinking;
            Elapsed = 0.f;
        }
        break;
    }
    case EPhase::Sinking:
    {
        const float Alpha = FMath::Clamp(Elapsed / SinkTime, 0.f, 1.f);
        const float Z = FMath::Lerp(0.f, -RiseDistance, Alpha);
        Mesh->SetRelativeLocation(FVector(0.f, 0.f, Z));
        if (Elapsed >= SinkTime)
        {
            Destroy();
        }
        break;
    }
    }
}
