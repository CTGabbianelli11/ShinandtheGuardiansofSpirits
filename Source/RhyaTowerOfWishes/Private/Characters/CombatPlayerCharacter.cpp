
// CPPCharacter.cpp
#include "Characters/CombatPlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
// EnhancedInput inline code narrows double->float; engine-owned, exempt from UnsafeTypeCastWarningLevel.
PRAGMA_DISABLE_UNSAFE_TYPECAST_WARNINGS
#include "EnhancedInputSubsystems.h"
#include <EnhancedInputComponent.h>
PRAGMA_RESTORE_UNSAFE_TYPECAST_WARNINGS
#include "Items/Item.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Components/BoxComponent.h"

#include "Kismet/GameplayStatics.h"
#include <Kismet/KismetMathLibrary.h>
#include "RhyaTowerOfWishes/DebugMacros.h"
#include <Components/TimelineComponent.h>
#include <Components/AC_HitStop.h>
#include <Enemy/Enemy.h>
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

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
        (float)movementVector.Y
    );

    AddMovementInput(
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y),
        (float)movementVector.X
    );
}

void ACombatPlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D lookValue = Value.Get<FVector2D>();
    AddControllerYawInput((float)lookValue.X);
    AddControllerPitchInput((float)-lookValue.Y);
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

void ACombatPlayerCharacter::SetActionState(EactionState ActionState)
{
    actionState = ActionState;
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
            (float)movementVector.Y
        );

        AddMovementInput(
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y),
            (float)movementVector.X
        );

        equippedWeapon->ignoreActors.Empty();

        PlayAttackMontage();
    }
}

void ACombatPlayerCharacter::DefensiveAction(const FInputActionValue& Value)
{
    if (actionState == EactionState::EAS_Dodging)
        return;
    // A dodge cancels a heal channel before rolling.
    if (actionState == EactionState::EAS_Healing)
        StopHeal();
    if (actionState == EactionState::EAS_Comboing || actionState == EactionState::EAS_Attacking)
        AttackEnd();

    // Need a movement direction: StartDodge rotates toward it, so a neutral press has nothing
    // to roll toward. Poll MoveAction directly — Move() suppresses AddMovementInput while
    // blocking, so GetLastInputVector() reads zero there and the dodge-cancel would never fire.
    if (!GetMoveInputWorldDirection().IsZero())
        StartDodge();
}

void ACombatPlayerCharacter::StartDodge()
{
    SetActionState( EactionState::EAS_Dodging);

    const FVector DodgeDir = GetMoveInputWorldDirection();
    if (!DodgeDir.IsZero())
    {
        SetActorRotation(DodgeDir.Rotation());
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(DodgeMontage);
    }
}

void ACombatPlayerCharacter::EndDodge()
{
    // A dodge out of block returns to block if the button is still held (Souls-style).
    ResumeBlockIfHeld();
}

void ACombatPlayerCharacter::StartBlock()
{
    SetActionState( EactionState::EAS_Blocking);

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(BlockMontage);
    }
}

void ACombatPlayerCharacter::EndBlock()
{
    if (actionState != EactionState::EAS_Blocking)
        return;

    SetActionState(EactionState::EAS_Unoccupied);

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        // run the last section of the montage to ease out of the block.
        if (AnimInstance->Montage_IsPlaying(BlockMontage)) 
        {
            AnimInstance->Montage_SetNextSection(TEXT("Hold"), TEXT("Release"));
        }
        AnimInstance->Montage_Stop(0.15f, BlockImpactMontage);
    }
}

bool ACombatPlayerCharacter::CanHeal() const
{
    // Neutral stance, alive, and missing health. The no-magic case isn't gated here —
    // the first HealTick spends magic and stops immediately if it can't afford a pulse.
    return actionState == EactionState::EAS_Unoccupied
        && attributeComponent
        && attributeComponent->IsAlive()
        && !attributeComponent->IsHealthFull();
}

void ACombatPlayerCharacter::StartHeal()
{
    if (!CanHeal())
    {
        if (CVarCombatDebug.GetValueOnGameThread() != 0 && attributeComponent && attributeComponent->IsHealthFull())
        {
            DrawCombatText(this, TEXT("Health full"), FColor::Yellow);
        }
        return;
    }

    SetActionState(EactionState::EAS_Healing);

    if (HealMontage)
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            AnimInstance->Montage_Play(HealMontage);
        }
    }

    // Pulse immediately so a held press feels responsive, but only if a full HealInterval has
    // elapsed since the last pulse — otherwise tapping the button would out-heal holding it.
    // Timer-driven pulses are paced by the timer itself, so they aren't gated this way.
    if (GetWorld()->GetTimeSeconds() - LastHealPulseTime >= HealInterval)
    {
        HealTick();
    }
    if (actionState == EactionState::EAS_Healing)
    {
        GetWorldTimerManager().SetTimer(HealTimerHandle, this,
            &ACombatPlayerCharacter::HealTick, HealInterval, /*bLoop*/ true);
    }
}

