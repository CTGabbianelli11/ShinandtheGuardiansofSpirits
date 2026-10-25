
#include "Characters/CombatPlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogPlayerActions, Log, All);

static TAutoConsoleVariable<int32> CVarActionDebug(
    TEXT("Rhya.Debug.Actions"), UE_BUILD_SHIPPING ? 0 : 1,
    TEXT("Player action debugging: 0 = off, 1 = persistent state display and transition logs, "
         "2 = also log notify Begin/End, interruptions, and ignored callbacks."),
    ECVF_Cheat);

static FString ActionStateName(EactionState State)
{
    FString Name = StaticEnum<EactionState>()->GetNameStringByValue(static_cast<int64>(State));
    Name.RemoveFromStart(TEXT("EAS_"));
    return Name;
}

static FString NotifyInterruptName(EGameplayNotifyInterruptReason Reason)
{
    return StaticEnum<EGameplayNotifyInterruptReason>()->GetNameStringByValue(static_cast<int64>(Reason));
}

static uint64 ActionDebugScreenKey(const ACombatPlayerCharacter* Player, bool bEvent)
{
    // Separate keys per actor and row: refresh in place, including multiple PIE players.
    return 0x5248590000000000ULL | (static_cast<uint64>(Player->GetUniqueID()) << 1) | (bEvent ? 1ULL : 0ULL);
}

static void LogActionDebug(const ACombatPlayerCharacter* Player, const FString& Message, int32 RequiredLevel = 1)
{
    if (CVarActionDebug.GetValueOnGameThread() < RequiredLevel) return;
    UE_LOG(LogPlayerActions, Log, TEXT("[%s][%s][%.3fs] %s"),
        *GetNameSafe(Player->GetWorld()), *Player->GetName(),
        Player->GetWorld() ? Player->GetWorld()->GetTimeSeconds() : 0.0, *Message);
}

static void ShowActionDebugEvent(const ACombatPlayerCharacter* Player, const FString& Message, FColor Color)
{
    LogActionDebug(Player, Message);
    if (CVarActionDebug.GetValueOnGameThread() == 0 || !GEngine
        || !Player->IsPlayerControlled() || !Player->IsLocallyControlled()) return;
    GEngine->AddOnScreenDebugMessage(ActionDebugScreenKey(Player, true), 3.f, Color,
        FString::Printf(TEXT("[%s] %s"), *Player->GetName(), *Message));
}

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

    //GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Red, FString::Printf(TEXT("%f"), movementVector.Size()));


    const FRotator controlRotation = GetControlRotation();
    YawRotation = FRotator(0.f, controlRotation.Yaw, 0.f);

    if (!IsActionInputAllowed() || actionState != EactionState::EAS_Unoccupied)
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
    if (!IsActionInputAllowed()) return;
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
    // A late combo notify may clear its window, but cannot arm another combo in stagger.
    if (AttackNumber != 0 && !IsActionInputAllowed()) return;
    comboIndex = AttackNumber;
    OnComboIndexChanged.Broadcast(comboIndex);
}

void ACombatPlayerCharacter::SetActionState(EactionState ActionState)
{
    // EnterStagger and FinishStagger own stagger transitions; animation markers cannot override them.
    if (!IsActionInputAllowed() || ActionState == EactionState::EAS_Staggered)
    {
        LogActionDebug(this, FString::Printf(TEXT("Ignored SetActionState(%s); current=%s, alive=%d"),
            *ActionStateName(ActionState), *ActionStateName(actionState), IsCombatAlive()), 2);
        return;
    }
    // The existing dodge montage has both this marker and EndDodgeRoll. Recover once.
    if (actionState == EactionState::EAS_Dodging && ActionState == EactionState::EAS_Unoccupied)
    {
        EndDodge();
        return;
    }
    if (ActionState != EactionState::EAS_Dodging) bDodgeInvulnerable = false;
    ChangeActionState(ActionState, TEXT("SetActionState / animation callback"));
}

void ACombatPlayerCharacter::ChangeActionState(EactionState NewState, const TCHAR* Reason)
{
    const EactionState PreviousState = actionState;
    actionState = NewState;
    if (PreviousState == NewState) return;
    ShowActionDebugEvent(this, FString::Printf(TEXT("%s -> %s (%s)"),
        *ActionStateName(PreviousState), *ActionStateName(NewState), Reason),
        NewState == EactionState::EAS_Staggered ? FColor::Red : FColor::Cyan);
}

