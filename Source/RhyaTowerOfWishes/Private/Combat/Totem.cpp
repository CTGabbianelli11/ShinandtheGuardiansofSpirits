#include "Combat/Totem.h"
#include "Components/AttributeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ATotem::ATotem()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
    // Hard-set here so a BP child can't silently break the damage path: WorldDynamic blocking
    // Visibility is what the sword's box trace hits, while the boss's own strikes (which damage
    // Pawns only) ignore it - the player alone can resolve the phase.
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Mesh->SetGenerateOverlapEvents(true);

    Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
}

ATotem* ATotem::SpawnConfigured(UWorld* World, TSubclassOf<ATotem> Class, const FTransform& Transform, AActor* Owner, APawn* Instigator)
{
    if (!ensureMsgf(World && Class, TEXT("ATotem::SpawnConfigured: missing World or Class")))
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = Owner;
    Params.Instigator = Instigator;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATotem* Totem = World->SpawnActor<ATotem>(Class, Transform, Params);
    ensureMsgf(Totem, TEXT("ATotem::SpawnConfigured: failed to spawn %s"), *Class->GetName());
    return Totem;
}

float ATotem::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // No death check here: the AttributeComponent's alive->dead edge calls CharacterDied() itself.
    Attributes->ReceiveDamage(DamageAmount);
    return DamageAmount;
}

void ATotem::GetHit_Implementation(const FVector& impactPoint, const FVector& impactDirection)
{
    OnTotemHit(impactPoint, impactDirection);
}

void ATotem::CharacterDied()
{
    if (!ensureMsgf(!bDied, TEXT("%s: CharacterDied called twice - the AttributeComponent death edge fires exactly once"), *GetName()))
    {
        return;
    }
    bDied = true;

    OnTotemDied();
    Destroy();
}
