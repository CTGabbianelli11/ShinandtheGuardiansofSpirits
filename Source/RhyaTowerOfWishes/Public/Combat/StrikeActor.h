#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrikeActor.generated.h"

/**
 * Base for anything spawned as a telegraph's StrikeClass. Radius carries the danger radius the
 * telegraph was set up with, so the strike's actual hit area always matches the warning the
 * player saw. It's a plain property (not a function call) so callers can set it any time before
 * FinishSpawning() - the Construction Script then reads it to resize whatever component defines
 * the strike's reach (e.g. an overlap sphere).
 */

 // For parameters (Pass the type, then the parameter name):
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStrikeHappened, FVector, impactLocation,FRotator, impactRotation);
UCLASS()
class RHYATOWEROFWISHES_API AStrikeActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strike")
    float Radius = 0.f;

    // Deals DamageAmount once to every pawn within Radius as RADIAL damage: the event carries
    // this actor's location as the epicenter.
    UFUNCTION(BlueprintCallable, Category = "Strike")
    void DealRadialDamage(float DamageAmount);

    // Spawns Class at Transform with Radius already configured. Encapsulates the
    // SpawnActorDeferred/FinishSpawning ordering Radius's safety depends on, so callers never
    // need to know it - returns nullptr (after an ensure) if World/Class/the spawn itself fail.
    static AStrikeActor* SpawnConfigured(UWorld* World, TSubclassOf<AStrikeActor> Class, const FTransform& Transform, float Radius, AActor* Owner, APawn* Instigator);

    // Spawned at end of telegraph to add vfx at end impact point.
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStrikeHappened StrikeHappened;
};
