
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
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

// Rhya.Debug.Combat 1 — floating hit/block text with the
// verdict's signal source, attacker arrows, claimed impact points, and the block
// cone while blocking.
static TAutoConsoleVariable<int32> CVarCombatDebug(
    TEXT("Rhya.Debug.Combat"), 0,
    TEXT("Combat debug overlay: 1 = floating hit/block verdict text, damage numbers, block cone."),
    ECVF_Cheat);

static void DrawCombatText(const AActor* Anchor, const FString& Text, const FColor& Color)
{
    static uint32 Slot = 0;
    Slot = (Slot + 1) % 3;
    DrawDebugString(Anchor->GetWorld(),
                    Anchor->GetActorLocation() + FVector(0.f, 0.f, 120.f + Slot * 26.f),
                    Text, nullptr, Color, 1.8f, /*bDrawShadow*/ true, /*FontScale*/ 1.25f);
}

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

    GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Red, FString::Printf(TEXT("%f"), movementVector.Size()));


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

void ACombatPlayerCharacter::SetAttackNumber(int AttackNumber)
{
    comboIndex = AttackNumber;
    OnComboIndexChanged.Broadcast(comboIndex);
}

int ACombatPlayerCharacter::GetAttackNumber()
{
    return comboIndex;
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

        equippedWeapon->ignoreActors.Empty();

        PlayAttackMontage();
        actionState = EactionState::EAS_Attacking;
    }
}

void ACombatPlayerCharacter::DefensiveAction(const FInputActionValue& Value)
{
    if (actionState == EactionState::EAS_Dodging || actionState == EactionState::EAS_Blocking)
        return;
    if (actionState == EactionState::EAS_Comboing || actionState == EactionState::EAS_Attacking)
        AttackEnd();

    GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Red, FString::Printf(TEXT("Input : %s"), *GetCharacterMovement()->GetLastInputVector().ToString()));

    if (GetCharacterMovement()->GetLastInputVector() != FVector::Zero())
        StartDodge();
    else
    {
        StartBlock();
    }




}

void ACombatPlayerCharacter::StartDodge()
{
    actionState = EactionState::EAS_Dodging;


    SetActorRotation(UKismetMathLibrary::Conv_VectorToRotator(GetLastMovementInputVector()));

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(DodgeMontage);
    }
}

void ACombatPlayerCharacter::EndDodge()
{
    actionState = EactionState::EAS_Unoccupied;
}

void ACombatPlayerCharacter::StartBlock()
{
    actionState = EactionState::EAS_Blocking;

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(BlockMontage);
    }
}

void ACombatPlayerCharacter::EndBlock()
{
    actionState = EactionState::EAS_Unoccupied;
}

bool ACombatPlayerCharacter::CanAttack()
{
    return (actionState == EactionState::EAS_Unoccupied ||
        actionState == EactionState::EAS_Comboing)
        && state != ECharacterState::ECS_Unequipped;
}

float ACombatPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                         AController* EventInstigator, AActor* DamageCauser)
{
    const bool bCombatDebug = CVarCombatDebug.GetValueOnGameThread() != 0;

    if (actionState == EactionState::EAS_Blocking)
    {
        bool bBlocked = false;
        const TCHAR* VerdictSource = TEXT("");
        if (DamageCauser)
        {
            // DamageCauser is the weapon/projectile, its Owner the wielder. Judge by
            // position, not impact point — tick-stepped projectiles tunnel past the
            // capsule and read as "rear".
            const AActor* Attacker = DamageCauser->GetOwner() ? DamageCauser->GetOwner() : DamageCauser;
            bBlocked = IsAttackerInBlockCone(Attacker);
            VerdictSource = TEXT("causer position");
            if (bCombatDebug)
            {
                DrawDebugDirectionalArrow(GetWorld(),
                    Attacker->GetActorLocation() + FVector(0.f, 0.f, 50.f),
                    GetActorLocation() + FVector(0.f, 0.f, 50.f),
                    60.f, bBlocked ? FColor::Green : FColor::Red, false, 1.8f, 0, 2.f);
            }
        }
        else if (LastBlockVerdictFrame == GFrameCounter)
        {
            // No causer: use the verdict GetHit published this frame.
            bBlocked = bBlockedLastHit;
            VerdictSource = TEXT("GetHit impact point");
        }
        else
        {
            // No directional info at all (bare ApplyDamage, no GetHit this frame):
            // favor the block — a held stance failing on missing data feels broken.
            bBlocked = true;
            VerdictSource = TEXT("NO directional info -> block favored");
        }

        // Publish so a following GetHit picks the matching react.
        bBlockedLastHit = bBlocked;
        LastBlockVerdictFrame = GFrameCounter;

        if (bBlocked)
        {
            const float ChipDamage = DamageAmount * BlockedDamageMultiplier;
            if (bCombatDebug)
            {
                DrawCombatText(this, FString::Printf(TEXT("BLOCKED %.1f -> %.1f  [%s]"),
                    DamageAmount, ChipDamage, VerdictSource),
                    ChipDamage > 0.f ? FColor::Yellow : FColor::Green);
            }
            if (ChipDamage <= 0.f)
            {
                // No Super: OnTakeAnyDamage (BP AnyDamage -> ReceiveDamage) never fires.
                return 0.f;
            }
            return Super::TakeDamage(ChipDamage, DamageEvent, EventInstigator, DamageCauser);
        }

        // The block break is decided here; GetHit never resets a blocking player's state.
        actionState = EactionState::EAS_Unoccupied;
        if (bCombatDebug)
        {
            DrawCombatText(this, FString::Printf(TEXT("BLOCK BROKEN  %.1f dmg  [%s]"),
                DamageAmount, VerdictSource), FColor::Red);
        }
    }
    else if (bCombatDebug)
    {
        const FString From = DamageCauser
            ? FString::Printf(TEXT("from %s"), *DamageCauser->GetName())
            : FString(TEXT("[no causer]"));
        DrawCombatText(this, FString::Printf(TEXT("HIT %.1f  %s"), DamageAmount, *From), FColor::Red);
    }

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool ACombatPlayerCharacter::IsAttackerInBlockCone(const AActor* Attacker) const
{
    return Attacker && IsDirectionInBlockCone(Attacker->GetActorLocation() - GetActorLocation());
}

bool ACombatPlayerCharacter::IsDirectionInBlockCone(const FVector& ToSource) const
{
    FVector Flat = ToSource;
    Flat.Z = 0.f;
    const FVector Dir = Flat.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        // Source directly above/below: not blocked.
        return false;
    }

    const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(BlockAngleDegrees));
    return FVector::DotProduct(GetActorForwardVector(), Dir) >= CosThreshold;
}

