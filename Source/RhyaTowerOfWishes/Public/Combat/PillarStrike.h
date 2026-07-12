#pragma once

#include "CoreMinimal.h"
#include "Combat/StrikeActor.h"
#include "PillarStrike.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * A telegraph's StrikeClass placeholder: on spawn it damages everything in Radius once at
 * emergence (matching the warning the decal showed), then its mesh rises from underground, holds,
 * sinks back, and the actor destroys itself. BP child supplies the mesh/material.
 */
UCLASS()
class RHYATOWEROFWISHES_API APillarStrike : public AStrikeActor
{
    GENERATED_BODY()

public:
    APillarStrike();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pillar Strike")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pillar Strike")
    UStaticMeshComponent* Mesh;

    // Seconds for the mesh to rise from -RiseDistance to 0.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Strike", meta = (ClampMin = "0.01"))
    float RiseTime = 0.25f;

    // Seconds the risen mesh holds at full height before sinking.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Strike", meta = (ClampMin = "0.0"))
    float HoldTime = 0.75f;

    // Seconds for the mesh to sink back down, after which the actor destroys itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Strike", meta = (ClampMin = "0.01"))
    float SinkTime = 0.4f;

    // How far below the root the mesh starts, and the height it rises through.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Strike", meta = (ClampMin = "1.0"))
    float RiseDistance = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pillar Strike", meta = (ClampMin = "0.0"))
    float Damage = 20.f;

private:
    void ApplyEmergenceDamage();

    enum class EPhase : uint8
    {
        Rising,
        Holding,
        Sinking
    };

    EPhase Phase = EPhase::Rising;
    float Elapsed = 0.f;
};
