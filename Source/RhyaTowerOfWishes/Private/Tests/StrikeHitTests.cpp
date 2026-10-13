#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/StrikeHitTestCharacter.h"
#include "Animation/AnimMontage.h"
#include "Combat/HomingStrike.h"
#include "Combat/PillarStrike.h"
#include "Combat/RingStrike.h"
#include "Combat/WallStrike.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrikeHitTest, "Rhya.Combat.Strikes.GetHit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrikeHitTest::RunTest(const FString& Parameters)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Wall mesh fixture"), Cube)) return false;
    UClass* ProjectileBlueprint = LoadClass<AProjectileStrike>(nullptr,
        TEXT("/Game/Blueprints/Attacks/Projectiles/BP_ProjectileStrike.BP_ProjectileStrike_C"));
    if (!TestNotNull(TEXT("Gym projectile Blueprint fixture"), ProjectileBlueprint)) return false;

    // Include the gym's Blueprint: serialized collision overrides can bypass the native hit path.
    // Every strike must notify once, including when blocking absorbs all of its damage.
    for (int32 Kind = 0; Kind < 6; ++Kind)
    {
        for (int32 Scenario = 0; Scenario < 3; ++Scenario)
        {
            FTestWorldWrapper Fixture;
            if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld())
            {
                Fixture.ForwardErrorMessages(this);
                return false;
            }
            UWorld* World = Fixture.GetTestWorld();
            const FVector TargetLocation(1000.f, 2000.f, 88.f);
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AStrikeHitTestCharacter* Target = World->SpawnActor<AStrikeHitTestCharacter>(
                TargetLocation, FRotator(0.f, 180.f, 0.f), Params);
            // The native block reaction requires an assigned montage, even without an anim instance.
            FObjectPropertyBase* MontageProperty = FindFProperty<FObjectPropertyBase>(
                ACombatPlayerCharacter::StaticClass(), TEXT("BlockImpactMontage"));
            if (!TestNotNull(TEXT("Block montage property"), MontageProperty)) return false;
            MontageProperty->SetObjectPropertyValue_InContainer(Target, NewObject<UAnimMontage>(Target));
            Target->GetCharacterMovement()->DisableMovement();
            Target->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Target->GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
            Target->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
            Target->SetActionState(Scenario == 1 ? EactionState::EAS_Blocking : EactionState::EAS_Unoccupied);
            Target->bDestroyOnDamage = Scenario == 2;

            // Multiple overlapping components must still produce only one damage/reaction pair.
            UBoxComponent* Extra = NewObject<UBoxComponent>(Target);
            Extra->SetupAttachment(Target->GetRootComponent());
            Extra->SetBoxExtent(FVector(20.f));
            Extra->SetCollisionObjectType(ECC_Pawn);
            Extra->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Extra->RegisterComponent();

            auto Attack = [&]()
            {
                FVector Origin = TargetLocation - FVector(150.f, 0.f, 0.f);
                if (Kind == 0 || Kind == 4) Origin.Z = 0.f;
                const FTransform Transform(Origin);
                if (Kind == 0)
                {
                    AStrikeActor::SpawnConfigured(World, APillarStrike::StaticClass(), Transform, 300.f, nullptr, nullptr);
                }
                else if (Kind == 1 || Kind == 2 || Kind == 5)
                {
                    const TSubclassOf<AProjectileStrike> Class = Kind == 5 ? ProjectileBlueprint
                        : Kind == 1 ? AProjectileStrike::StaticClass() : AHomingStrike::StaticClass();
                    AProjectileStrike* Shot = AProjectileStrike::SpawnConfigured(World, Class, Transform, 1000.f, 1000.f, nullptr, nullptr);
                    Shot->Tick(0.1f);
                }
                else if (Kind == 3)
                {
                    AWallStrike* Wall = World->SpawnActorDeferred<AWallStrike>(AWallStrike::StaticClass(), Transform);
                    Wall->FindComponentByClass<UStaticMeshComponent>()->SetStaticMesh(Cube);
                    Wall->FinishSpawning(Transform);
                    Wall->Tick(1.f); // Arm.
                    Wall->Tick(0.25f); // Sweep through the target.
                    Wall->Tick(0.01f); // Remaining overlaps must not re-hit.
                    Wall->Destroy();
                }
                else
                {
                    ARingStrike* Ring = World->SpawnActorDeferred<ARingStrike>(ARingStrike::StaticClass(), Transform);
                    Ring->Configure(FRadiatingAttackSettings(), 0.f, 300.f);
                    Ring->FinishSpawning(Transform);
                }
            };

            const FString Context = FString::Printf(TEXT("Strike %d scenario %d"), Kind, Scenario);
            Attack();
            TestEqual(Context + TEXT(": damage once"), Target->DamageCalls, 1);
            TestEqual(Context + TEXT(": reaction once unless destroyed"), Target->HitCalls, Scenario == 2 ? 0 : 1);
            if (Scenario != 2)
            {
                TestTrue(Context + TEXT(": damage precedes reaction"), Target->bDamageBeforeHit);
                TestEqual(Context + TEXT(": reaction has original damage"), Target->HitDamage, 20.f);
                TestTrue(Context + TEXT(": impact near target, not world origin"), FVector::Dist(Target->HitPoint, TargetLocation) < 150.f);
                TestTrue(Context + TEXT(": impact direction points toward source"), Target->HitDirection.X < 0.f);
                TestEqual(Context + TEXT(": block verdict"), Target->bLastHitBlocked, Scenario == 1);
                TestEqual(Context + TEXT(": applied damage"), Target->AppliedDamage, Scenario == 1 ? 0.f : 20.f);
            }
            if (Scenario == 1)
            {
                // Same frame, but no longer guarding: do not reuse the first hit's block verdict.
                Target->SetActionState(EactionState::EAS_Unoccupied);
                Attack();
                TestEqual(Context + TEXT(": second reaction"), Target->HitCalls, 2);
                TestFalse(Context + TEXT(": second hit is unblocked"), Target->bLastHitBlocked);
                TestEqual(Context + TEXT(": second hit applies damage"), Target->AppliedDamage, 20.f);
            }
            Fixture.ForwardErrorMessages(this);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWallInitialOverlapHitTest, "Rhya.Combat.Strikes.InitialOverlapGetHit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallInitialOverlapHitTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    const FVector Location(1000.f, 2000.f, 88.f);
    AStrikeHitTestCharacter* Target = World->SpawnActor<AStrikeHitTestCharacter>(Location, FRotator::ZeroRotator);
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Wall mesh fixture"), Cube)) return false;
    AWallStrike* Wall = World->SpawnActorDeferred<AWallStrike>(AWallStrike::StaticClass(), FTransform(Location));
    Wall->FindComponentByClass<UStaticMeshComponent>()->SetStaticMesh(Cube);
    Wall->FinishSpawning(FTransform(Location));
    Wall->Tick(1.f);
    TestEqual(TEXT("Arming wall notifies existing overlap once"), Target->HitCalls, 1);
    TestTrue(TEXT("Un-swept overlap uses target position"), Target->HitPoint.Equals(Location));
    TestTrue(TEXT("Un-swept overlap has source-facing direction"), Target->HitDirection.Equals(-FVector::ForwardVector));
    Fixture.ForwardErrorMessages(this);
    return true;
}

#endif
