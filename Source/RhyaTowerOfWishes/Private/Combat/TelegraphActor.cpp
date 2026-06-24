#include "Combat/TelegraphActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATelegraphActor::ATelegraphActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
    SetRootComponent(Disc);
    Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Disc->SetCastShadow(false);

    // Bake the intrinsic visual (a flat plane + the radial-fill material) here so every
    // instance renders without relying on Blueprint component-default serialization.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (PlaneMesh.Succeeded())
    {
        Disc->SetStaticMesh(PlaneMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TelegraphMat(TEXT("/Game/VFX/Telegraph/M_TelegraphCircle.M_TelegraphCircle"));
    if (TelegraphMat.Succeeded())
    {
        TelegraphMaterial = TelegraphMat.Object;
        Disc->SetMaterial(0, TelegraphMat.Object);
    }
}

void ATelegraphActor::BeginPlay()
{
    Super::BeginPlay();

    if (!ensureMsgf(TelegraphMaterial != nullptr, TEXT("%s has no TelegraphMaterial"), *GetName()))
    {
        return;
    }

    DiscMID = Disc->CreateDynamicMaterialInstance(0, TelegraphMaterial);
    if (ensureMsgf(DiscMID, TEXT("%s: failed to create the telegraph MID despite a valid material"), *GetName()))
    {
        DiscMID->SetScalarParameterValue(TEXT("Radius"), Radius);
        DiscMID->SetVectorParameterValue(TEXT("Color"), Color);
        DiscMID->SetScalarParameterValue(TEXT("Progress"), 0.f);
    }

    // Scale the disc so it physically covers the circle
    if (const UStaticMesh* Mesh = Disc->GetStaticMesh())
    {
        const double HalfExtent = Mesh->GetBounds().BoxExtent.X;
        if (HalfExtent > KINDA_SMALL_NUMBER)
        {
            const float S = static_cast<float>(Radius / HalfExtent);
            Disc->SetWorldScale3D(FVector(S, S, 1.f));
        }
    }
}

void ATelegraphActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bStruck)
    {
        return;
    }

    Elapsed += DeltaSeconds;
    const float Progress = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
    // Visual-only; the null case is reported loudly at creation, and skipping the fill never blocks the strike.
    if (DiscMID)
    {
        DiscMID->SetScalarParameterValue(TEXT("Progress"), Progress);
    }

    if (Elapsed >= Duration)
    {
        bStruck = true;

        // Missing StrikeClass = a telegraph that warns then does nothing; treat as misconfig (relax to a plain `if` for feints).
        if (ensureMsgf(StrikeClass, TEXT("%s: telegraph completed with no StrikeClass set"), *GetName()))
        {
            FActorSpawnParameters Params;
            Params.Owner = GetOwner();
            Params.Instigator = GetInstigator();
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            GetWorld()->SpawnActor<AActor>(StrikeClass, GetActorTransform(), Params);
        }

        Destroy();
    }
}