bool ACombatPlayerCharacter::IsCombatAlive() const
{
    return !bCombatDead && !IsActorBeingDestroyed() && attributeComponent && attributeComponent->IsAlive();
}

bool ACombatPlayerCharacter::IsActionInputAllowed() const
{
    return IsCombatAlive() && actionState != EactionState::EAS_Staggered;
}

bool ACombatPlayerCharacter::CanStartAction(EactionState RequestedAction) const
{
    if (!IsActionInputAllowed()) return false;
    switch (RequestedAction)
    {
    case EactionState::EAS_Attacking:
        return (actionState == EactionState::EAS_Unoccupied || actionState == EactionState::EAS_Comboing)
            && state != ECharacterState::ECS_Unequipped && IsValid(equippedWeapon);
    case EactionState::EAS_Dodging:
        return actionState != EactionState::EAS_Dodging;
    case EactionState::EAS_Blocking:
        return actionState != EactionState::EAS_Dodging && actionState != EactionState::EAS_Blocking;
    case EactionState::EAS_Healing:
        // HealTick spends magic and stops healing if it cannot afford a pulse.
        return actionState == EactionState::EAS_Unoccupied && !attributeComponent->IsHealthFull();
    case EactionState::EAS_Unoccupied:
    case EactionState::EAS_Comboing:
    case EactionState::EAS_Staggered:
    default:
        // Unoccupied/Comboing are recovery/animation phases, not input actions.
        return false;
    }
}

bool ACombatPlayerCharacter::TryStartAction(EactionState RequestedAction)
{
    if (!CanStartAction(RequestedAction)) return false;
    const int RequestedComboIndex = comboIndex;
    ChangeActionState(RequestedAction, TEXT("TryStartAction"));
    CancelCurrentAction(EGameplayNotifyInterruptReason::ActionCancelled);
    // Cancellation can dispatch callbacks that change the state or destroy the character.
    if (!IsActionInputAllowed() || actionState != RequestedAction) return false;
    // Outgoing notify ends may reset comboIndex while the next combo attack starts.
    if (RequestedAction == EactionState::EAS_Attacking) SetAttackNumber(RequestedComboIndex);
    return true;
}

void ACombatPlayerCharacter::Jump()
{
    if (IsActionInputAllowed()) Super::Jump();
}

void ACombatPlayerCharacter::SetDodgeInvulnerable(bool bEnabled)
{
    bDodgeInvulnerable = bEnabled && IsActionInputAllowed() && actionState == EactionState::EAS_Dodging;
}

void ACombatPlayerCharacter::RegisterAnimNotifyState(const UAnimNotifyState* NotifyState,
    const FAnimNotifyEventReference& EventReference)
{
    if (!ensureMsgf(IsValid(NotifyState), TEXT("RegisterAnimNotifyState requires the ASN's Self"))) return;
    const auto* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
    const int32 MontageInstanceID = MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE;
    ActiveGameplayNotifies.RemoveAll([](const FActiveGameplayNotify& Entry) { return !Entry.NotifyState.IsValid(); });
    if (ActiveGameplayNotifies.ContainsByPredicate([&](const FActiveGameplayNotify& Entry)
        { return Entry.NotifyState == NotifyState && Entry.MontageInstanceID == MontageInstanceID; }))
    {
        // The same window is already registered; do not overwrite its interruption reason.
        LogActionDebug(this, FString::Printf(TEXT("Notify Begin duplicate: %s (montage %d); original reason retained"),
            *GetNameSafe(NotifyState->GetClass()), MontageInstanceID), 2);
        return;
    }
    ActiveGameplayNotifies.Add({ NotifyState, MontageInstanceID,
        IsCombatAlive() ? EGameplayNotifyInterruptReason::None : EGameplayNotifyInterruptReason::Death });
    LogActionDebug(this, FString::Printf(TEXT("Notify Begin: %s (montage %d), state=%s, active=%d"),
        *GetNameSafe(NotifyState->GetClass()), MontageInstanceID, *ActionStateName(actionState), ActiveGameplayNotifies.Num()), 2);
}

