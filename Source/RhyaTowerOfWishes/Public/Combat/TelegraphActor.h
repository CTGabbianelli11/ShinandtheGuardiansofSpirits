#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TelegraphActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Ground telegraph: a flat disc whose material fills from the center out to Radius over
 * Duration, then spawns StrikeClass at this actor's transform and destroys itself.
 */
UCLASS()
class RHYATOWEROFWISHES_API ATelegraphActor : public AActor
{
    GENERATED_BODY()

public:
    ATelegraphActor();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telegraph")
    UStaticMeshComponent* Disc;

    // Danger radius in world units. Drives the visual and should match the spawned strike's reach.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
    float Radius = 120.f;

    // Wind-up length in seconds. The fill sweeps 0 -> Radius over this; this is the reaction window.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.01"))
    float Duration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    FLinearColor Color = FLinearColor(1.f, 0.15f, 0.05f, 1.f);

    // Spawned at this actor's transform when the wind-up completes (e.g. BP_AoE). The decoupled strike.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    TSubclassOf<AActor> StrikeClass;

    // Needs scalar params Progress (0..1) + Radius and vector param Color (see M_TelegraphCircle).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    UMaterialInterface* TelegraphMaterial;

private:
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* DiscMID = nullptr;

    float Elapsed = 0.f;
    bool bStruck = false;
};
