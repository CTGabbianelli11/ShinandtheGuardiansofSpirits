#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WallTelegraph.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Ground telegraph for a wall sweep: an oriented rectangular decal covering the area the wall
 * will slide across (actor forward = slide direction). The material fills along the slide axis
 * over Duration, then the actor destroys itself. Unlike ATelegraphActor it never spawns a
 * strike - the wall pre-exists in a dormant state and owns its own wind-up timing.
 */
UCLASS()
class RHYATOWEROFWISHES_API AWallTelegraph : public AActor
{
    GENERATED_BODY()

public:
    AWallTelegraph();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Telegraph")
    void Configure(float InWidth, float InLength, float InDuration);

    // Spawns Class at Transform with the danger rect already configured; the telegraph
    // counterpart of AWallStrike::SpawnConfigured. Returns nullptr (after an ensure) on failure.
    static AWallTelegraph* SpawnConfigured(UWorld* World, TSubclassOf<AWallTelegraph> Class, const FTransform& Transform, float InWidth, float InLength, float InDuration, AActor* Owner, APawn* Instigator);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telegraph")
    UDecalComponent* Decal;

    // Danger-rect width, across the slide direction. Should match the wall's measured footprint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
    float Width = 300.f;

    // Danger-rect length, along the slide direction: the full span the wall body covers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
    float Length = 1000.f;

    // Wind-up length in seconds. The fill sweeps start -> end over this; this is the reaction window.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.01"))
    float Duration = 1.0f;

    // How far above/below the actor the decal projects; only needs to exceed floor unevenness.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "1.0"))
    float ProjectionDepth = 200.f;

    // Deferred-decal material with scalar param Progress (0..1) filling along the slide axis.
    // Set on the BP child (see M_Wall_Telegraph).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    UMaterialInterface* TelegraphMaterial;

private:
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* DecalMID = nullptr;

    float Elapsed = 0.f;
};