void ACombatPlayerCharacter::GetHit_Implementation(const FVector& impactPoint, const FVector& impactDirection)
{
    // GetHit only picks the react. Consume a same-frame verdict from TakeDamage
    // if one exists; otherwise judge here and publish for the TakeDamage that follows.
    bool bBlockedThisHit = false;
    if (LastBlockVerdictFrame == GFrameCounter)
    {
        bBlockedThisHit = bBlockedLastHit;
    }
    else
    {
        bBlockedThisHit = actionState == EactionState::EAS_Blocking &&
                          IsDirectionInBlockCone(impactPoint - GetActorLocation());
        bBlockedLastHit = bBlockedThisHit;
        LastBlockVerdictFrame = GFrameCounter;
    }

    if (CVarCombatDebug.GetValueOnGameThread() != 0)
    {
        // Dealer-claimed impact point, colored by verdict.
        DrawDebugSphere(GetWorld(), impactPoint, 8.f, 8,
            bBlockedThisHit ? FColor::Green : FColor::Red, false, 1.8f);
    }

    if (bBlockedThisHit)
    {
        CharacterBlockedHit(impactPoint, impactDirection);
        PlayBlockImpactMontage();
        // actionState stays EAS_Blocking: the next hit blocks without re-pressing.
        return;
    }

    CharacterHit(impactPoint, impactDirection);

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (HitReactMontage)
        {
            AnimInstance->Montage_Play(HitReactMontage);
            if (actionState != EactionState::EAS_Blocking)
            {
                actionState = EactionState::EAS_Unoccupied;
            }
        }
    }
}

void ACombatPlayerCharacter::PlayBlockImpactMontage()
{
    if (!ensureMsgf(BlockImpactMontage, TEXT("%s: BlockImpactMontage is not assigned — blocked hits play no react"), *GetName()))
    {
        return;
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(BlockImpactMontage);
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &ACombatPlayerCharacter::OnBlockImpactMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, BlockImpactMontage);
    }
}

void ACombatPlayerCharacter::OnBlockImpactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (!bInterrupted && actionState == EactionState::EAS_Blocking && BlockMontage)
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            AnimInstance->Montage_Play(BlockMontage);
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
    SetAttackNumber(0);
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
    SetAttackNumber(comboIndex+1);
    if (comboIndex > 3) comboIndex = 1;
    return FName("Attack " + FString::FromInt(comboIndex));
}

void ACombatPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Block-cone overlay: the edge lines are the exact bounds the verdict uses.
    if (actionState == EactionState::EAS_Blocking && CVarCombatDebug.GetValueOnGameThread() != 0)
    {
        const FVector Center = GetActorLocation() - FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() - 10.f);
        const FVector Forward = GetActorForwardVector();
        constexpr float Radius = 150.f;
        const FVector LeftEdge = Forward.RotateAngleAxis(-BlockAngleDegrees, FVector::UpVector);
        const FVector RightEdge = Forward.RotateAngleAxis(BlockAngleDegrees, FVector::UpVector);
        DrawDebugLine(GetWorld(), Center, Center + LeftEdge * Radius, FColor::Cyan, false, -1.f, 0, 1.5f);
        DrawDebugLine(GetWorld(), Center, Center + RightEdge * Radius, FColor::Cyan, false, -1.f, 0, 1.5f);
        DrawDebugCircleArc(GetWorld(), Center, Radius, Forward,
            FMath::DegreesToRadians(BlockAngleDegrees), 24, FColor::Cyan, false, -1.f, 0, 1.5f);
    }
}

void ACombatPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent =
        CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Look);
        //EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Jump);
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Interact);
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::Attack);
        EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &ACombatPlayerCharacter::DefensiveAction);
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

