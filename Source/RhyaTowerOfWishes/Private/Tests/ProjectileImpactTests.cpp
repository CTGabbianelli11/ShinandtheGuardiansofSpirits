#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/StrikeHitTestCharacter.h"
#include "Animation/AnimMontage.h"
#include "Combat/HomingStrike.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"

namespace
{
    AStrikeHitTestCharacter* ImpactTarget(UWorld* World, FVector Location)
    {
        AStrikeHitTestCharacter* Target = World->SpawnActor<AStrikeHitTestCharacter>(Location, FRotator::ZeroRotator);
        Target->GetCharacterMovement()->DisableMovement();
        // Exercise the real blocked-hit event without requiring an animation asset.
        FindFProperty<FObjectPropertyBase>(ACombatPlayerCharacter::StaticClass(), TEXT("BlockImpactMontage"))
            ->SetObjectPropertyValue_InContainer(Target, NewObject<UAnimMontage>(Target));
        return Target;
    }

    UBoxComponent* BlockingBody(AActor* Actor, FVector Location, FVector Extent)
    {
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        if (Actor->GetRootComponent()) Box->SetupAttachment(Actor->GetRootComponent());
        else Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->SetGenerateOverlapEvents(false);
        Box->RegisterComponent();
        Box->SetWorldLocation(Location);
        return Box;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileImpactTest, "Rhya.Combat.Projectile.BlockingImpact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectileImpactTest::RunTest(const FString& Parameters)
{
    UClass* ProjectileClass = LoadClass<AProjectileStrike>(nullptr,
        TEXT("/Game/Blueprints/Attacks/Projectiles/BP_ProjectileStrike.BP_ProjectileStrike_C"));
    if (!TestNotNull(TEXT("Actual projectile Blueprint"), ProjectileClass)) return false;
    for (int32 Shape = 0; Shape < 3; ++Shape)
    {
        for (int32 Guard = 0; Guard < 3; ++Guard)
        {
            FTestWorldWrapper Fixture;
            if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
            UWorld* World = Fixture.GetTestWorld();
            const FVector Location(1000.f, 2000.f, 100.f);
            AStrikeHitTestCharacter* Target = ImpactTarget(World, Location);
            if (Guard != 0)
            {
                Target->SetActionState(EactionState::EAS_Blocking);
                Target->SetActorRotation(FRotator(0.f, Guard == 1 ? 180.f : 0.f, 0.f));
            }
            if (Shape == 1) Target->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (Shape != 0) BlockingBody(Target, Location, FVector(20.f));

            AProjectileStrike* Shot = AProjectileStrike::SpawnConfigured(World, ProjectileClass,
                FTransform(Location - FVector(200.f, 0.f, 0.f)), 1000.f, 1000.f, nullptr, nullptr);
            USphereComponent* Sphere = Shot->FindComponentByClass<USphereComponent>();
            if (Shape == 0) Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
            // Shape 1: solid body without pawn overlap. Shape 2: pawn overlap and solid body in one sweep.
            Shot->Tick(0.4f);
            const FString Label = FString::Printf(TEXT("Shape %d guard %d"), Shape, Guard);
            TestEqual(Label + TEXT(" damage once"), Target->DamageCalls, 1);
            TestEqual(Label + TEXT(" GetHit once"), Target->HitCalls, 1);
            TestTrue(Label + TEXT(" directional point damage"), Target->bPointDamage);
            TestTrue(Label + TEXT(" reaction after damage"), Target->bDamageBeforeHit);
            TestEqual(Label + TEXT(" guard faces shot"), Target->AppliedDamage, Guard == 1 ? 0.f : 20.f);
            TestEqual(Label + TEXT(" correct hit/block reaction"), Target->bLastHitBlocked, Guard == 1);
            TestTrue(Label + TEXT(" source-facing normal"), Target->HitDirection.X < 0.f);
            TestTrue(Label + TEXT(" impact stays near target"), FVector::Dist(Target->HitPoint, Location) < 100.f);
            TestTrue(Label + TEXT(" projectile removed"), Shot->IsActorBeingDestroyed());

            // Pending callback delivery and a later tick must not spend this shot a second time.
            FHitResult Hit(Target, Target->GetCapsuleComponent(), Location, -FVector::ForwardVector);
            Sphere->OnComponentHit.Broadcast(Sphere, Target, Target->GetCapsuleComponent(), FVector::ZeroVector, Hit);
            Sphere->OnComponentBeginOverlap.Broadcast(Sphere, Target, Target->GetCapsuleComponent(), 0, true, Hit);
            Shot->Tick(0.4f);
            TestEqual(Label + TEXT(" no repeated damage from another callback"), Target->DamageCalls, 1);
            TestEqual(Label + TEXT(" no repeated GetHit"), Target->HitCalls, 1);
            Fixture.ForwardErrorMessages(this);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileWallAndHomingTest, "Rhya.Combat.Projectile.WallAndHoming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectileWallAndHomingTest::RunTest(const FString& Parameters)
{
    for (int32 Mode = 0; Mode < 3; ++Mode)
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld()) return false;
        UWorld* World = Fixture.GetTestWorld();
        const FVector WallLocation(1000.f, 2000.f, 100.f);
        AStrikeHitTestCharacter* Target = ImpactTarget(World, WallLocation + FVector(100.f, 0.f, 0.f));
        if (Mode != 2)
        {
            AActor* Wall = World->SpawnActor<AActor>();
            BlockingBody(Wall, WallLocation, FVector(20.f, 100.f, 100.f));
        }
        // Mode 0: straight shot cannot hurt a pawn behind a wall.
        // Mode 1: homing wall impact still explodes once; Mode 2: range expiry still explodes once.
        AProjectileStrike* Shot = AProjectileStrike::SpawnConfigured(World,
            Mode == 0 ? AProjectileStrike::StaticClass() : AHomingStrike::StaticClass(),
            FTransform(WallLocation - FVector(200.f, 0.f, 0.f)), 1000.f, Mode == 2 ? 200.f : 1000.f, nullptr, nullptr);
        Shot->Tick(0.4f);
        Shot->Tick(0.4f);
        TestTrue(TEXT("Projectile is removed"), Shot->IsActorBeingDestroyed());
        TestEqual(TEXT("Wall stops direct damage; homing explodes once"), Target->DamageCalls, Mode == 0 ? 0 : 1);
        TestEqual(TEXT("One reaction per explosion"), Target->HitCalls, Mode == 0 ? 0 : 1);
        if (Mode != 0)
        {
            TestTrue(TEXT("Homing keeps radial damage"), Target->bRadialDamage);
            TestEqual(TEXT("No added direct-impact damage"), Target->AppliedDamage, 20.f);
        }
        Fixture.ForwardErrorMessages(this);
    }
    return true;
}

#endif
