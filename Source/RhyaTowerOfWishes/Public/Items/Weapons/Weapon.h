
// Weapon.h
#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h" // Base class providing ItemMesh/sphereCollider & overlap hooks
#include "Weapon.generated.h"

class UBoxComponent;
class USceneComponent;
class UTexture2D;
class USoundBase;
class UWeaponDataAsset;
class UPrimitiveComponent;
class APawn;

/** Events broadcast in Weapon.cpp */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquipped, bool, bEquipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPurchased, bool, bPurchased);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponUpgraded, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponUnequipped, bool, bEquipped);

UCLASS()
class RHYATOWEROFWISHES_API AWeapon : public AItem
{
    GENERATED_BODY()

public:
    AWeapon();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator);

    /** Explicit toggle so external code (Character/Shop) can keep state & events consistent. */
    UFUNCTION(BlueprintCallable, Category = "Weapon|State")
    void SetEquipped(bool bInEquipped);


    /** Current (level-scaled) price using your formula. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Config", meta = (AllowPrivateAccess = "true"))
    float  KnockBackAmount = 10.0f;

    /** Accessor used in anim notifies / gameplay to gate collisions */
    UFUNCTION(BlueprintPure, Category = "Weapon|Collision")
    UBoxComponent* GetWeaponBoxComponent() const { return WeaponBoxComponent; }


    // ===== Events =====
    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FOnWeaponEquipped OnEquipped;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FOnWeaponPurchased OnPurchased;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FOnWeaponUpgraded OnUpgraded;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FOnWeaponUnequipped OnUnequipped;

    // ===== Exposed for Character montage collision gate =====
    // Your Character clears this on collision enable:
    // equippedWeapon->ignoreActors.Empty();
    UPROPERTY()
    TArray<AActor*> ignoreActors;

protected:
    // Overlap hooks (AItem provides UFUNCTIONs; DO NOT repeat UFUNCTION() here)
    virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult) override;

    virtual void OnEndSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

    // Needs UFUNCTION because we bind it with AddDynamic in BeginPlay
    UFUNCTION()
    void OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);


private:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = "true"))
    UBoxComponent* WeaponBoxComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = "true"))
    USceneComponent* BoxTraceStart = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = "true"))
    USceneComponent* BoxTraceEnd = nullptr;

    // ===== Audio =====
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio", meta = (AllowPrivateAccess = "true"))
    USoundBase* HitSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio", meta = (AllowPrivateAccess = "true"))
    USoundBase* MissSoundEffect = nullptr;


    // Current computed damage (used in ApplyDamage)
    UPROPERTY(BlueprintReadOnly, Category = "Weapon|Runtime", meta = (AllowPrivateAccess = "true"))
    float damage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon|State", meta = (AllowPrivateAccess = "true"))
    bool bIsEquipped = false;

public:
};
