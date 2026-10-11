#pragma once

#include "CoreMinimal.h"
#include "RadiatingAttackSettings.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

/**
 * Damage, geometry, timing, and visual/audio settings for a radiating attack.
 *
 *
 * Times below are seconds after starting the attack; radii are centimeters from
 * the shared, floor-snapped Center. The first pulse includes the center; later
 * pulses exclude their inner boundary to avoid hitting a shared edge twice.
 *
 * RingInterval = 0.3f;
 * TelegraphDuration = 0.5f;
 * ImpactLifetime = 1.f
 *
 * Pulse   Damage band       Warning starts   Hits at   Impact actor expires
 * 1       0-150 (disk)      0.0              0.5       1.5
 * 2       >150 to 300       0.3              0.8       1.8
 * 3       >300 to 450       0.6              1.1       2.1
 *
 * Each warning lasts 0.5 seconds, so adjacent warnings overlap for 0.2 seconds.
 * Each pulse applies 20 damage once per target in its band. Later pulses do not
 * hit the inner bands again. Target collision must also overlap the volume from
 * 100 cm below to 100 cm above the center. The 1-second impact lifetime allows
 * visuals to finish; it does not extend the damage window.
 *
 * The warning uses WarningColor. ImpactEffect and ImpactSound are optional and
 * unset by default; when assigned, they play at each impact. The Niagara effect
 * must use the supplied radius parameters to match the band.
 */
USTRUCT(BlueprintType)
struct RHYATOWEROFWISHES_API FRadiatingAttackSettings
{
    GENERATED_BODY()

    /** Damage applied once to each target in a ring when it activates. Each ring has its own hit check. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
    float Damage = 20.f;

    /** Radial thickness in centimeters. A width of 150 gives bands of 0-150, 150-300, then 300-450. The first is a disk; later rings have hollow centers. Ring count is supplied when starting the attack. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
    float RingWidth = 150.f;

    /** Vertical reach in centimeters above AND below the attack's floor position. A value of 100 gives a total height of 200. Target collision must overlap this height to be hit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
    float DamageHalfHeight = 100.f;

    /** Seconds between successive warning starts, and between successive impacts. If shorter than Telegraph Duration, warnings overlap. This is not added after the previous warning finishes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.01"))
    float RingInterval = 0.3f;

    /** Seconds each ring warns before hitting. With the default duration of 0.5 and Ring Interval of 0.3, impacts occur at 0.5, 0.8, and 1.1 seconds after starting a three-ring attack. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.01"))
    float TelegraphDuration = 0.5f;

    /** Seconds the impact actor remains after hitting, allowing its visual effect to play. Damage happens once at impact; this does not create a lingering damage area. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.01"))
    float ImpactLifetime = 1.f;

    /** Required warning decal material. Use M_RingTelegraph for hollow rings. The material must use InnerRadiusRatio and Progress (both 0-1), plus WarningColor; an ordinary circle material will still draw filled disks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    TObjectPtr<UMaterialInterface> TelegraphMaterial = nullptr;

    /** Warning decal color, supplied to the material's WarningColor parameter. Does not change the Niagara impact effect's color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    FLinearColor WarningColor = FLinearColor(1.f, 0.15f, 0.05f, 1.f);

    /** Optional Niagara effect played at each ring's impact. To match the damage band, the effect must use User.InnerRadius and User.OuterRadius (centimeters). It also receives User.ImpactLifetime (seconds). Assigning an effect does not automatically make it ring-shaped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    TObjectPtr<UNiagaraSystem> ImpactEffect = nullptr;

    /** Optional sound played at the shared attack center each time a ring hits. Leave empty for no impact sound. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    TObjectPtr<USoundBase> ImpactSound = nullptr;
};