EGameplayNotifyInterruptReason ACombatPlayerCharacter::ConsumeAnimNotifyStateInterruptReason(const UAnimNotifyState* NotifyState,
    const FAnimNotifyEventReference& EventReference)
{
    const auto MissingReason = IsCombatAlive()
        ? EGameplayNotifyInterruptReason::ActionCancelled : EGameplayNotifyInterruptReason::Death;
    if (!IsValid(NotifyState)) return MissingReason;
    const auto* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
    const int32 MontageInstanceID = MontageContext ? MontageContext->MontageInstanceID : INDEX_NONE;
    const int32 Index = ActiveGameplayNotifies.IndexOfByPredicate([&](const FActiveGameplayNotify& Entry)
        { return Entry.NotifyState == NotifyState && Entry.MontageInstanceID == MontageInstanceID; });
    if (Index == INDEX_NONE)
    {
        LogActionDebug(this, FString::Printf(TEXT("Notify End without registration: %s (montage %d), reason=%s"),
            *GetNameSafe(NotifyState->GetClass()), MontageInstanceID, *NotifyInterruptName(MissingReason)), 2);
        return MissingReason;
    }

    auto InterruptReason = ActiveGameplayNotifies[Index].InterruptReason;
    ActiveGameplayNotifies.RemoveAtSwap(Index);
    // Death overrides any earlier interruption reason.
    if (!IsCombatAlive()) InterruptReason = EGameplayNotifyInterruptReason::Death;
    // Stops and section jumps outside the action API must also skip normal completion.
    if (InterruptReason == EGameplayNotifyInterruptReason::None
        && !UAnimNotifyLibrary::NotifyStateReachedEnd(EventReference))
        InterruptReason = EGameplayNotifyInterruptReason::ActionCancelled;
    LogActionDebug(this, FString::Printf(TEXT("Notify End: %s (montage %d), reason=%s, state=%s, remaining=%d"),
        *GetNameSafe(NotifyState->GetClass()), MontageInstanceID, *NotifyInterruptName(InterruptReason),
        *ActionStateName(actionState), ActiveGameplayNotifies.Num()), 2);
    return InterruptReason;
}

void ACombatPlayerCharacter::InterruptGameplayNotifies(EGameplayNotifyInterruptReason InterruptReason)
{
    LogActionDebug(this, FString::Printf(TEXT("Interrupt notify windows: reason=%s, active=%d"),
        *NotifyInterruptName(InterruptReason), ActiveGameplayNotifies.Num()), 2);
    switch (InterruptReason)
    {
    case EGameplayNotifyInterruptReason::None:
        return;
    case EGameplayNotifyInterruptReason::Staggered:
    case EGameplayNotifyInterruptReason::ActionCancelled:
        for (FActiveGameplayNotify& Entry : ActiveGameplayNotifies)
        {
            // Keep the original cause while an interrupted montage is blending out.
            if (Entry.InterruptReason == EGameplayNotifyInterruptReason::None)
                Entry.InterruptReason = InterruptReason;
        }
        return;
    case EGameplayNotifyInterruptReason::Death:
        // Death supersedes earlier reasons so cleanup cannot restore locomotion on a corpse.
        for (FActiveGameplayNotify& Entry : ActiveGameplayNotifies) Entry.InterruptReason = InterruptReason;
        return;
    default:
        checkNoEntry();
        return;
    }
}

void ACombatPlayerCharacter::CancelCurrentAction(EGameplayNotifyInterruptReason InterruptReason)
{
    // Publish the reason before Stop dispatches any End callbacks. ASNs own their
    // effects until End, including the brief weapon trade window during blend-out.
    InterruptGameplayNotifies(InterruptReason);
    ++DodgeGeneration;
    bDodgeInvulnerable = false;
    GetWorldTimerManager().ClearTimer(HealTimerHandle);
    StopJumping();
    ConsumeMovementInputVector();
    GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        AnimInstance->Montage_Stop(0.1f);
}

