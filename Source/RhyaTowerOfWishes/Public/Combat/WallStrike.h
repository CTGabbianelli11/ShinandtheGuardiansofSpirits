#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WallStrike.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * A sliding wall: spawns visible but hitless so the player sees it during the telegraph, then
 * after WindupTime arms its collision and slides TravelDistance along its forward axis at
 * SlideSpeed, damaging each pawn it sweeps through once, then destroys itself.
 *
 * The collision box is the root and is sized from the mesh's bounds in OnConstruction, so the
 * mesh is the single source of truth for the wall's size - iterate by resizing the mesh in the
 * BP child and the hitbox (and the telegraph, which measures the spawned wall) follow. The mesh
 * must stay centered on the root for the two to line up.
 */
UCLASS()
class RHYATOWEROFWISHES_API AWallStrike : public AActor
{
    GENERATED_BODY()

public:
    AWallStrike();
    virtual void Tick(float DeltaSeconds) override;

    // Half-extents of the armed hitbox; the component reads this to size the telegraph.
    UFUNCTION(BlueprintCallable, Category = "Wall Strike")
    FVector GetCollisionExtent() const;

    // Spawns Class at Transform with the slide parameters already configured. Encapsulates the
    // SpawnActorDeferred/FinishSpawning ordering the plain properties depend on - returns
    // nullptr (after an ensure) if World/Class/the parameters/the spawn itself fail.
    // Transform's location is the wall's BASE (the floor point); the wall lifts itself onto it.
    static AWallStrike* SpawnConfigured(UWorld* World, TSubclassOf<AWallStrike> Class, const FTransform& Transform, float InWindupTime, float InSlideSpeed, float InTravelDistance, AActor* Owner, APawn* Instigator);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Strike")
    UBoxComponent* Box;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Strike")
    UStaticMeshComponent* Mesh;

    // Seconds the wall sits dormant before arming and sliding. Matches the telegraph's duration.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Strike", meta = (ClampMin = "0.0"))
    float WindupTime = 1.0f;

    // Slide speed in units/second once armed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Strike", meta = (ClampMin = "1.0"))
    float SlideSpeed = 800.f;

    // How far the wall travels along its forward axis before destroying itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Strike", meta = (ClampMin = "0.0"))
    float TravelDistance = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Strike", meta = (ClampMin = "0.0"))
    float Damage = 20.f;

private:
    UFUNCTION()
    void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    enum class EPhase : uint8
    {
        Dormant,
        Sliding
    };

    EPhase Phase = EPhase::Dormant;
    float Elapsed = 0.f;
    float Traveled = 0.f;

    // Each actor is damaged once per slide, not once per tick spent inside the box.
    UPROPERTY(Transient)
    TSet<AActor*> DamagedActors;
};
