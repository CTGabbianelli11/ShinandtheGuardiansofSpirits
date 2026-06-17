
// CPPCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
// EnhancedInput inline code narrows double->float; engine-owned, exempt from UnsafeTypeCastWarningLevel.
PRAGMA_DISABLE_UNSAFE_TYPECAST_WARNINGS
#include "InputAction.h"
PRAGMA_RESTORE_UNSAFE_TYPECAST_WARNINGS
#include "Characters/CharacterTypes.h"
#include "Interfaces/PickupInterface.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/DeathInterface.h"
#include "CombatPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UAttributeComponent;
class AItem;
class AWeapon;
class ACurrency;
class UAnimInstance;
class UWeaponDataAsset;
class UTimelineComponent;
class UAC_HitStop;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboAttackChanged, int, ComboIndex);

UCLASS()
class RHYATOWEROFWISHES_API ACombatPlayerCharacter : public ACharacter, public IPickupInterface, public IHitInterface, public IDeathInterface
{
    GENERATED_BODY()

public:

    ACombatPlayerCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Blocking gate for all damage sources: a blocked frontal hit returns without
    // calling Super, so the BP AnyDamage -> ReceiveDamage path never fires.
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser) override;

    // Interface overrides
    virtual void GetHit_Implementation(const FVector& impactPoint, const FVector& impactDirection) override;

    virtual void CharacterDied() override;

    // BlueprintAssignable delegates
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnComboAttackChanged OnComboIndexChanged;

    //Combat Helpers
    void MovePlayerToEnemy(AActor* player, AActor* enemy);

    // Weapon Collision
    UFUNCTION(BlueprintCallable)
    void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

    // Equip an existing weapon instance
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EquipExistingWeapon(AWeapon* WeaponInstance);

    // Spawn from DataAsset
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    AWeapon* SpawnAndEquipWeapon(TSubclassOf<AWeapon> Weapon);

    UAttributeComponent* GetAttributes() { return attributeComponent; }

    void SetAttackNumber(int AttackNumber);
    UFUNCTION(BlueprintPure, Category = "ComboIndex")
    int GetAttackNumber();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UAC_HitStop* hitStopComponent;

protected:

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
    UAttributeComponent* attributeComponent;

    UPROPERTY(BlueprintReadWrite, Category = "Weapon")
    AWeapon* equippedWeapon;

    // Input
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputMappingContext* CharacterInputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* InteractAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* AttackAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* DodgeAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* BlockAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    int maxComboNumber = 3;

    int comboIndex = 0;

    UFUNCTION(BlueprintImplementableEvent)
    void CharacterDiedEvent();
    UFUNCTION(BlueprintImplementableEvent)
    void CharacterHit(const FVector& impactPoint, const FVector& impactDirection);

    // Blocked-hit counterpart to CharacterHit, for BP VFX/SFX on a successful deflect.
    UFUNCTION(BlueprintImplementableEvent)
    void CharacterBlockedHit(const FVector& impactPoint, const FVector& impactDirection);

    bool IsAttackerInBlockCone(const AActor* Attacker) const;
    bool IsDirectionInBlockCone(const FVector& ToSource) const;
    void PlayBlockImpactMontage();
    void OnBlockImpactMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    // True while the block input is held — polled from Enhanced Input, not latched in a flag.
    bool IsBlockHeld() const;
    // Re-enter block if still held, else go neutral. Shared by dodge and hit-react recovery.
    void ResumeBlockIfHeld();
    // Live world-space move direction polled from MoveAction — valid even while blocking,
    // when Move() suppresses AddMovementInput. Zero if no direction is held.
    FVector GetMoveInputWorldDirection() const;

    // Input Callbacks
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Interact(const FInputActionValue& Value);

    void EquipWeapon(AWeapon* overlappingWeapon);
    void Attack(const FInputActionValue& Value);
    void DefensiveAction(const FInputActionValue& Value);


    void PlayAttackMontage();

    FVector2D movementVector;
    FRotator YawRotation;

protected: 
    UFUNCTION(BlueprintCallable) void StartInputBuffer();   
    UFUNCTION(BlueprintCallable) void AttackEnd();
    UFUNCTION(BlueprintCallable) void EndBuffer();
    UFUNCTION(BlueprintCallable) FName GetCurrentAttack();
    UFUNCTION(BlueprintCallable) void StartDodge();
    UFUNCTION(BlueprintCallable) void EndDodge();
    UFUNCTION(BlueprintCallable) void StartBlock();
    UFUNCTION(BlueprintCallable) void EndBlock();

    bool CanAttack();

private:

    ECharacterState state = ECharacterState::ECS_Unequipped;

    UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    EactionState actionState = EactionState::EAS_Unoccupied;

    UPROPERTY(VisibleAnywhere)
    UCameraComponent* CameraComponent;

    UPROPERTY(VisibleAnywhere)
    USpringArmComponent* SpringArm;
    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* AttackTimeline;

    UPROPERTY(VisibleInstanceOnly)
    AItem* overlappingItem;

    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* AttackMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* DodgeMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* BlockMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* HitReactMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* BlockImpactMontage;

    // Half-angle of the frontal block cone, measured from actor forward in the ground plane.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Block", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float BlockAngleDegrees = 60.f;

    // 0 = full deflect; > 0 = that fraction leaks through as chip damage.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Block", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BlockedDamageMultiplier = 0.f;

    // Per-hit block verdict, published by whichever of TakeDamage/GetHit runs
    // first that frame, so the damage gate and the react can't disagree.
    bool bBlockedLastHit = false;
    uint64 LastBlockVerdictFrame = 0;

public:

    FORCEINLINE ECharacterState GetCharacterState() const { return state; }

    // ----------- FIXED INTERFACE OVERRIDES -----------
    virtual void SetOverlappingItem_Implementation(AItem* Item) override;
};