void ACombatPlayerCharacter::EnterStagger(bool bWaitForLanding)
{
    if (!IsCombatAlive()) return;
    if (actionState == EactionState::EAS_Staggered && !bRestartStaggerOnHit)
    {
        LogActionDebug(this, TEXT("Hit during stagger: keeping original recovery deadline"));
        return;
    }

    // Invalidate old recovery before stopping: Stop can synchronously dispatch callbacks.
    const uint32 Generation = ++StaggerGeneration;
    bWaitingForHitLanding = bWaitForLanding;
    if (actionState == EactionState::EAS_Staggered)
        ShowActionDebugEvent(this, TEXT("Stagger restarted by another hit"), FColor::Red);
    ChangeActionState(EactionState::EAS_Staggered, TEXT("Hit"));
    GetWorldTimerManager().ClearTimer(StaggerTimerHandle);
    CancelCurrentAction(EGameplayNotifyInterruptReason::Staggered);
    SetAttackNumber(0);
    // Stop horizontal attack momentum without changing vertical velocity.
    GetCharacterMovement()->Velocity.X = 0.f;
    GetCharacterMovement()->Velocity.Y = 0.f;
    if (!IsCombatAlive() || Generation != StaggerGeneration) return;

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (HitReactMontage && AnimInstance->Montage_Play(HitReactMontage) > 0.f)
        {
            if (bWaitingForHitLanding)
            {
                // Hold the final air pose without changing the shared montage asset.
                // Landed replaces this instance with the landing montage.
                if (FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(HitReactMontage))
                    Instance->bEnableAutoBlendOut = false;
                return;
            }
            FOnMontageEnded EndDelegate;
            EndDelegate.BindWeakLambda(this, [this, Generation](UAnimMontage*, bool)
            {
                // Unexpected interruption releases this stagger; a restart/death invalidates it.
                FinishStagger(Generation);
            });
            AnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactMontage);
            LogActionDebug(this, FString::Printf(TEXT("Stagger recovery follows montage %s (generation %u)"),
                *GetNameSafe(HitReactMontage), Generation));
            return;
        }
    }
    // Missing air animation must not release control while the launched capsule is airborne.
    if (bWaitingForHitLanding) return;
    FTimerDelegate Recovery;
    Recovery.BindWeakLambda(this, [this, Generation]() { FinishStagger(Generation); });
    GetWorldTimerManager().SetTimer(StaggerTimerHandle, Recovery,
        FMath::Max(StaggerFallbackDuration, 0.01f), false);
    ShowActionDebugEvent(this, FString::Printf(TEXT("Stagger: %.2fs fallback timer; hit-react montage missing or unplayable"),
        FMath::Max(StaggerFallbackDuration, 0.01f)), FColor::Orange);
}

void ACombatPlayerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    if (!bWaitingForHitLanding || actionState != EactionState::EAS_Staggered || !IsCombatAlive()) return;
    bWaitingForHitLanding = false;
    const uint32 Generation = StaggerGeneration;
    GetWorldTimerManager().ClearTimer(StaggerTimerHandle);
    // waffling on this: do we stop movement or not on land? let's keep the skid for now.
    // GetCharacterMovement()->Velocity.X = 0.f;
    // GetCharacterMovement()->Velocity.Y = 0.f;
    if (HitGroundedHoldDuration > 0.f)
    {
        FTimerDelegate BeginRecovery;
        BeginRecovery.BindWeakLambda(this, [this, Generation]() { StartHitLandingRecovery(Generation); });
        GetWorldTimerManager().SetTimer(StaggerTimerHandle, BeginRecovery, HitGroundedHoldDuration, false);
        LogActionDebug(this, TEXT("Hit landed: holding downed pose before recovery"));
        return;
    }
    StartHitLandingRecovery(Generation);
}

