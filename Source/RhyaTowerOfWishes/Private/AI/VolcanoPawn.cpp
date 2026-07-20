#include "AI/VolcanoPawn.h"
#include "AI/VolcanoAIController.h"
#include "Components/AC_LobbedProjectile.h"
#include "Combat/CombatUtils.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

AVolcanoPawn::AVolcanoPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    LobComponent = CreateDefaultSubobject<UAC_LobbedProjectile>(TEXT("LobComponent"));

    AIControllerClass = AVolcanoAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AVolcanoPawn::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoStart)
    {
        StartEruptions();
    }
}

void AVolcanoPawn::StartEruptions()
{
    bEruptionsActive = true;
}

void AVolcanoPawn::StopEruptions()
{
    bEruptionsActive = false;
}

static FAutoConsoleCommandWithWorldAndArgs GRhyaDebugEruption(
    TEXT("Rhya.Debug.Eruption"),
    TEXT("No arg: toggles StartEruptions/StopEruptions on the first AVolcanoPawn in the world. "
         "Numeric arg N: throws a one-off volley of N bombs at the player, bypassing the behavior tree."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        World = Rhya::FindGameWorld(World);
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.Eruption: no running game world"));
            return;
        }

        // Not FindFirstComponent<UAC_LobbedProjectile>: the gym's lava-bomb dummy owns one too.
        AVolcanoPawn* Volcano = nullptr;
        for (TActorIterator<AVolcanoPawn> It(World); It; ++It)
        {
            Volcano = *It;
            break;
        }
        if (!Volcano)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.Eruption: no AVolcanoPawn in world"));
            return;
        }

        if (Args.Num() == 0)
        {
            if (Volcano->AreEruptionsActive())
            {
                Volcano->StopEruptions();
            }
            else
            {
                Volcano->StartEruptions();
            }
            UE_LOG(LogTemp, Log, TEXT("Rhya.Debug.Eruption: eruptions %s"),
                Volcano->AreEruptionsActive() ? TEXT("ON") : TEXT("OFF"));
            return;
        }

        APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
        if (!Player)
        {
            UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.Eruption: no player pawn to aim at"));
            return;
        }

        const int32 N = FMath::Max(1, FCString::Atoi(*Args[0]));
        for (int32 Index = 0; Index < N; ++Index)
        {
            const FVector2D Scatter = FMath::RandPointInCircle(250.f);
            const FVector Candidate = Player->GetActorLocation() + FVector(Scatter.X, Scatter.Y, 0.f);

            FVector FloorPoint;
            if (!Rhya::SnapToFloor(*World, Candidate, Player, FloorPoint))
            {
                UE_LOG(LogTemp, Warning, TEXT("Rhya.Debug.Eruption: no floor under %s"), *Candidate.ToString());
                continue;
            }
            Volcano->LobComponent->ThrowAt(FloorPoint);
        }
        UE_LOG(LogTemp, Log, TEXT("Rhya.Debug.Eruption: threw %d bomb(s) at the player"), N);
    }),
    ECVF_Cheat);
