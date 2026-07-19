#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TotemAttackComponent.generated.h"

class ATotem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotemPhaseStarted, float, TimeLimit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotemPhaseCleared, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotemPhaseExpired, int32, SurvivingTotems);

/**
 * Destroy-the-totems phase: DoTotemAttack() floor-snaps every TotemPoint, spawns one TotemClass
 * at each, and starts a TimeLimit deadline. Destroying every totem first broadcasts
 * OnPhaseCleared; the deadline firing first broadcasts OnPhaseExpired, and the owning Blueprint
 * decides the punishment (e.g. wire it into a PillarFieldComponent). Death detection is each
 * totem's OnDestroyed delegate - no ticking, no world scans.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UTotemAttackComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    // Totem spawn locations in component-local space, one totem per point.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem Attack")
    TArray<FVector> TotemPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem Attack")
    TSubclassOf<ATotem> TotemClass;

    // Seconds the player has to destroy every totem before OnPhaseExpired fires.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem Attack", meta = (ClampMin = "0.01"))
    float TimeLimit = 8.f;

    UPROPERTY(BlueprintAssignable, Category = "Totem Attack")
    FOnTotemPhaseStarted OnPhaseStarted;

    UPROPERTY(BlueprintAssignable, Category = "Totem Attack")
    FOnTotemPhaseCleared OnPhaseCleared;

    // Surviving totems are left standing; listeners decide their fate.
    UPROPERTY(BlueprintAssignable, Category = "Totem Attack")
    FOnTotemPhaseExpired OnPhaseExpired;

    UFUNCTION(BlueprintCallable, Category = "Totem Attack")
    void DoTotemAttack();

    // Silent teardown: no broadcasts, remaining totems destroyed.
    UFUNCTION(BlueprintCallable, Category = "Totem Attack")
    void CancelTotemPhase();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Totem Attack")
    bool IsPhaseActive() const { return bPhaseActive; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Totem Attack")
    TArray<ATotem*> GetAliveTotems() const { return AliveTotems; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleTotemDestroyed(AActor* DestroyedActor);

    void HandleDeadlineExpired();
    void RegisterTotem(ATotem* Totem);
    void DrawDebugState();

    // Roster of the phase's live totems: joined only by RegisterTotem (add paired with the
    // OnDestroyed bind), left only by HandleTotemDestroyed - so it can't drift from the world.
    UPROPERTY(Transient)
    TArray<ATotem*> AliveTotems;

    FTimerHandle DeadlineHandle;
    FTimerHandle StateDrawHandle;
    bool bPhaseActive = false;
};
