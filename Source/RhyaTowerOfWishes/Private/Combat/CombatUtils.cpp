#include "Combat/CombatUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace Rhya
{
    void DealRadialDamage(AActor& Dealer, float DamageAmount, float Radius)
    {
        UWorld* World = Dealer.GetWorld();

        FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RadialDamage), false, &Dealer);
        if (Dealer.GetOwner())
        {
            QueryParams.AddIgnoredActor(Dealer.GetOwner());
        }
        if (Dealer.GetInstigator())
        {
            QueryParams.AddIgnoredActor(Dealer.GetInstigator());
        }

        TArray<FOverlapResult> Overlaps;
        World->OverlapMultiByObjectType(Overlaps, Dealer.GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), QueryParams);

        AController* InstigatorController = Dealer.GetInstigator() ? Dealer.GetInstigator()->GetController() : nullptr;

        FRadialDamageEvent RadialEvent;
        RadialEvent.Origin = Dealer.GetActorLocation();
        RadialEvent.Params = FRadialDamageParams(DamageAmount, Radius, Radius, 1.f);

        TSet<AActor*> Damaged;
        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* HitActor = Overlap.GetActor();
            UPrimitiveComponent* HitComponent = Overlap.GetComponent();
            if (HitActor && HitComponent && !Damaged.Contains(HitActor))
            {
                Damaged.Add(HitActor);
                const FVector HitLocation = HitComponent->GetComponentLocation();
                RadialEvent.ComponentHits = { FHitResult(HitActor, HitComponent, HitLocation, (HitLocation - RadialEvent.Origin).GetSafeNormal()) };
                HitActor->TakeDamage(DamageAmount, RadialEvent, InstigatorController, &Dealer);
            }
        }
    }

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
