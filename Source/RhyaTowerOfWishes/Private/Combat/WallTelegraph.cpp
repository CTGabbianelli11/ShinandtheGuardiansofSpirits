#include "Combat/WallTelegraph.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
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
    if (!ensureMsgf(InWidth > 0.f && InLength > 0.f && InDuration > 0.f, TEXT("%s: Configure with non-positive dimensions/duration"), *GetName()))
    {
        return;
    }
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
        Destroy();
        return;
    }

    Decal->SetDecalMaterial(TelegraphMaterial);
    DecalMID = Decal->CreateDynamicMaterialInstance();
    if (!ensureMsgf(DecalMID, TEXT("%s: failed to create the telegraph MID despite a valid material"), *GetName()))
    {
        Destroy();
        return;
    }
    DecalMID->SetScalarParameterValue(TEXT("Progress"), 0.f);
    DecalMID->SetScalarParameterValue(TEXT("Width"), Width);
    DecalMID->SetScalarParameterValue(TEXT("Length"), Length);
}

void AWallTelegraph::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    Elapsed += DeltaSeconds;
    DecalMID->SetScalarParameterValue(TEXT("Progress"), FMath::Clamp(Elapsed / Duration, 0.f, 1.f));

    if (Elapsed >= Duration)
    {
        Destroy();
    }
}

AWallTelegraph* AWallTelegraph::SpawnConfigured(UWorld* World, TSubclassOf<AWallTelegraph> Class, const FTransform& Transform, float InWidth, float InLength, float InDuration, AActor* Owner, APawn* Instigator)
{
    if (!ensureMsgf(World && Class, TEXT("AWallTelegraph::SpawnConfigured: missing World or Class")))
    {
        return nullptr;
    }

    AWallTelegraph* Telegraph = World->SpawnActorDeferred<AWallTelegraph>(Class, Transform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ensureMsgf(Telegraph, TEXT("AWallTelegraph::SpawnConfigured: failed to spawn %s"), *Class->GetName()))
    {
        return nullptr;
    }

    Telegraph->Configure(InWidth, InLength, InDuration);
    Telegraph->FinishSpawning(Transform);
    return Telegraph;
}
