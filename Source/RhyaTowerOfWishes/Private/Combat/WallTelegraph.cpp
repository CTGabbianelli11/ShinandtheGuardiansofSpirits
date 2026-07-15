#include "Combat/WallTelegraph.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AWallTelegraph::AWallTelegraph()
{
    PrimaryActorTick.bCanEverTick = true;

    Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
    SetRootComponent(Decal);
    Decal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void AWallTelegraph::Configure(float InWidth, float InLength, float InDuration)
{
    Width = InWidth;
    Length = InLength;
    Duration = InDuration;
}

void AWallTelegraph::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Sizing belongs here, not BeginPlay: OnConstruction runs during FinishSpawning (after
    // Configure) AND whenever a placed instance is edited, so the decal previews correctly.
    Decal->DecalSize = FVector(ProjectionDepth, Width * 0.5f, Length * 0.5f);
    Decal->MarkRenderStateDirty();
}

void AWallTelegraph::BeginPlay()
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
        DecalMID->SetScalarParameterValue(TEXT("Width"), Width);
        DecalMID->SetScalarParameterValue(TEXT("Length"), Length);
    }
}

void AWallTelegraph::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    Elapsed += DeltaSeconds;
    const float Progress = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

    if (DecalMID)
    {
        DecalMID->SetScalarParameterValue(TEXT("Progress"), Progress);
    }

    if (Elapsed >= Duration)
    {
        Destroy();
    }
}
