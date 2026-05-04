
// CPPCharacter.cpp
#include "Characters/CombatPlayerCharacter.h"
#include "Animation/AnimMontage.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <EnhancedInputComponent.h>
#include "Items/Item.h"
#include "Components/AttributeComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Components/BoxComponent.h"

#include "Kismet/GameplayStatics.h"
#include <Kismet/KismetMathLibrary.h>
#include "RhyaTowerOfWishes/DebugMacros.h"
#include <Components/TimelineComponent.h>
#include <Components/AC_HitStop.h>
#include <Enemy/Enemy.h>

ACombatPlayerCharacter::ACombatPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
    SpringArm->SetupAttachment(GetRootComponent());
    SpringArm->TargetArmLength = 300.f;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComponent->SetupAttachment(SpringArm);
    
    hitStopComponent = CreateDefaultSubobject<UAC_HitStop>(TEXT("HitStop"));

    attributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
}

void ACombatPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {

        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {

            Subsystem->AddMappingContext(CharacterInputMappingContext, 0);
        }
    }
}

void ACombatPlayerCharacter::Move(const FInputActionValue& Value)
{

    movementVector = Value.Get<FVector2D>();

    const FRotator controlRotation = GetControlRotation();
    YawRotation = FRotator(0.f, controlRotation.Yaw, 0.f);

    FString EnumAsString = UEnum::GetValueAsString(actionState);

    if (actionState != EactionState::EAS_Unoccupied)
        return;
    


    AddMovementInput(
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X),
        movementVector.Y
    );

    AddMovementInput(
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y),
        movementVector.X
    );
}

void ACombatPlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D lookValue = Value.Get<FVector2D>();
    AddControllerYawInput(lookValue.X);
    AddControllerPitchInput(-lookValue.Y);
}

void ACombatPlayerCharacter::Interact(const FInputActionValue& /*Value*/)
{
    if (AWeapon* overlappingWeapon = Cast<AWeapon>(overlappingItem))
    {
        //EquipExistingWeapon(overlappingWeapon);
    }
}

void ACombatPlayerCharacter::EquipWeapon(AWeapon* overlappingWeapon)
{
    if (!overlappingWeapon) return;
    overlappingWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
    equippedWeapon = overlappingWeapon;
    state = ECharacterState::ECS_EquippedOneHandedWeapon;
    equippedWeapon->SetEquipped(true);
}

void ACombatPlayerCharacter::EquipExistingWeapon(AWeapon* WeaponInstance)
{
    EquipWeapon(WeaponInstance);
}

AWeapon* ACombatPlayerCharacter::SpawnAndEquipWeapon(TSubclassOf<AWeapon> Weapon)
{ 
    if (!Weapon) return nullptr;



    UWorld* World = GetWorld();
    if (!World) return nullptr;

    AWeapon* NewWeapon = World->SpawnActor<AWeapon>(Weapon);
    if (!NewWeapon) return nullptr;

    EquipWeapon(NewWeapon);

    return NewWeapon;
}


void ACombatPlayerCharacter::Attack(const FInputActionValue& /*Value*/)
{
    if (CanAttack())
    {

        GetCharacterMovement()->RotationRate = FRotator(0, 50000, 0);



        AddMovementInput(
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X),
            movementVector.Y
        );

        AddMovementInput(
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y),
            movementVector.X
        );

        TArray<AActor*> ActorsInFront;
        FHitResult Hit;
        UWorld* world = GetWorld();
        if (world)
        {

            for (int i = -10; i < 10; i++)
            {
                // [TwstdTree] Convert ECC_GameTraceChannel1 to EObjectTypeQuery
                TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
                ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
                UKismetSystemLibrary::LineTraceSingleForObjects(
                    world,
                    GetActorLocation(),
                    GetActorLocation() + UKismetMathLibrary::GreaterGreater_VectorRotator(GetActorForwardVector(), FRotator(0, 5 * i, 0)) * 400.f,
                     ObjectTypes,
                    false,
                    ActorsInFront,
                    EDrawDebugTrace::None,
                    Hit,
                    true);
                if (Hit.GetActor() && Cast<AEnemy>(Hit.GetActor()))
                {
                    ActorsInFront.AddUnique(Hit.GetActor());
                    //DRAW_SPHERE_COLOR(Hit.GetActor()->GetActorLocation(), FColor::Orange);
                }
            }
        }

        if (ActorsInFront.Num() > 0)
        {
            MovePlayerToEnemy(this, ActorsInFront[0]);
        }
        else
        {
            //SetActorRotation(UKismetMathLibrary::Conv_VectorToRotator(GetLastMovementInputVector()));
        }

        equippedWeapon->ignoreActors.Empty();

        PlayAttackMontage();
        actionState = EactionState::EAS_Attacking;
    }
}

