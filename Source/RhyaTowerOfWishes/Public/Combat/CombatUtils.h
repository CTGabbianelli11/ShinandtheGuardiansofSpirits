#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectIterator.h"

class AActor;
class UWorld;

namespace Rhya
{
    // Finds the ground under Point (probes +500/-1000 on Z). Traces WorldStatic objects only, so
    // pawns and other dynamic actors standing in the area are never mistaken for the floor.
    RHYATOWEROFWISHES_API bool SnapToFloor(const UWorld& World, const FVector& Point, const AActor* IgnoredActor, FVector& OutFloorPoint);

    // Resolves the running game world for console commands, which receive the EDITOR world when
    // typed into the editor's console instead of PIE's.
    RHYATOWEROFWISHES_API UWorld* FindGameWorld(UWorld* World);

    // Deals DamageAmount once to every pawn within Radius of Dealer's location as RADIAL damage
    RHYATOWEROFWISHES_API void DealRadialDamage(AActor& Dealer, float DamageAmount, float Radius);

    // First registered component of type T in World; the debug-cheat lookup.
    template <typename T>
    T* FindFirstComponent(UWorld* World)
    {
        for (TObjectIterator<T> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsRegistered())
            {
                return *It;
            }
        }
        return nullptr;
    }
}
