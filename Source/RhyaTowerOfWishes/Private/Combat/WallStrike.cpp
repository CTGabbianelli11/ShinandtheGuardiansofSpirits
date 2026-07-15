#include "Combat/WallStrike.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AWallStrike::AWallStrike()
{
    PrimaryActorTick.bCanEverTick = true;

    // The box must be the root: a swept move only sweeps the root component; children teleport.
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    SetRootComponent(Box);
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldDynamic);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Box->SetGenerateOverlapEvents(true);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Box);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // The dormant wall stands inside its own telegraph's projection volume; keep the decal off it.
    Mesh->SetReceivesDecals(false);
}

void AWallStrike::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (Mesh->GetStaticMesh())
    {
        const FBox MeshBounds = Mesh->GetStaticMesh()->GetBoundingBox().TransformBy(Mesh->GetRelativeTransform());
        Box->SetBoxExtent(MeshBounds.GetExtent());
    }
}

void AWallStrike::BeginPlay()
{
    Super::BeginPlay();

    // Dormant during the telegraph: visible but hitless until the slide starts.
    SetActorEnableCollision(false);
    Box->OnComponentBeginOverlap.AddDynamic(this, &AWallStrike::OnBoxBeginOverlap);
}

FVector AWallStrike::GetCollisionExtent() const
{
    return Box->GetScaledBoxExtent();
}

void AWallStrike::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    Elapsed += DeltaSeconds;

    switch (Phase)
    {
    case EPhase::Dormant:
    {
        if (Elapsed >= WindupTime)
        {
            Phase = EPhase::Sliding;
            // Arming also fires Begin events for anyone already standing inside the wall.
            SetActorEnableCollision(true);
        }
        break;
    }
    case EPhase::Sliding:
    {
        const float Step = FMath::Min(SlideSpeed * DeltaSeconds, TravelDistance - Traveled);
        // bSweep=true so pawns crossed between ticks still get detected
        AddActorWorldOffset(GetActorForwardVector() * Step, true);
        Traveled += Step;
        if (Traveled >= TravelDistance)
        {
            Destroy();
        }
        break;
    }
    }
}

void AWallStrike::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator() || DamagedActors.Contains(OtherActor))
    {
        return;
    }
    DamagedActors.Add(OtherActor);

    AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
    UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorController, this, nullptr);
}

AWallStrike* AWallStrike::SpawnConfigured(UWorld* World, TSubclassOf<AWallStrike> Class, const FTransform& Transform, float InWindupTime, float InSlideSpeed, float InTravelDistance, AActor* Owner, APawn* Instigator)
{
    if (!ensureMsgf(World && Class, TEXT("AWallStrike::SpawnConfigured: missing World or Class")))
    {
        return nullptr;
    }

    AWallStrike* Wall = World->SpawnActorDeferred<AWallStrike>(Class, Transform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ensureMsgf(Wall, TEXT("AWallStrike::SpawnConfigured: failed to spawn %s"), *Class->GetName()))
    {
        return nullptr;
    }

    Wall->WindupTime = InWindupTime;
    Wall->SlideSpeed = InSlideSpeed;
    Wall->TravelDistance = InTravelDistance;
    Wall->FinishSpawning(Transform);
    return Wall;
}
