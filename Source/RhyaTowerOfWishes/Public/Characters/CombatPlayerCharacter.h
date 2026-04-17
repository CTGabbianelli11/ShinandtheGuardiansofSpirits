
// CPPCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
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

UCLASS()
class RHYATOWEROFWISHES_API ACombatPlayerCharacter : public ACharacter, public IPickupInterface, public IHitInterface, public IDeathInterface
{
    GENERATED_BODY()

public:

    ACombatPlayerCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Interface overrides
    virtual void GetHit(const FVector& impactPoint, const FVector& impactDirection) override;

    virtual void CharacterDied() override;

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
    void SpawnAndEquipWeapon(TSubclassOf<AWeapon> Weapon);

    UAttributeComponent* GetAttributes() { return attributeComponent; }

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



    int attackNumber = 0;

    UFUNCTION(BlueprintImplementableEvent)
    void CharacterDiedEvent();
    UFUNCTION(BlueprintImplementableEvent)
    void CharacterHit(const FVector& impactPoint, const FVector& impactDirection);

    // Input Callbacks
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Interact(const FInputActionValue& Value);

    void EquipWeapon(AWeapon* overlappingWeapon);
    void Attack(const FInputActionValue& Value);
    void Dodge(const FInputActionValue& Value);


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
    UAnimMontage* HitReactMontage;

public:

    FORCEINLINE ECharacterState GetCharacterState() const { return state; }

    // ----------- FIXED INTERFACE OVERRIDES -----------
    virtual void SetOverlappingItem_Implementation(AItem* Item) override;
};
