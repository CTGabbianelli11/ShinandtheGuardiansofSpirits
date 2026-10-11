#pragma once

#include "CoreMinimal.h"
#include "Combat/StrikeActor.h"
#include "Combat/RadiatingAttackSettings.h"
#include "RingStrike.generated.h"

class UNiagaraComponent;

/** One ground-plane annulus hit. Uses pawn centers in XY, with vertical collision bounds. */
UCLASS()
class RHYATOWEROFWISHES_API ARingStrike : public AStrikeActor
{
    GENERATED_BODY()

public:
    ARingStrike();

    // Configure between SpawnActorDeferred and FinishSpawning.
    void Configure(const FRadiatingAttackSettings& InSettings, float InInnerRadius, float InOuterRadius);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ring Strike")
    float InnerRadius = 0.f;

protected:
    virtual void BeginPlay() override;

private:
    void ApplyRingDamage();

    UPROPERTY(VisibleAnywhere, Category = "Ring Strike")
    TObjectPtr<UNiagaraComponent> Effect;

    UPROPERTY(Transient)
    FRadiatingAttackSettings Settings;
};
