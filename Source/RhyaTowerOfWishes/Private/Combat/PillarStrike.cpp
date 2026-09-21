#include "Combat/PillarStrike.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

APillarStrike::APillarStrike()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
    // Damage is a one-shot query in BeginPlay; the mesh never touches the physics scene.
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APillarStrike::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    const UStaticMesh* StaticMesh = Mesh->GetStaticMesh();
    if (!StaticMesh)
    {
        return; 
    }

    // infer the mesh scale from radius.
    const FVector Extent = StaticMesh->GetBounds().BoxExtent;
    const double MeshRadius = FMath::Max(Extent.X, Extent.Y);
    if (!ensureAlwaysMsgf(MeshRadius > UE_SMALL_NUMBER,
        TEXT("%s: pillar mesh %s must have nonzero horizontal bounds."),
        *GetPathName(), *StaticMesh->GetPathName()))
    {
        return;
    }

    const double HorizontalScale = Radius / MeshRadius;
    Mesh->SetRelativeScale3D(FVector(HorizontalScale, HorizontalScale, HeightScale));
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