void ACombatPlayerCharacter::StartHitLandingRecovery(uint32 Generation)
{
    if (Generation != StaggerGeneration || bWaitingForHitLanding
        || actionState != EactionState::EAS_Staggered || !IsCombatAlive()) return;
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Stop(0.1f, HitReactMontage);
        if (LandHitReactMontage && AnimInstance->Montage_Play(LandHitReactMontage) > 0.f)
        {
            if (FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(LandHitReactMontage))
                Instance->bEnableAutoBlendOut = true;
            FOnMontageEnded EndDelegate;
            EndDelegate.BindWeakLambda(this, [this, Generation](UAnimMontage*, bool)
            {
                FinishStagger(Generation);
            });
            AnimInstance->Montage_SetEndDelegate(EndDelegate, LandHitReactMontage);
            LogActionDebug(this, TEXT("Hit landed: recovery follows landing montage"));
            return;
        }
    }
    FTimerDelegate Recovery;
    Recovery.BindWeakLambda(this, [this, Generation]() { FinishStagger(Generation); });
    GetWorldTimerManager().SetTimer(StaggerTimerHandle, Recovery,
        FMath::Max(StaggerFallbackDuration, 0.01f), false);
    LogActionDebug(this, TEXT("Hit landed: landing montage unavailable, using recovery timer"));
}

void ACombatPlayerCharacter::FinishStagger(uint32 Generation)
{
    if (Generation != StaggerGeneration || bWaitingForHitLanding || actionState != EactionState::EAS_Staggered || !IsCombatAlive())
    {
        LogActionDebug(this, FString::Printf(TEXT("Ignored stagger recovery: callback=%u, current=%u, state=%s, alive=%d"),
            Generation, StaggerGeneration, *ActionStateName(actionState), IsCombatAlive()), 2);
        return;
    }
    GetWorldTimerManager().ClearTimer(StaggerTimerHandle);
    ChangeActionState(EactionState::EAS_Unoccupied, TEXT("Stagger recovery"));
    ResumeBlockIfHeld();
}

void ACombatPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ++StaggerGeneration;
    ++DodgeGeneration;
    bCombatDead = true;
    InterruptGameplayNotifies(EGameplayNotifyInterruptReason::Death);
    GetWorldTimerManager().ClearTimer(StaggerTimerHandle);
    GetWorldTimerManager().ClearTimer(HealTimerHandle);
    Super::EndPlay(EndPlayReason);
    ActiveGameplayNotifies.Reset();
    if (GEngine)
    {
        GEngine->RemoveOnScreenDebugMessage(ActionDebugScreenKey(this, false));
        GEngine->RemoveOnScreenDebugMessage(ActionDebugScreenKey(this, true));
    }
}

int ACombatPlayerCharacter::GetAttackNumber()
{
    return comboIndex;
}


void ACombatPlayerCharacter::Attack(const FInputActionValue& /*Value*/)
{
    if (!TryStartAction(EactionState::EAS_Attacking)) return;

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

void ACombatPlayerCharacter::DefensiveAction(const FInputActionValue& Value)
{
    if (!CanStartAction(EactionState::EAS_Dodging)) return;

    // Need a movement direction: StartDodge rotates toward it, so a neutral press has nothing
    // to roll toward. Poll MoveAction directly — Move() suppresses AddMovementInput while
    // blocking, so GetLastInputVector() reads zero there and the dodge-cancel would never fire.
    if (!GetMoveInputWorldDirection().IsZero())
        StartDodge();
}

void ACombatPlayerCharacter::StartDodge()
{
    if (!TryStartAction(EactionState::EAS_Dodging)) return;
    const uint32 Generation = DodgeGeneration;
    SetAttackNumber(0);

    const FVector DodgeDir = GetMoveInputWorldDirection();
    if (!DodgeDir.IsZero())
    {
        SetActorRotation(DodgeDir.Rotation());
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (DodgeMontage && AnimInstance->Montage_Play(DodgeMontage) > 0.f)
        {
            FOnMontageEnded EndDelegate;
            EndDelegate.BindWeakLambda(this, [this, Generation](UAnimMontage*, bool)
            {
                // An early recovery marker can allow another dodge before this montage ends.
                if (Generation == DodgeGeneration) EndDodge();
            });
            AnimInstance->Montage_SetEndDelegate(EndDelegate, DodgeMontage);
            return;
        }
    }
    EndDodge();
}

void ACombatPlayerCharacter::EndDodge()
{
    if (actionState != EactionState::EAS_Dodging || !IsCombatAlive()) return;
    bDodgeInvulnerable = false;
    ChangeActionState(EactionState::EAS_Unoccupied, TEXT("Dodge recovery"));
    // A dodge out of block returns to block if the button is still held (Souls-style).
    ResumeBlockIfHeld();
}

void ACombatPlayerCharacter::StartBlock()
{
    if (!TryStartAction(EactionState::EAS_Blocking)) return;
    SetAttackNumber(0);

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
        if (BlockImpactMontage) AnimInstance->Montage_Stop(0.15f, BlockImpactMontage);
    }
}

