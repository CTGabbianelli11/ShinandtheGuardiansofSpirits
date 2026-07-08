#include "Combat/TelegraphActor.h"
#include "Combat/StrikeActor.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATelegraphActor::ATelegraphActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
    SetRootComponent(Decal);

    // A deferred decal projects its material along the component's -X axis. Pitch it so -X aims
    // straight down (world -Z) 
    Decal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

    // Bake the intrinsic material here so every instance renders without relying on Blueprint
    // component-default serialization (a Substrate deferred-decal; see M_AoE_Telegraph).
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TelegraphMat(TEXT("/Game/Blueprints/Characters/M_AoE_Telegraph.M_AoE_Telegraph"));
    if (TelegraphMat.Succeeded())
    {
        TelegraphMaterial = TelegraphMat.Object;
        Decal->SetDecalMaterial(TelegraphMat.Object);
    }
}

void ATelegraphActor::Configure(float InRadius, float InDuration, TSubclassOf<AStrikeActor> InStrikeClass)
{
    Radius = InRadius;
    Duration = InDuration;
    StrikeClass = InStrikeClass;
}

void ATelegraphActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Sizing from Radius belongs here, not BeginPlay: OnConstruction runs during FinishSpawning
    // (after Configure() sets Radius, before BeginPlay) AND whenever a placed instance is edited
    // in the editor, so the decal previews correctly either way.
    Decal->DecalSize = FVector(Radius, Radius, Radius);
    Decal->MarkRenderStateDirty();
}

void ATelegraphActor::BeginPlay()
{
    Super::BeginPlay();

    if (!ensureMsgf(TelegraphMaterial != nullptr, TEXT("%s has no TelegraphMaterial"), *GetName()))
    {
        return;
    }

    Decal->SetDecalMaterial(TelegraphMaterial);
    DecalMID = Decal->CreateDynamicMaterialInstance();
    if (ensureMsgf(DecalMID, TEXT("%s: failed to create the telegraph MID despite a valid material"), *GetName()))
    {
        DecalMID->SetScalarParameterValue(TEXT("Progress"), 0.f);
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
    if (DecalMID)
    {
        DecalMID->SetScalarParameterValue(TEXT("Progress"), Progress);
    }

    if (Elapsed >= Duration)
    {
        bStruck = true;

        // Missing StrikeClass = a telegraph that warns then does nothing; treat as misconfig (relax to a plain `if` for feints).
        if (ensureMsgf(StrikeClass, TEXT("%s: telegraph completed with no StrikeClass set"), *GetName()))
        {
            AStrikeActor::SpawnConfigured(GetWorld(), StrikeClass, GetActorTransform(), Radius, GetOwner(), GetInstigator());
        }

        Destroy();
    }
}
