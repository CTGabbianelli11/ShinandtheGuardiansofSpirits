
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/TimerHandle.h"
#include "Animation/AnimNotifyQueue.h"
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
class UAnimNotifyState;
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
    virtual void Landed(const FHitResult& Hit) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Blocking gate for all damage sources: a blocked frontal hit returns without
    // calling Super, so the BP AnyDamage -> ReceiveDamage path never fires.
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser) override;

    // Interface overrides
    virtual void GetHit_Implementation(const FVector& impactPoint, const FVector& impactDirection, const float& damage) override;

    virtual void CharacterDied_Implementation() override;

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

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    AWeapon* SpawnAndEquipWeapon(TSubclassOf<AWeapon> Weapon);

    UAttributeComponent* GetAttributes() { return attributeComponent; }
    UFUNCTION(BlueprintCallable, Category = "ComboIndex")
    void SetAttackNumber(int AttackNumber);
    UFUNCTION(BlueprintCallable, Category = "ComboIndex")
    void SetActionState(EactionState ActionState);
    UFUNCTION(BlueprintPure, Category = "Combat|Actions")
    bool IsActionInputAllowed() const;
    UFUNCTION(BlueprintPure, Category = "Combat|Actions")
    bool CanStartAction(EactionState RequestedAction) const;
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    bool TryStartAction(EactionState RequestedAction);
    UFUNCTION(BlueprintPure, Category = "Combat|Actions")
    EactionState GetActionState() const { return actionState; }

    // Register from AnimNotifyState Begin to track interruptions for this player.
    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void RegisterAnimNotifyState(const UAnimNotifyState* NotifyState, const FAnimNotifyEventReference& EventReference);
    // Consume from AnimNotifyState End. Always clean up; only None permits normal completion.
    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    EGameplayNotifyInterruptReason ConsumeAnimNotifyStateInterruptReason(const UAnimNotifyState* NotifyState,
        const FAnimNotifyEventReference& EventReference);
    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void SetDodgeInvulnerable(bool bEnabled);
    virtual void Jump() override;
    UFUNCTION(BlueprintPure, Category = "ComboIndex")
    int GetAttackNumber();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UAC_HitStop* hitStopComponent;

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
    UInputAction* HealAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    int maxComboNumber = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attacks)
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
    void EnterStagger(bool bWaitForLanding = false);
    void StartHitLandingRecovery(uint32 Generation);
    void FinishStagger(uint32 Generation);
    void CancelCurrentAction(EGameplayNotifyInterruptReason InterruptReason);
    void InterruptGameplayNotifies(EGameplayNotifyInterruptReason InterruptReason);
    bool IsCombatAlive() const;

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
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Montages")
    UObject* CurrentAttackTarget;

    UFUNCTION(BlueprintCallable) void StartInputBuffer();   
    UFUNCTION(BlueprintCallable) void AttackEnd();
    UFUNCTION(BlueprintCallable) void EndBuffer();
    UFUNCTION(BlueprintCallable) FName GetCurrentAttack();
    UFUNCTION(BlueprintCallable) void StartDodge();
    UFUNCTION(BlueprintCallable) void EndDodge();
    UFUNCTION(BlueprintCallable) void StartBlock();
    UFUNCTION(BlueprintCallable) void EndBlock();

    // Channelled heal: held input drains magic in pulses and adds health each pulse.
    // Start on press, stop on release / out of magic / full health / interruption.
    UFUNCTION(BlueprintCallable) void StartHeal();
    UFUNCTION(BlueprintCallable) void StopHeal();
    void HealTick();

    bool CanAttack();

private:

    // Callers enforce transition rules; this assigns the state and logs changes.
    void ChangeActionState(EactionState NewState, const TCHAR* Reason);

    ECharacterState state = ECharacterState::ECS_Unequipped;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))

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
    UAnimMontage* AerialAttackMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* GroundToAirAttackMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* DodgeMontage;
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* BlockMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* HitReactMontage;

    // When assigned, launched hits hold their air pose until landing, then recover through this montage.
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* LandHitReactMontage;

    // Disable the scripted facing, stagger, launch and landing sequence. Damage and hit effects still run.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Hit Reaction")
    bool bEnableKnockbackHitReaction = true;

    // Backward launch speed, independent of damage. Zero disables horizontal knockback.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Hit Reaction", meta = (ClampMin = "0.0", Units = "cm/s"))
    float KnockbackSpeed = 800.f;

    // Upward component of the launch velocity; independent of horizontal knockback.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Hit Reaction", meta = (ClampMin = "0.0", Units = "cm/s"))
    float HitLaunchUpSpeed = 1500.f;

    // Hold the downed pose after ground contact before starting the get-up montage.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Hit Reaction", meta = (ClampMin = "0.0", Units = "s"))
    float HitGroundedHoldDuration = 0.1f;

    // Restart recovery when hit again during stagger.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Stagger")
    bool bRestartStaggerOnHit = true;

    // Recovery time in seconds when HitReactMontage cannot play.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Stagger", meta = (ClampMin = "0.01"))
    float StaggerFallbackDuration = 0.42f;

    FTimerHandle StaggerTimerHandle;
    uint32 StaggerGeneration = 0;
    bool bWaitingForHitLanding = false;
    uint32 DodgeGeneration = 0;
    bool bCombatDead = false;
    bool bDodgeInvulnerable = false;
    struct FActiveGameplayNotify
    {
        TWeakObjectPtr<const UAnimNotifyState> NotifyState;
        // Distinguishes montage activations; INDEX_NONE when the event has no montage context.
        int32 MontageInstanceID = INDEX_NONE;
        EGameplayNotifyInterruptReason InterruptReason = EGameplayNotifyInterruptReason::None;
    };
    TArray<FActiveGameplayNotify> ActiveGameplayNotifies;

    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* BlockImpactMontage;

    // Optional channel loop played while healing; heal works without it.
    UPROPERTY(EditDefaultsOnly, Category = "Montages")
    UAnimMontage* HealMontage;


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

    // --- Channelled heal tunables ---
    // Health restored per pulse.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Heal", meta = (ClampMin = "0.0"))
    float HealPerTick = 5.f;
    // Magic spent per pulse; the channel stops when the next pulse is unaffordable.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Heal", meta = (ClampMin = "0.0"))
    float MagicPerTick = 8.f;
    // Seconds between pulses.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Heal", meta = (ClampMin = "0.01"))
    float HealInterval = 0.5f;

    FTimerHandle HealTimerHandle;
    // Game-time of the last heal pulse; gates the press-triggered immediate pulse so tapping
    // the heal button can't out-pace holding it. -1000 = "never healed" (first press always pulses).
    double LastHealPulseTime = -1000.0;

public:

    FORCEINLINE ECharacterState GetCharacterState() const { return state; }

    // ----------- FIXED INTERFACE OVERRIDES -----------
    virtual void SetOverlappingItem_Implementation(AItem* Item) override;
};
