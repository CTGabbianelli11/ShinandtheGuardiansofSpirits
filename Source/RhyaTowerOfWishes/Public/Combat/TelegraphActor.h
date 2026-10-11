#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/RadiatingAttackSettings.h"
#include "TelegraphActor.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AStrikeActor;

/**
 * Ground warning decal for a disk or ring. Advances the material's Progress from 0 to 1
 * over Duration, then spawns the strike at this actor's location and destroys itself.
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

    // Explicit configuration for a ring pulse, before FinishSpawning. Existing Configure callers stay circular.
    void ConfigureRadiatingPulse(const FRadiatingAttackSettings& InSettings, float InInnerRadius, float InOuterRadius);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telegraph")
    UDecalComponent* Decal;

    // Danger radius in world units. Drives the decal size and should match the spawned strike's reach.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
    float Radius = 120.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telegraph")
    float InnerRadius = 0.f;

    // Warning duration in seconds before the strike spawns.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.01"))
    float Duration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
    FLinearColor Color = FLinearColor(1.f, 0.15f, 0.05f, 1.f);

    // Spawned at this actor's location when the warning completes.
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

    UPROPERTY(Transient)
    FRadiatingAttackSettings PulseSettings;

    bool bRadiatingPulse = false;


    bool bStruck = false;
};