void ACombatPlayerCharacter::StartHeal()
{
    if (!TryStartAction(EactionState::EAS_Healing))
    {
        if (CVarCombatDebug.GetValueOnGameThread() != 0 && attributeComponent && attributeComponent->IsHealthFull())
        {
            DrawCombatText(this, TEXT("Health full"), FColor::Yellow);
        }
        return;
    }

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
    // Animation callbacks can change the state before the next heal pulse.
    if (actionState != EactionState::EAS_Healing || !IsCombatAlive())
    {
        StopHeal();
        return;
    }

    // Check fullness before spending magic.
    if (!attributeComponent || attributeComponent->IsHealthFull())
    {
        StopHeal();
        return;
    }

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
        InterruptGameplayNotifies(EGameplayNotifyInterruptReason::ActionCancelled);
        ChangeActionState(EactionState::EAS_Unoccupied, TEXT("Heal stopped"));
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
        return EnhancedInputComponent->GetBoundActionValue(BlockAction).Get<bool>();
    return false;
}

void ACombatPlayerCharacter::ResumeBlockIfHeld()
{
    if (!IsActionInputAllowed()) return;
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
    return CanStartAction(EactionState::EAS_Attacking);
}

float ACombatPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                         AController* EventInstigator, AActor* DamageCauser)
{
    if (!IsCombatAlive() || (actionState == EactionState::EAS_Dodging && bDodgeInvulnerable))
        return 0.f;
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
    else
    {
        // A second hit in the same frame must not reuse an earlier successful block.
        bBlockedLastHit = false;
        LastBlockVerdictFrame = GFrameCounter;
        if (bCombatDebug)
        {
            const FString From = DamageCauser
                ? FString::Printf(TEXT("from %s"), *DamageCauser->GetName())
                : FString(TEXT("[no causer]"));
            DrawCombatText(this, FString::Printf(TEXT("HIT %.1f  %s"), DamageAmount, *From), FColor::Red);
        }
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
    // Damage dealers use both call orders. Never present a hurt/block reaction for a corpse
    // or an invulnerable dodge, even if damage was rejected before this notification.
    if (!IsCombatAlive() || !CanBeDamaged()
        || (actionState == EactionState::EAS_Dodging && bDodgeInvulnerable)) return;

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
        const FString Reaction = FString::Printf(TEXT("GetHit: %s -> %s"), *GetName(),
            bBlockedThisHit ? TEXT("CharacterBlockedHit") : TEXT("CharacterHit"));
        UE_LOG(LogTemp, Display, TEXT("%s"), *Reaction);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f,
                bBlockedThisHit ? FColor::Green : FColor::Red, Reaction);
        }
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

    if (!bEnableKnockbackHitReaction)
    {
        CharacterHit(impactPoint, impactDirection);
        return;
    }

    FVector ToImpact = impactDirection.GetSafeNormal2D();
    if (ToImpact.ContainsNaN() || ToImpact.IsNearlyZero())
        ToImpact = (impactPoint - GetActorLocation()).GetSafeNormal2D();
    // A centered/vertical hit has no horizontal source; recoil backward from current facing.
    if (ToImpact.ContainsNaN() || ToImpact.IsNearlyZero()) ToImpact = GetActorForwardVector().GetSafeNormal2D();
    FRotator Facing = GetActorRotation();
    Facing.Yaw = ToImpact.Rotation().Yaw;
    SetActorRotation(Facing);

    if (CVarCombatDebug.GetValueOnGameThread() != 0)
    {
        DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(),
            GetActorLocation() + ToImpact * 150.f, 20.f, FColor::Yellow, false, 2.f, SDPG_Foreground, 2.f);
        UE_LOG(LogPlayerActions, Log, TEXT("Hit facing: normal=%s contact=%s source=%s yaw=%.1f"),
            *impactDirection.ToCompactString(), *impactPoint.ToCompactString(),
            *ToImpact.ToCompactString(), Facing.Yaw);
    }

    const UCharacterMovementComponent* Movement = GetCharacterMovement();
    const bool bWillLaunch = FMath::IsFinite(damage) && damage > 0.f && HitLaunchUpSpeed > 0.f
        && Movement->IsActive() && Movement->MovementMode != MOVE_None;
    EnterStagger(LandHitReactMontage && bWillLaunch);
    if (!IsCombatAlive()) return;

    if (FMath::IsFinite(damage) && damage > 0.f)
    {
        const float Speed = FMath::Max(0.f, KnockbackSpeed);
        const FVector LaunchVelocity = -ToImpact * Speed + FVector::UpVector * FMath::Max(0.f, HitLaunchUpSpeed);
        LaunchCharacter(LaunchVelocity, true, true);
    }
    CharacterHit(impactPoint, impactDirection);
}