void ACombatPlayerCharacter::HealTick()
{
    // The timer outlives the heal state (a hit-react or dodge can flip actionState
    // without touching the timer); stop ourselves if we're no longer healing.
    if (actionState != EactionState::EAS_Healing)
    {
        StopHeal();
        return;
    }

    // Check fullness before spending: a wasted pulse on a full bar feels like a bug.
    if (!attributeComponent || attributeComponent->IsHealthFull())
    {
        StopHeal();
        return;
    }

    // RemoveMagic returns false when the next pulse is unaffordable — the natural end of a channel.
    if (!attributeComponent->RemoveMagic(MagicPerTick))
    {
        if (CVarCombatDebug.GetValueOnGameThread() != 0)
        {
            DrawCombatText(this, TEXT("Not enough magic"), FColor::Yellow);
        }
        StopHeal();
        return;
    }

    attributeComponent->AddHealth(HealPerTick);
    LastHealPulseTime = GetWorld()->GetTimeSeconds();

    if (CVarCombatDebug.GetValueOnGameThread() != 0)
    {
        DrawCombatText(this, FString::Printf(TEXT("HEAL +%.0f  (-%.0f magic)"),
            HealPerTick, MagicPerTick), FColor::Green);
    }
}

void ACombatPlayerCharacter::StopHeal()
{
    GetWorldTimerManager().ClearTimer(HealTimerHandle);

    if (actionState == EactionState::EAS_Healing)
    {
        actionState = EactionState::EAS_Unoccupied;
    }

    if (HealMontage)
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            if (AnimInstance->Montage_IsPlaying(HealMontage))
            {
                AnimInstance->Montage_Stop(0.15f, HealMontage);
            }
        }
    }
}

bool ACombatPlayerCharacter::IsBlockHeld() const
{
    // Poll the live input value rather than mirror it in a flag — one source of truth,
    // so block intent can't drift out of sync. Needs BindActionValue(BlockAction) in setup.
    if (const UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        return EnhancedInputComponent->GetBoundActionValue(BlockAction).Get<bool>();
    }
    return false;
}

void ACombatPlayerCharacter::ResumeBlockIfHeld()
{
    if (IsBlockHeld())
    {
        StartBlock();
    }
    else
    {
        SetActionState(EactionState::EAS_Unoccupied);
    }
}

FVector ACombatPlayerCharacter::GetMoveInputWorldDirection() const
{
    if (const UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        const FVector2D Move = EnhancedInputComponent->GetBoundActionValue(MoveAction).Get<FVector2D>();
        if (!Move.IsNearlyZero())
        {
            // Match Move()'s mapping: stick Y -> forward, X -> right, in control-yaw space.
            const FRotator YawOnly(0.f, GetControlRotation().Yaw, 0.f);
            const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
            const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
            return (Fwd * Move.Y + Right * Move.X).GetSafeNormal();
        }
    }
    return FVector::ZeroVector;
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
        if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
        {
            // The attack supplied its travel direction (ApplyPointDamage); judge the block
            // against it. Position guessing is wrong for attacks that travel away from
            // their owner, e.g. a sliding wall.
            const FPointDamageEvent& PointEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
            bBlocked = IsDirectionInBlockCone(-PointEvent.ShotDirection);
            VerdictSource = TEXT("point-damage direction");
            if (bCombatDebug)
            {
                DrawDebugDirectionalArrow(GetWorld(),
                    GetActorLocation() - PointEvent.ShotDirection * 150.f + FVector(0.f, 0.f, 50.f),
                    GetActorLocation() + FVector(0.f, 0.f, 50.f),
                    60.f, bBlocked ? FColor::Green : FColor::Red, false, 1.8f, SDPG_Foreground, 2.f);
            }
        }
        else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
        {
            // The attack radiates from an epicenter (ApplyRadialDamage); face it to block.
            // An epicenter inside the capsule's own footprint leaves no direction to face,
            // so the held stance wins there rather than failing on geometry noise.
            const FRadialDamageEvent& RadialEvent = static_cast<const FRadialDamageEvent&>(DamageEvent);
            const FVector ToEpicenter = RadialEvent.Origin - GetActorLocation();
            const float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
            bBlocked = ToEpicenter.SizeSquared2D() <= FMath::Square(CapsuleRadius)
                || IsDirectionInBlockCone(ToEpicenter);
            VerdictSource = TEXT("radial-damage epicenter");
            if (bCombatDebug)
            {
                DrawDebugDirectionalArrow(GetWorld(),
                    RadialEvent.Origin + FVector(0.f, 0.f, 50.f),
                    GetActorLocation() + FVector(0.f, 0.f, 50.f),
                    60.f, bBlocked ? FColor::Green : FColor::Red, false, 1.8f, SDPG_Foreground, 2.f);
            }
        }
        else if (DamageCauser)
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
                    60.f, bBlocked ? FColor::Green : FColor::Red, false, 1.8f, SDPG_Foreground, 2.f);
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
        SetActionState(EactionState::EAS_Unoccupied);
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

