#include "Combat/StrikeActor.h"
#include "Engine/World.h"

AStrikeActor* AStrikeActor::SpawnConfigured(UWorld* World, TSubclassOf<AStrikeActor> Class, const FTransform& Transform, float Radius, AActor* Owner, APawn* Instigator)
{
    if (!ensureMsgf(World && Class, TEXT("AStrikeActor::SpawnConfigured: missing World or Class")))
    {
        return nullptr;
    }

    AStrikeActor* Strike = World->SpawnActorDeferred<AStrikeActor>(Class, Transform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ensureMsgf(Strike, TEXT("AStrikeActor::SpawnConfigured: failed to spawn %s"), *Class->GetName()))
    {
        return nullptr;
    }

    // Radius is a plain property, safe to set before FinishSpawning: the Construction Script
    // (which FinishSpawning runs) reads it to size the strike's reach.
    Strike->Radius = Radius;
    Strike->FinishSpawning(Transform);
    return Strike;
}
