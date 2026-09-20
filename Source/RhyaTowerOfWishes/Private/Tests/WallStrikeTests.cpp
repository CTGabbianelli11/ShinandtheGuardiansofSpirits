#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/WallStrike.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWallStrikeShatterTest, "Rhya.Combat.WallStrike.Shatter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallStrikeShatterTest::RunTest(const FString& Parameters)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    // Existing one-shot system is a test fixture; the production wall's effect is artist-assigned.
    UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_IcicleImpact.NS_IcicleImpact"));
    FObjectPropertyBase* EffectProperty = FindFProperty<FObjectPropertyBase>(AWallStrike::StaticClass(), TEXT("ShatterEffect"));
    if (!TestNotNull(TEXT("Cube fixture"), Cube) || !TestNotNull(TEXT("Niagara fixture"), System)
        || !TestNotNull(TEXT("Editable shatter effect"), EffectProperty))
    {
        return false;
    }

    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    const FTransform SpawnTransform(FRotator(0.f, 90.f, 0.f), FVector(100.f, 200.f, 300.f));
    auto SpawnWall = [&](UNiagaraSystem* Effect)
    {
        AWallStrike* Wall = World->SpawnActorDeferred<AWallStrike>(AWallStrike::StaticClass(), SpawnTransform);
        UStaticMeshComponent* Mesh = Wall->FindComponentByClass<UStaticMeshComponent>();
        Mesh->SetStaticMesh(Cube);
        Mesh->SetRelativeScale3D(FVector(0.5f, 3.f, 2.5f));
        EffectProperty->SetObjectPropertyValue_InContainer(Wall, Effect);
        Wall->FinishSpawning(SpawnTransform);
        return Wall;
    };
    auto FindEffects = [&]()
    {
        TArray<UNiagaraComponent*> Effects;
        for (TObjectIterator<UNiagaraComponent> It; It; ++It)
        {
            if (IsValid(*It) && It->GetWorld() == World && It->GetAsset() == System)
            {
                Effects.Add(*It);
            }
        }
        return Effects;
    };

    AWallStrike* Wall = SpawnWall(System);
    Wall->Tick(1.f); // Finish the default windup.
    Wall->Tick(1.f); // 800 of the default 1000 units.
    TestFalse(TEXT("Wall survives until the endpoint"), Wall->IsActorBeingDestroyed());
    TestEqual(TEXT("No premature shatter"), FindEffects().Num(), 0);
    Wall->Tick(1.f); // Overshoot clamps to the endpoint.
    TestTrue(TEXT("Wall is removed at the endpoint"), Wall->IsActorBeingDestroyed());
    TestFalse(TEXT("Finished wall cannot damage pawns"), Wall->GetActorEnableCollision());

    const TArray<UNiagaraComponent*> Effects = FindEffects();
    TestEqual(TEXT("Exactly one shatter survives wall destruction"), Effects.Num(), 1);
    if (Effects.Num() == 1)
    {
        UNiagaraComponent* Effect = Effects[0];
        TestTrue(TEXT("Effect is at the rotated slide endpoint"), Effect->GetComponentLocation().Equals(FVector(100.f, 1200.f, 300.f), 0.1f));
        TestTrue(TEXT("Effect follows wall orientation"), Effect->GetComponentRotation().Equals(FRotator(0.f, 90.f, 0.f), 0.1f));
        TestNull(TEXT("Effect is not attached to the destroyed wall"), Effect->GetAttachParent());
        bool bHasWallSize = false;
        const FVector WallSize = Effect->GetVariableVec3(TEXT("User.WallSize"), bHasWallSize);
        TestTrue(TEXT("Effect receives scaled wall dimensions"), bHasWallSize && WallSize.Equals(FVector(50.f, 300.f, 250.f), 0.1f));
    }

    AWallStrike* CancelledWall = SpawnWall(System);
    CancelledWall->Destroy();
    TestEqual(TEXT("Cancellation does not shatter"), FindEffects().Num(), Effects.Num());

    AWallStrike* WallWithoutEffect = SpawnWall(nullptr);
    WallWithoutEffect->Tick(1.f);
    WallWithoutEffect->Tick(2.f);
    TestTrue(TEXT("Unassigned effect still cleans up the wall"), WallWithoutEffect->IsActorBeingDestroyed());
    TestEqual(TEXT("Unassigned effect spawns no particles"), FindEffects().Num(), Effects.Num());
    return true;
}

#endif
