#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/StrikeHitTestCharacter.h"
#include "Tests/GameplayNotifyTestState.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyEndDataContext.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/AttributeComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

namespace
{
    void Hit(AStrikeHitTestCharacter* Player)
    {
        FDamageEvent DamageEvent;
        Player->TakeDamage(10.f, DamageEvent, nullptr, nullptr);
        Player->GetHit_Implementation(Player->GetActorLocation(), FVector::ForwardVector, 10.f);
    }

    void Advance(FTestWorldWrapper& World, int32 Frames)
    {
        for (int32 Frame = 0; Frame < Frames; ++Frame) World.TickTestWorld(0.01f);
    }

    FAnimNotifyEventReference NotifyReference(int32 MontageID, bool bReachedEnd = true)
    {
        FAnimNotifyEventReference Reference;
        Reference.AddContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(MontageID);
        Reference.AddContextData<UE::Anim::FAnimNotifyEndDataContext>(bReachedEnd);
        return Reference;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerStaggerCleanupTest, "Rhya.Combat.Player.Stagger.CleanupAndPermissions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerStaggerCleanupTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
    UWorld* World = Fixture.GetTestWorld();
    auto* Player = World->SpawnActor<AStrikeHitTestCharacter>();
    auto* Weapon = World->SpawnActorDeferred<AWeapon>(AWeapon::StaticClass(), FTransform::Identity);
    // This fixture exercises an equipped weapon, not pickup or damage overlap dispatch.
    Weapon->FindComponentByClass<USphereComponent>()->SetGenerateOverlapEvents(false);
    Weapon->GetWeaponBoxComponent()->SetGenerateOverlapEvents(false);
    Weapon->FinishSpawning(FTransform::Identity);
    Player->EquipExistingWeapon(Weapon);
    TestTrue(TEXT("Attack can start"), Player->TryStartAction(EactionState::EAS_Attacking));
    Player->SetAttackNumber(3);
    auto* AirStall = NewObject<UGameplayNotifyTestState>();
    AirStall->bAirStall = true;
    auto* WeaponWindow = NewObject<UGameplayNotifyTestState>();
    WeaponWindow->bWeaponWindow = true;
    WeaponWindow->bFinishAction = true;
    const FAnimNotifyEventReference Reference;
    AirStall->NotifyBegin(Player->GetMesh(), nullptr, 1.f, Reference);
    WeaponWindow->NotifyBegin(Player->GetMesh(), nullptr, 1.f, Reference);
    Player->GetCharacterMovement()->Velocity = FVector(200.f, 100.f, -90.f);

    Hit(Player);
    TestEqual(TEXT("Hit enters stagger without animation"), Player->GetActionState(), EactionState::EAS_Staggered);
    TestEqual(TEXT("Weapon remains active until ASN End: trade window"), Weapon->GetWeaponBoxComponent()->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
    TestEqual(TEXT("Combo cleared"), Player->GetAttackNumber(), 0);
    TestEqual(TEXT("Air stall remains owned by ASN"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Flying);
    TestEqual(TEXT("Falling velocity retained"), Player->GetCharacterMovement()->Velocity.Z, -90.0);
    TestTrue(TEXT("Attack momentum replaced by knockback"),
        Player->GetCharacterMovement()->Velocity.Equals(FVector(-200.f, 0.f, -90.f)));
    TestFalse(TEXT("Blueprint action gate closed"), Player->IsActionInputAllowed());
    for (EactionState Action : {EactionState::EAS_Attacking, EactionState::EAS_Comboing,
        EactionState::EAS_Dodging, EactionState::EAS_Blocking, EactionState::EAS_Healing, EactionState::EAS_Unoccupied})
    {
        TestFalse(TEXT("Cannot start action during stagger"), Player->TryStartAction(Action));
        Player->SetActionState(Action);
        TestEqual(TEXT("Legacy notify cannot clear stagger"), Player->GetActionState(), EactionState::EAS_Staggered);
    }
    Player->AttackEnd();
    Player->StartInputBuffer();
    Player->EndBuffer();
    AirStall->NotifyEnd(Player->GetMesh(), nullptr, Reference);
    WeaponWindow->NotifyEnd(Player->GetMesh(), nullptr, Reference);
    TestEqual(TEXT("Both active ASNs ended once"), Player->GameplayNotifyEndReasons.Num(), 2);
    for (EGameplayNotifyInterruptReason Reason : Player->GameplayNotifyEndReasons)
        TestEqual(TEXT("Each ASN receives stagger reason"), Reason, EGameplayNotifyInterruptReason::Staggered);
    TestEqual(TEXT("ASN restores gravity during stagger"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Falling);

    // End always runs idempotent cleanup, but consumed context cannot complete normally again.
    WeaponWindow->NotifyEnd(Player->GetMesh(), nullptr, Reference);
    TestEqual(TEXT("Duplicate End is treated as cancellation"), Player->GameplayNotifyEndReasons.Last(),
        EGameplayNotifyInterruptReason::ActionCancelled);
    TestEqual(TEXT("Repeated cleanup leaves weapon disabled"), Weapon->GetWeaponBoxComponent()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

    // Simulate effects whose Begin forgot to register: cleanup must still run even when
    // the engine reports a natural End, without releasing the current stagger.
    auto* UnregisteredWindow = NewObject<UGameplayNotifyTestState>();
    UnregisteredWindow->bAirStall = true;
    UnregisteredWindow->bFinishAction = true;
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    UnregisteredWindow->NotifyEnd(Player->GetMesh(), nullptr, NotifyReference(99));
    TestEqual(TEXT("Unregistered End still restores movement"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Falling);
    TestEqual(TEXT("Unregistered natural End is not normal completion"), Player->GameplayNotifyEndReasons.Last(),
        EGameplayNotifyInterruptReason::ActionCancelled);
    TestEqual(TEXT("Interruption never completes normally"), Player->NormalNotifyCompletions, 0);
    Player->EndDodge();
    Player->StartBlock();
    Player->StartDodge();
    Player->StartHeal();
    Player->Jump();
    Player->SetWeaponCollisionEnabled(ECollisionEnabled::QueryOnly);
    TestFalse(TEXT("Jump input rejected"), Player->bPressedJump);
    TestEqual(TEXT("Old callbacks cannot release stagger"), Player->GetActionState(), EactionState::EAS_Staggered);
    TestEqual(TEXT("Old notify cannot enable weapon"), Weapon->GetWeaponBoxComponent()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    Advance(Fixture, 50);
    TestEqual(TEXT("Missing-animation fallback recovers"), Player->GetActionState(), EactionState::EAS_Unoccupied);
    TestTrue(TEXT("Actions enabled after recovery"), Player->IsActionInputAllowed());

    Player->GetAttributes()->ReceiveDamage(40.f);
    Player->StartHeal();
    TestEqual(TEXT("Heal starts"), Player->GetActionState(), EactionState::EAS_Healing);
    Hit(Player);
    const float HealthAfterHit = Player->GetAttributes()->GetHealthPercentage();
    const float MagicAfterHit = Player->GetAttributes()->GetMagicPercentage();
    Advance(Fixture, 110);
    TestEqual(TEXT("Interrupted heal does not restore health"), Player->GetAttributes()->GetHealthPercentage(), HealthAfterHit);
    TestEqual(TEXT("Interrupted heal does not spend magic"), Player->GetAttributes()->GetMagicPercentage(), MagicAfterHit);
    Fixture.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerStaggerRecoveryTest, "Rhya.Combat.Player.Stagger.RestartAndDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerStaggerRecoveryTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
    auto* Player = Fixture.GetTestWorld()->SpawnActor<AStrikeHitTestCharacter>();
    Hit(Player); // First stagger generation.
    Advance(Fixture, 25);
    Hit(Player); // Restart invalidates the first callback.
    Player->FinishStagger(1);
    TestEqual(TEXT("Old montage completion ignored"), Player->GetActionState(), EactionState::EAS_Staggered);
    Advance(Fixture, 25);
    TestEqual(TEXT("Second hit extends recovery"), Player->GetActionState(), EactionState::EAS_Staggered);
    Advance(Fixture, 25);
    TestEqual(TEXT("Restart eventually recovers"), Player->GetActionState(), EactionState::EAS_Unoccupied);

    FBoolProperty* Restart = FindFProperty<FBoolProperty>(Player->GetClass(), TEXT("bRestartStaggerOnHit"));
    if (!TestNotNull(TEXT("Designer restart option"), Restart)) return false;
    Restart->SetPropertyValue_InContainer(Player, false);
    Hit(Player);
    Advance(Fixture, 25);
    Hit(Player);
    Advance(Fixture, 25);
    TestEqual(TEXT("Non-restarting policy keeps original deadline"), Player->GetActionState(), EactionState::EAS_Unoccupied);

    Hit(Player);
    Player->GetAttributes()->ReceiveDamage(1000.f);
    Advance(Fixture, 60);
    Player->SetActionState(EactionState::EAS_Unoccupied);
    Player->StartBlock();
    TestFalse(TEXT("Death prevents recovery/actions"), Player->IsActionInputAllowed());
    TestEqual(TEXT("Death movement remains disabled"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
    const int32 HurtEvents = Player->HurtEvents;
    Hit(Player);
    TestEqual(TEXT("No corpse hurt reaction"), Player->HurtEvents, HurtEvents);
    Fixture.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDodgeImmunityTest, "Rhya.Combat.Player.Stagger.DodgeInvulnerability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDodgeImmunityTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
    auto* Player = Fixture.GetTestWorld()->SpawnActor<AStrikeHitTestCharacter>();
    Player->TryStartAction(EactionState::EAS_Dodging);
    Player->SetDodgeInvulnerable(true);
    Hit(Player);
    TestEqual(TEXT("Dodge window rejects damage"), Player->AppliedDamage, 0.f);
    TestEqual(TEXT("Dodge window rejects hurt event"), Player->HurtEvents, 0);
    TestEqual(TEXT("Dodge remains active"), Player->GetActionState(), EactionState::EAS_Dodging);
    Player->SetDodgeInvulnerable(false);
    Hit(Player);
    TestEqual(TEXT("Vulnerable dodge takes damage"), Player->AppliedDamage, 10.f);
    TestEqual(TEXT("Vulnerable dodge is staggered"), Player->GetActionState(), EactionState::EAS_Staggered);
    Player->SetDodgeInvulnerable(true); // A queued marker cannot grant immunity in stagger.
    Hit(Player);
    TestEqual(TEXT("Stale invulnerability marker rejected"), Player->AppliedDamage, 10.f);
    Fixture.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerStaggerMontageTest, "Rhya.Combat.Player.Stagger.MontageLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerStaggerMontageTest::RunTest(const FString& Parameters)
{
    auto* Montage = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/Blueprints/Characters/MainRobo/Montages/AM_HitReact.AM_HitReact"));
    auto* MeshAsset = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/PlayerCharacter/Model/MainRoboWithDodgeRoll.MainRoboWithDodgeRoll"));
    if (!TestNotNull(TEXT("Hurt montage"), Montage) || !TestNotNull(TEXT("Matching mesh"), MeshAsset)) return false;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
    auto* Player = Fixture.GetTestWorld()->SpawnActor<AStrikeHitTestCharacter>();
    Player->GetMesh()->SetSkeletalMeshAsset(MeshAsset);
    Player->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Player->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    auto* Property = FindFProperty<FObjectPropertyBase>(Player->GetClass(), TEXT("HitReactMontage"));
    if (!TestNotNull(TEXT("Montage property"), Property)) return false;
    Property->SetObjectPropertyValue_InContainer(Player, Montage);
    UAnimInstance* Anim = Player->GetMesh()->GetAnimInstance();
    if (!TestNotNull(TEXT("Native anim instance"), Anim)) return false;

    Hit(Player);
    TestTrue(TEXT("Hurt montage plays"), Anim->Montage_IsPlaying(Montage));
    Advance(Fixture, 10);
    Hit(Player);
    Advance(Fixture, 15); // Let the previous instance's interrupted end callback run.
    TestEqual(TEXT("Old montage does not finish restarted stagger"), Player->GetActionState(), EactionState::EAS_Staggered);
    Advance(Fixture, 80);
    TestEqual(TEXT("Montage completion recovers"), Player->GetActionState(), EactionState::EAS_Unoccupied);

    Hit(Player);
    Anim->Montage_Stop(0.f, Montage);
    Advance(Fixture, 2);
    TestEqual(TEXT("Unexpected stop cannot strand player"), Player->GetActionState(), EactionState::EAS_Unoccupied);

    // Reuse a short montage to exercise dodge callbacks independently of Blueprint notifies.
    auto* DodgeProperty = FindFProperty<FObjectPropertyBase>(Player->GetClass(), TEXT("DodgeMontage"));
    if (!TestNotNull(TEXT("Dodge montage property"), DodgeProperty)) return false;
    DodgeProperty->SetObjectPropertyValue_InContainer(Player, Montage);
    Player->StartDodge();
    Advance(Fixture, 10);
    Player->EndDodge(); // Simulate the existing early recovery marker.
    Player->StartDodge();
    Advance(Fixture, 15);
    TestEqual(TEXT("Old dodge callback cannot end new dodge"), Player->GetActionState(), EactionState::EAS_Dodging);
    Advance(Fixture, 80);
    TestEqual(TEXT("Current dodge recovers"), Player->GetActionState(), EactionState::EAS_Unoccupied);

    // Use a transient copy to test overlapping notify windows.
    auto* WindowMontage = DuplicateObject<UAnimMontage>(Montage, GetTransientPackage());
    WindowMontage->Notifies.Reset();
    for (int32 Index = 0; Index < 2; ++Index)
    {
        auto* State = NewObject<UGameplayNotifyTestState>(WindowMontage);
        State->bAirStall = Index == 0;
        State->bFinishAction = Index == 1;
        FAnimNotifyEvent& Event = WindowMontage->Notifies.AddDefaulted_GetRef();
        Event.NotifyStateClass = State;
        Event.Link(WindowMontage, 0.02f);
        Event.SetDuration(0.25f);
    }
    WindowMontage->RefreshCacheData();

    Player->SetActionState(EactionState::EAS_Attacking);
    Anim->Montage_Play(WindowMontage);
    Advance(Fixture, 6);
    TestEqual(TEXT("Two real notify windows began"), Player->GameplayNotifyBegins, 2);
    Hit(Player);
    Advance(Fixture, 20);
    TestEqual(TEXT("Stopping montage delivers both End events"), Player->GameplayNotifyEndReasons.Num(), 2);
    for (EGameplayNotifyInterruptReason Reason : Player->GameplayNotifyEndReasons)
        TestEqual(TEXT("Engine End sees interruption"), Reason, EGameplayNotifyInterruptReason::Staggered);
    for (EactionState State : Player->GameplayNotifyEndStates)
        TestEqual(TEXT("Stagger established before End"), State, EactionState::EAS_Staggered);
    TestEqual(TEXT("Interrupted engine End skips completion"), Player->NormalNotifyCompletions, 0);
    TestEqual(TEXT("Engine End restores gravity"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Falling);
    Advance(Fixture, 80);

    Player->GameplayNotifyEndReasons.Reset();
    Player->SetActionState(EactionState::EAS_Attacking);
    Anim->Montage_Play(WindowMontage);
    Advance(Fixture, 80);
    TestEqual(TEXT("Normal playback ends both windows"), Player->GameplayNotifyEndReasons.Num(), 2);
    for (EGameplayNotifyInterruptReason Reason : Player->GameplayNotifyEndReasons)
        TestEqual(TEXT("Normal engine End reason"), Reason, EGameplayNotifyInterruptReason::None);
    TestEqual(TEXT("Normal End performs completion"), Player->NormalNotifyCompletions, 2);
    TestEqual(TEXT("Normal finisher releases state"), Player->GetActionState(), EactionState::EAS_Unoccupied);

    // An external montage stop also counts as cancellation, even outside CPC's action API.
    Player->GameplayNotifyEndReasons.Reset();
    Player->SetActionState(EactionState::EAS_Attacking);
    Anim->Montage_Play(WindowMontage);
    Advance(Fixture, 6);
    Anim->Montage_Stop(0.f, WindowMontage);
    Advance(Fixture, 2);
    TestEqual(TEXT("External stop ends both windows"), Player->GameplayNotifyEndReasons.Num(), 2);
    for (EGameplayNotifyInterruptReason Reason : Player->GameplayNotifyEndReasons)
        TestEqual(TEXT("External stop is not normal completion"), Reason, EGameplayNotifyInterruptReason::ActionCancelled);

    Player->GameplayNotifyEndReasons.Reset();
    Anim->Montage_Play(WindowMontage);
    Advance(Fixture, 6);
    Player->GetAttributes()->ReceiveDamage(1000.f);
    Advance(Fixture, 2);
    TestEqual(TEXT("Death flushes active windows"), Player->GameplayNotifyEndReasons.Num(), 2);
    for (EGameplayNotifyInterruptReason Reason : Player->GameplayNotifyEndReasons)
        TestEqual(TEXT("Death End receives terminal reason"), Reason, EGameplayNotifyInterruptReason::Death);
    TestEqual(TEXT("Death cleanup does not restore falling"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
    Fixture.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerNotifyInterruptionTest, "Rhya.Combat.Player.Stagger.NotifyInterruptionContext",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerNotifyInterruptionTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
    auto* Player = Fixture.GetTestWorld()->SpawnActor<AStrikeHitTestCharacter>();
    auto* OtherPlayer = Fixture.GetTestWorld()->SpawnActor<AStrikeHitTestCharacter>(
        FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator);
    auto* SharedNotify = NewObject<UGameplayNotifyTestState>();
    const auto OldReference = NotifyReference(42);
    const auto NewReference = NotifyReference(43);
    Player->RegisterAnimNotifyState(SharedNotify, OldReference);
    OtherPlayer->RegisterAnimNotifyState(SharedNotify, OldReference);
    Hit(Player);
    Player->RegisterAnimNotifyState(SharedNotify, OldReference); // Must retain the original interruption.
    auto Reason = OtherPlayer->ConsumeAnimNotifyStateInterruptReason(SharedNotify, OldReference);
    TestEqual(TEXT("Other player's window was not interrupted"), Reason, EGameplayNotifyInterruptReason::None);
    Advance(Fixture, 50);
    Player->RegisterAnimNotifyState(SharedNotify, NewReference);
    Reason = Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, OldReference);
    TestEqual(TEXT("Reason survives duplicate Begin and stagger recovery"), Reason, EGameplayNotifyInterruptReason::Staggered);
    TestEqual(TEXT("Repeated End cannot finish another window"), Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, OldReference),
        EGameplayNotifyInterruptReason::ActionCancelled);
    Reason = Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, NewReference);
    TestEqual(TEXT("New window has normal reason"), Reason, EGameplayNotifyInterruptReason::None);
    TestEqual(TEXT("Normal completion is also consumed"), Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, NewReference),
        EGameplayNotifyInterruptReason::ActionCancelled);
    TestEqual(TEXT("Invalid notify cannot complete normally"), Player->ConsumeAnimNotifyStateInterruptReason(nullptr, NewReference),
        EGameplayNotifyInterruptReason::ActionCancelled);

    Player->RegisterAnimNotifyState(SharedNotify, OldReference);
    TestTrue(TEXT("Replacement action starts"), Player->TryStartAction(EactionState::EAS_Dodging));
    Hit(Player); // A later stagger does not rewrite the earlier cancellation's cause.
    Reason = Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, OldReference);
    TestEqual(TEXT("Dodge cancellation reason retained"), Reason, EGameplayNotifyInterruptReason::ActionCancelled);
    Advance(Fixture, 50);

    Player->RegisterAnimNotifyState(SharedNotify, OldReference);
    Hit(Player);
    Player->GetAttributes()->ReceiveDamage(1000.f);
    Reason = Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, OldReference);
    TestEqual(TEXT("Death supersedes stagger for cleanup"), Reason, EGameplayNotifyInterruptReason::Death);
    TestEqual(TEXT("Duplicate End after death retains safe cleanup reason"), Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, OldReference),
        EGameplayNotifyInterruptReason::Death);
    TestEqual(TEXT("Unregistered End after death retains safe cleanup reason"), Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, NewReference),
        EGameplayNotifyInterruptReason::Death);
    Player->RegisterAnimNotifyState(SharedNotify, NewReference);
    TestEqual(TEXT("Late Begin records death without acting as an action gate"), Player->ConsumeAnimNotifyStateInterruptReason(SharedNotify, NewReference),
        EGameplayNotifyInterruptReason::Death);
    Fixture.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerHitKnockbackTest, "Rhya.Combat.Player.HitKnockback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerHitKnockbackTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
    auto* Player = Fixture.GetTestWorld()->SpawnActor<AStrikeHitTestCharacter>();
    auto* Movement = Player->GetCharacterMovement();
    Player->SetActorRotation(FRotator(10.f, 0.f, 5.f));
    Movement->Velocity = FVector(123.f, 456.f, -90.f);
    const FVector Contact = Player->GetActorLocation() + FVector(0.f, 50.f, 100.f);
    Player->GetHit_Implementation(Contact, FVector::ForwardVector, 10.f);
    TestTrue(TEXT("Faces contact using yaw only"), Player->GetActorRotation().Equals(FRotator(10.f, 90.f, 5.f)));
    TestTrue(TEXT("Pushes away horizontally and preserves vertical speed"),
        Movement->Velocity.Equals(FVector(0.f, -200.f, -90.f)));

    Player->GetHit_Implementation(Contact, FVector::ForwardVector, 20.f);
    TestTrue(TEXT("Damage scales speed"), Movement->Velocity.Equals(FVector(0.f, -400.f, -90.f)));
    Player->GetHit_Implementation(Contact, FVector::ForwardVector, 100.f);
    TestTrue(TEXT("Large hits are capped"), Movement->Velocity.Equals(FVector(0.f, -600.f, -90.f)));
    Player->GetHit_Implementation(Player->GetActorLocation(), FVector(-1.f, 0.f, 1.f), 10.f);
    TestTrue(TEXT("Centered hit falls back to normal"), Movement->Velocity.Equals(FVector(200.f, 0.f, -90.f)));
    Player->GetHit_Implementation(Player->GetActorLocation(), FVector::UpVector, 10.f);
    TestTrue(TEXT("No horizontal source produces no arbitrary push"), Movement->Velocity.Equals(FVector(0.f, 0.f, -90.f)));
    Player->GetHit_Implementation(Contact, FVector::ForwardVector, 0.f);
    TestTrue(TEXT("Zero damage does not push"), Movement->Velocity.Equals(FVector(0.f, 0.f, -90.f)));
    Fixture.ForwardErrorMessages(this);
    return true;
}

#endif
