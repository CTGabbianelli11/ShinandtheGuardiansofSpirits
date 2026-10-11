#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/RadiatingAttackComponent.h"
#include "Combat/RingStrike.h"
#include "Combat/TelegraphActor.h"
#include "Components/AttributeComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Enemy/Enemy.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    AEnemy* SpawnTarget(UWorld* World, FVector Location)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEnemy* Target = World->SpawnActor<AEnemy>(Location, FRotator::ZeroRotator, Params);
        Target->GetCharacterMovement()->DisableMovement();
        Target->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Target->GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
        return Target;
    }

    float Health(AEnemy* Target)
    {
        return Target->FindComponentByClass<UAttributeComponent>()->GetHealthPercentage();
    }

    template <typename T>
    TArray<T*> Actors(UWorld* World)
    {
        TArray<T*> Result;
        for (TActorIterator<T> It(World); It; ++It)
        {
            if (!It->IsActorBeingDestroyed()) Result.Add(*It);
        }
        return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRingStrikeDamageTest, "Rhya.Combat.RadiatingAttack.Damage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRingStrikeDamageTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AEnemy* Center = SpawnTarget(World, FVector(0.f, 0.f, 88.f));
    AEnemy* Boundary = SpawnTarget(World, FVector(150.f, 0.f, 88.f));
    AEnemy* Band = SpawnTarget(World, FVector(225.f, 0.f, 88.f));
    AEnemy* OuterBoundary = SpawnTarget(World, FVector(300.f, 0.f, 88.f));
    AEnemy* Outside = SpawnTarget(World, FVector(301.f, 0.f, 88.f));
    AEnemy* Above = SpawnTarget(World, FVector(225.f, 0.f, 500.f));
    AEnemy* Owner = SpawnTarget(World, FVector(225.f, 50.f, 88.f));
    AEnemy* Instigator = SpawnTarget(World, FVector(225.f, -50.f, 88.f));

    // A second pawn-channel component must not cause a second damage event.
    UBoxComponent* Extra = NewObject<UBoxComponent>(Band);
    Extra->SetupAttachment(Band->GetRootComponent());
    Extra->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Extra->SetCollisionObjectType(ECC_Pawn);
    Extra->SetBoxExtent(FVector(20.f));
    Extra->RegisterComponent();

    FRadiatingAttackSettings Settings;
    auto Strike = [&](float Inner, float Outer)
    {
        ARingStrike* Actor = World->SpawnActorDeferred<ARingStrike>(ARingStrike::StaticClass(),
            FTransform::Identity, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        Actor->Configure(Settings, Inner, Outer);
        Actor->FinishSpawning(FTransform::Identity);
        return Actor;
    };
    Strike(0.f, 150.f);
    TestEqual(TEXT("First pulse includes center"), Health(Center), 0.8f);
    TestEqual(TEXT("Boundary belongs to first pulse"), Health(Boundary), 0.8f);
    TestEqual(TEXT("First pulse excludes outer band"), Health(Band), 1.f);

    ARingStrike* OuterStrike = Strike(150.f, 300.f);
    TestEqual(TEXT("Cleared center is safe"), Health(Center), 0.8f);
    TestEqual(TEXT("Adjacent pulse does not re-hit shared boundary"), Health(Boundary), 0.8f);
    TestEqual(TEXT("Band gets exactly one hit despite multiple components"), Health(Band), 0.8f);
    TestEqual(TEXT("Outer edge is included"), Health(OuterBoundary), 0.8f);
    TestEqual(TEXT("Pawn center outside is safe despite capsule overlap"), Health(Outside), 1.f);
    TestEqual(TEXT("Above the height slab is safe"), Health(Above), 1.f);
    TestEqual(TEXT("Owner excluded"), Health(Owner), 1.f);
    TestEqual(TEXT("Instigator excluded"), Health(Instigator), 1.f);
    for (int32 Index = 0; Index < 110; ++Index) Fixture.TickTestWorld(0.01f);
    TestEqual(TEXT("Lingering strike never repeats damage"), Health(Band), 0.8f);
    TestTrue(TEXT("Strike expires"), !IsValid(OuterStrike) || OuterStrike->IsActorBeingDestroyed());
    Fixture.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRadiatingSequenceTest, "Rhya.Combat.RadiatingAttack.Sequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRadiatingSequenceTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AActor* Floor = World->SpawnActor<AActor>();
    UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(FloorBox);
    FloorBox->SetBoxExtent(FVector(2000.f, 2000.f, 10.f));
    FloorBox->SetCollisionObjectType(ECC_WorldStatic);
    FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
    FloorBox->RegisterComponent();
    Floor->SetActorLocation(FVector(0.f, 0.f, -10.f));

    AEnemy* Owner = SpawnTarget(World, FVector(0.f, 0.f, 88.f));
    URadiatingAttackComponent* Attack = NewObject<URadiatingAttackComponent>(Owner);
    Owner->AddInstanceComponent(Attack);
    Attack->RegisterComponent();
    // This test checks timing, not material rendering.
    Attack->AttackSettings.TelegraphMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Blueprints/Characters/M_AoE_Telegraph.M_AoE_Telegraph"));
    if (!TestNotNull(TEXT("Material fixture"), Attack->AttackSettings.TelegraphMaterial.Get())) return false;
    Attack->AttackSettings.RingInterval = 0.1f;
    Attack->AttackSettings.TelegraphDuration = 0.25f;
    const FRadiatingAttackSettings OriginalSettings = Attack->AttackSettings;
    AEnemy* Target = SpawnTarget(World, FVector(225.f, 0.f, 88.f));
    auto Advance = [&](int32 Frames)
    {
        for (int32 Index = 0; Index < Frames; ++Index) Fixture.TickTestWorld(0.01f);
    };

    TestTrue(TEXT("Valid attack starts"), Attack->StartRadiatingAttack(FVector(0.f, 0.f, 88.f), 3));
    TestEqual(TEXT("First warning starts immediately"), Actors<ATelegraphActor>(World).Num(), 1);
    Attack->AttackSettings.RingWidth = 900.f;
    Attack->AttackSettings.Damage = 99.f;
    Owner->SetActorLocation(FVector(1000.f, 0.f, 88.f));
    Advance(22);
    TestEqual(TEXT("Three warnings stagger before their impacts"), Actors<ATelegraphActor>(World).Num(), 3);
    TestEqual(TEXT("Warnings have not damaged target"), Health(Target), 1.f);
    Advance(26);
    const TArray<ARingStrike*> Strikes = Actors<ARingStrike>(World);
    TestEqual(TEXT("Exactly three strikes"), Strikes.Num(), 3);
    for (const ARingStrike* Strike : Strikes)
    {
        TestTrue(TEXT("All pulses retain the original floor center"), Strike->GetActorLocation().Equals(FVector::ZeroVector, 0.1f));
        TestEqual(TEXT("All pulses retain snapshotted width"), Strike->Radius - Strike->InnerRadius, 150.f);
    }
    TestEqual(TEXT("Damage settings are snapshotted too"), Health(Target), 0.8f);
    Advance(10);
    TestEqual(TEXT("Sequence stops at requested count"), Actors<ARingStrike>(World).Num(), 3);

    Attack->AttackSettings = OriginalSettings;
    TestTrue(TEXT("Next sequence starts"), Attack->StartRadiatingAttack(FVector(0.f, 0.f, 88.f), 3));
    Advance(12);
    TestEqual(TEXT("Two pending warnings before cancellation"), Actors<ATelegraphActor>(World).Num(), 2);
    Attack->CancelRadiatingAttack();
    TestEqual(TEXT("Cancel removes visible warnings"), Actors<ATelegraphActor>(World).Num(), 0);
    Advance(50);
    TestEqual(TEXT("Cancel prevents pending impacts"), Health(Target), 0.8f);
    TestEqual(TEXT("Cancel prevents future warnings"), Actors<ATelegraphActor>(World).Num(), 0);

    TestTrue(TEXT("Restart setup"), Attack->StartRadiatingAttack(FVector(0.f, 0.f, 88.f), 3));
    TestTrue(TEXT("Restart with single ring"), Attack->StartRadiatingAttack(FVector(0.f, 0.f, 88.f), 1));
    TestEqual(TEXT("Restart discards earlier warnings"), Actors<ATelegraphActor>(World).Num(), 1);
    Owner->Destroy();
    TestEqual(TEXT("Owner destruction cancels pending warning"), Actors<ATelegraphActor>(World).Num(), 0);
    Advance(50);
    TestEqual(TEXT("No orphaned attack damages target"), Health(Target), 0.8f);
    Fixture.ForwardErrorMessages(this);
    return true;
}

#endif
