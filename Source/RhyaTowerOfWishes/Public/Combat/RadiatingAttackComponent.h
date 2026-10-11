#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/RadiatingAttackSettings.h"
#include "RadiatingAttackComponent.generated.h"

class ATelegraphActor;

/** Sequences warnings at one fixed center. The owning Blueprint supplies phase/ring count. */
UCLASS(Blueprintable, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API URadiatingAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URadiatingAttackComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radiating Attack")
    FRadiatingAttackSettings AttackSettings;

    // Replaces any pending sequence. Floor-snaps Center once; returns false on invalid setup/no floor.
    UFUNCTION(BlueprintCallable, Category = "Radiating Attack")
    bool StartRadiatingAttack(FVector Center, int32 RingCount);

    // Cancels timers AND already visible warnings. Call on death/attack interruption as appropriate.
    UFUNCTION(BlueprintCallable, Category = "Radiating Attack")
    void CancelRadiatingAttack();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void SpawnNextRing();

    UPROPERTY(Transient)
    FRadiatingAttackSettings ActiveSettings;

    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<ATelegraphActor>> PendingWarnings;

    FTimerHandle RingTimer;
    FVector AttackCenter = FVector::ZeroVector;
    int32 NextRing = 0;
    int32 TotalRings = 0;
};