void ACombatPlayerCharacter::PlayBlockImpactMontage()
{
    if (!ensureMsgf(BlockImpactMontage, TEXT("%s: BlockImpactMontage is not assigned — blocked hits play no react"), *GetName()))
        return;

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

void ACombatPlayerCharacter::CharacterDied_Implementation()
{
    // Establish terminal priority before stopping montages and their notify states.
    bCombatDead = true;
    ++StaggerGeneration;
    GetWorldTimerManager().ClearTimer(StaggerTimerHandle);
    CancelCurrentAction(EGameplayNotifyInterruptReason::Death);
    comboIndex = 0;
    GetCharacterMovement()->DisableMovement();
    ShowActionDebugEvent(this, TEXT("Death: actions cancelled, movement disabled"), FColor::Red);
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
    if (!IsActionInputAllowed()) return;
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
        AttackEnd();
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
            if (AnimInstance->Montage_Play(AttackMontageToUse) > 0.f)
            {
                AnimInstance->Montage_JumpToSection(GetCurrentAttack(), AttackMontageToUse);
                return;
            }
        }

    }
    AttackEnd();
}

void ACombatPlayerCharacter::AttackEnd()
{
    if (actionState != EactionState::EAS_Attacking && actionState != EactionState::EAS_Comboing) return;
    SetActionState(EactionState::EAS_Unoccupied);
    SetAttackNumber(0);
}

void ACombatPlayerCharacter::StartInputBuffer()
{
    if (actionState != EactionState::EAS_Attacking) return;
    SetActionState(EactionState::EAS_Comboing);
}

void ACombatPlayerCharacter::EndBuffer()
{
    if (actionState != EactionState::EAS_Comboing) return;
    SetActionState(EactionState::EAS_Unoccupied);
}

FName ACombatPlayerCharacter::GetCurrentAttack()
{

    return FName("Attack " + FString::FromInt(comboIndex));
}

void ACombatPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GEngine)
    {
        if (CVarActionDebug.GetValueOnGameThread() > 0 && IsPlayerControlled() && IsLocallyControlled())
        {
            const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
            const UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
            const FString StateLabel = IsCombatAlive() ? ActionStateName(actionState) : TEXT("Dead");
            const FColor Color = !IsCombatAlive() || actionState == EactionState::EAS_Staggered ? FColor::Red : FColor::Cyan;
            GEngine->AddOnScreenDebugMessage(ActionDebugScreenKey(this, false), 0.5f, Color,
                FString::Printf(TEXT("[%s] ACTION: %s | Combo: %d | Dodge invulnerable: %s\nMontage: %s | Active ASNs: %d"),
                    *GetName(), *StateLabel, comboIndex, bDodgeInvulnerable ? TEXT("yes") : TEXT("no"),
                    *GetNameSafe(Montage), ActiveGameplayNotifies.Num()), false, FVector2D(1.2f, 1.2f));
        }
        else
        {
            GEngine->RemoveOnScreenDebugMessage(ActionDebugScreenKey(this, false));
            GEngine->RemoveOnScreenDebugMessage(ActionDebugScreenKey(this, true));
        }
    }

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
    // Cleanup is always allowed; an old notify cannot re-enable the hitbox during stagger/death.
    if (CollisionEnabled != ECollisionEnabled::NoCollision && !IsActionInputAllowed()) return;
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

