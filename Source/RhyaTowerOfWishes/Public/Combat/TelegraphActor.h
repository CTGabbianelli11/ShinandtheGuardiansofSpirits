#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TelegraphActor.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AStrikeActor;

/**
 * Ground telegraph: a deferred decal projected onto the terrain whose material fills from the
 * center out to Radius over Duration, then spawns StrikeClass at this actor's transform and
 * destroys itself.
 */
UCLASS()
class RHYATOWEROFWISHES_API ATelegraphActor : public AActor
{
    GENERATED_BODY()

public:
    ATelegraphActor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Telegraph")
    void Configure(float InRadius, float InDuration, TSubclassOf<AStrikeActor> InStrikeClass);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telegraph")
    UDecalComponent* Decal;

    // Danger radius in world units. Drives the decal size and should match the spawned strike's reach.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
    float Radius = 120.f;

    // Wind-up length in seconds. The fill sweeps 0 -> Radius over this; this is the reaction window.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.01"))
    float Duration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    FLinearColor Color = FLinearColor(1.f, 0.15f, 0.05f, 1.f);

    // Spawned at this actor's transform when the wind-up completes (e.g. BP_AoE). The decoupled strike.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    TSubclassOf<AStrikeActor> StrikeClass;

    // Substrate deferred-decal material with scalar param Progress (0..1). See M_AoE_Telegraph.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    UMaterialInterface* TelegraphMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
    float Elapsed = 0.f;
private:
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* DecalMID = nullptr;


    bool bStruck = false;
};
