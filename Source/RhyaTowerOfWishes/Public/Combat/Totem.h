#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/DeathInterface.h"
#include "Totem.generated.h"

class UAttributeComponent;
class UStaticMeshComponent;

/**
 * Destructible objective spawned by UTotemAttackComponent. Damage routes through the shared
 * AttributeComponent, whose alive->dead edge calls CharacterDied(): death FX hook, then
 * Destroy(). The totem notifies nobody itself - interested systems bind the engine's
 * OnDestroyed. A BP child supplies the mesh/material and health tuning.
 */
UCLASS()
class RHYATOWEROFWISHES_API ATotem : public AActor, public IHitInterface, public IDeathInterface
{
    GENERATED_BODY()

public:
    ATotem();

    // Spawns Class at Transform with Owner/Instigator wired - returns nullptr (after an ensure)
    // if World/Class/the spawn itself fail.
    static ATotem* SpawnConfigured(UWorld* World, TSubclassOf<ATotem> Class, const FTransform& Transform, AActor* Owner, APawn* Instigator);

    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    virtual void GetHit_Implementation(const FVector& impactPoint, const FVector& impactDirection, const float& damage) override;
    virtual void CharacterDied() override;

protected:
    // Hit feedback (flash/sound) - fires on every contact, dead or alive.
    UFUNCTION(BlueprintImplementableEvent, Category = "Totem")
    void OnTotemHit(const FVector& ImpactPoint, const FVector& ImpactDirection);

    // Death FX hook - runs synchronously before Destroy(), so emitters spawned here outlive the actor.
    UFUNCTION(BlueprintImplementableEvent, Category = "Totem")
    void OnTotemDied();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
    UAttributeComponent* Attributes;

private:
    bool bDied = false;
};
