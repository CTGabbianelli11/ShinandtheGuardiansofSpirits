#include "Combat/CombatUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace Rhya
{
    bool SnapToFloor(const UWorld& World, const FVector& Point, const AActor* IgnoredActor, FVector& OutFloorPoint)
    {
        FHitResult Hit;
        const FVector Start = Point + FVector(0.f, 0.f, 500.f);
        const FVector End = Point - FVector(0.f, 0.f, 1000.f);
        if (!World.LineTraceSingleByObjectType(
                Hit, Start, End,
                FCollisionObjectQueryParams(ECC_WorldStatic),
                FCollisionQueryParams(SCENE_QUERY_STAT(FloorSnap), false, IgnoredActor)))
        {
            return false;
        }
        OutFloorPoint = FVector(Hit.ImpactPoint);
        return true;
    }

    UWorld* FindGameWorld(UWorld* World)
    {
        if (World && World->IsGameWorld())
        {
            return World;
        }
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.World() && Context.World()->IsGameWorld())
            {
                return Context.World();
            }
        }
        return nullptr;
    }
}
