#include "Combat/PillarStrike.h"
#include "Components/StaticMeshComponent.h"

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
    DealRadialDamage(Damage);
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