void ACombatPlayerCharacter::Dodge(const FInputActionValue& Value)
{
    if (actionState == EactionState::EAS_Dodging)
        return;
    if (actionState == EactionState::EAS_Comboing || actionState == EactionState::EAS_Attacking)
        AttackEnd();


    actionState = EactionState::EAS_Dodging;


    SetActorRotation(UKismetMathLibrary::Conv_VectorToRotator(GetLastMovementInputVector()));

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(DodgeMontage);
        StartDodge();
    }
}

void ACombatPlayerCharacter::StartDodge()
{

}

void ACombatPlayerCharacter::EndDodge()
{
    actionState = EactionState::EAS_Unoccupied;
}

bool ACombatPlayerCharacter::CanAttack()
{
    return (actionState == EactionState::EAS_Unoccupied ||
        actionState == EactionState::EAS_Comboing)
        && state != ECharacterState::ECS_Unequipped;
}

void ACombatPlayerCharacter::GetHit(const FVector& impactPoint, const FVector& impactDirection)
{
    CharacterHit(impactPoint, impactDirection);

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (HitReactMontage)
        {
            AnimInstance->Montage_Play(HitReactMontage);
            actionState = EactionState::EAS_Unoccupied;
        }
    }
}

void ACombatPlayerCharacter::CharacterDied()
{
    GEngine->AddOnScreenDebugMessage(0, 1, FColor::Red, TEXT("Dead"));
    GetMesh()->SetAnimInstanceClass(nullptr);

    if (UWorld* World = GetWorld())
    {
        GetMesh()->SetSimulatePhysics(true);
        GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    }

    CharacterDiedEvent();
}

void ACombatPlayerCharacter::MovePlayerToEnemy(AActor* player, AActor* enemy)
{
    FVector delta = player->GetActorLocation() - enemy->GetActorLocation();
    FVector direction = UKismetMathLibrary::Normal(delta, .001f);
    FVector magnitude = direction * 150.f;

    FVector positionOffset = magnitude + enemy->GetActorLocation();
    positionOffset.Z = GetActorLocation().Z;

    FRotator forwardRotation = UKismetMathLibrary::FindLookAtRotation(player->GetActorLocation(), enemy->GetActorLocation());

    FLatentActionInfo LatentInfo;
    LatentInfo.CallbackTarget = this;
    LatentInfo.ExecutionFunction = FName("MoveToTargetFinished");
    LatentInfo.Linkage = 0;
    LatentInfo.UUID = 0;
    UKismetSystemLibrary::MoveComponentTo(GetRootComponent(), positionOffset, forwardRotation, true, true, .3f, true, EMoveComponentAction::Move, LatentInfo);

}

void ACombatPlayerCharacter::PlayAttackMontage()
{
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (AttackMontage)
        {
            AnimInstance->Montage_Play(AttackMontage);
            AnimInstance->Montage_JumpToSection(GetCurrentAttack(), AttackMontage);
        }
    }
}

void ACombatPlayerCharacter::AttackEnd()
{
    actionState = EactionState::EAS_Unoccupied;
    attackNumber = 0;
}

void ACombatPlayerCharacter::StartInputBuffer()
{
    actionState = EactionState::EAS_Comboing;
}

void ACombatPlayerCharacter::EndBuffer()
{
    actionState = EactionState::EAS_Unoccupied;
}

FName ACombatPlayerCharacter::GetCurrentAttack()
{
    attackNumber++;
    if (attackNumber > 3) attackNumber = 1;
    return FName("Attack " + FString::FromInt(attackNumber));
}

void ACombatPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACombatPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent =
        CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Jump);
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Interact);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Attack);
        EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Dodge);
    }
}

void ACombatPlayerCharacter::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
    if (equippedWeapon && equippedWeapon->GetWeaponBoxComponent())
    {
        equippedWeapon->GetWeaponBoxComponent()->SetCollisionEnabled(CollisionEnabled);
        //equippedWeapon->ignoreActors.Empty();
    }
}

//
// -------- Pickup Interface Overrides --------
//

void ACombatPlayerCharacter::SetOverlappingItem_Implementation(AItem* Item)
{
    overlappingItem = Item;
}