void ACombatPlayerCharacter::GetHit_Implementation(const FVector& impactPoint, const FVector& impactDirection, const float& damage )
{
    // A hit interrupts a heal channel (you were never blocking, so this is an unblocked hit).
    if (actionState == EactionState::EAS_Healing)
        StopHeal();

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
            bBlockedThisHit ? FColor::Green : FColor::Red, false, 1.8f, SDPG_Foreground);
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
            // An unblocked hit breaks whatever you were doing, including a held guard.
            SetActionState(EactionState::EAS_Unoccupied);
            // Re-raise the guard once the stagger ends, if block is still held.
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &ACombatPlayerCharacter::OnHitReactMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactMontage);
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

void ACombatPlayerCharacter::OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // Resume only on a clean finish; if another hit or a dodge interrupted the stagger,
    // that action owns the next state.
    if (!bInterrupted)
    {
        ResumeBlockIfHeld();
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
    //There should always be a base attack montage to default to. Otherwise stop and tell user
    if (!AttackMontage)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("No Base AttackMontage Set. Aborting"));
        return;
    }

    UAnimMontage* AttackMontageToUse = AttackMontage;

    if (AerialAttackMontage && !GetCharacterMovement()->IsMovingOnGround())
    {
        AttackMontageToUse = AerialAttackMontage;
    }
    if (CurrentAttackTarget && GetCharacterMovement()->IsMovingOnGround() && Cast<AActor>(CurrentAttackTarget)->GetActorLocation().Z > GetActorLocation().Z + 100.f)
    {
        AttackMontageToUse = GroundToAirAttackMontage;
    }
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (AttackMontageToUse)
        {
            AnimInstance->Montage_Play(AttackMontageToUse);
            AnimInstance->Montage_JumpToSection(GetCurrentAttack(), AttackMontageToUse);
        }

    }
}

void ACombatPlayerCharacter::AttackEnd()
{
    SetActionState(EactionState::EAS_Unoccupied);
    SetAttackNumber(0);
}

void ACombatPlayerCharacter::StartInputBuffer()
{
    SetActionState(EactionState::EAS_Comboing);
}

void ACombatPlayerCharacter::EndBuffer()
{
    SetActionState(EactionState::EAS_Unoccupied);
}

FName ACombatPlayerCharacter::GetCurrentAttack()
{

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
        DrawDebugLine(GetWorld(), Center, Center + LeftEdge * Radius, FColor::Cyan, false, -1.f, SDPG_Foreground, 1.5f);
        DrawDebugLine(GetWorld(), Center, Center + RightEdge * Radius, FColor::Cyan, false, -1.f, SDPG_Foreground, 1.5f);
        DrawDebugCircleArc(GetWorld(), Center, Radius, Forward,
            FMath::DegreesToRadians(BlockAngleDegrees), 24, FColor::Cyan, false, -1.f, SDPG_Foreground, 1.5f);
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
        // IA_Block uses a Down trigger: Started fires once on press, Completed on real
        // release. (Triggered would re-fire every held frame and restart the montage.)
        EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &ACombatPlayerCharacter::StartBlock);
        EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &ACombatPlayerCharacter::EndBlock);
        // IA_Heal mirrors block's hold pattern: Started begins the channel, Completed ends it.
        EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started, this, &ACombatPlayerCharacter::StartHeal);
        EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Completed, this, &ACombatPlayerCharacter::StopHeal);
        // Value bindings (no callback) so we can poll live input state: block-held for resume
        // decisions, move direction for dodging out of states where Move() suppresses movement.
        EnhancedInputComponent->BindActionValue(BlockAction);
        EnhancedInputComponent->BindActionValue(MoveAction);
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

