#include "Combat/CombatUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarRadialDamageDebug(
    TEXT("Rhya.Debug.RadialDamage"), 0,
    TEXT("Radial damage diagnostics: 0 = off, 1 = log queries and returned damage, 2 = also draw the sphere and hit points."),
    ECVF_Cheat);

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

        const int32 DebugLevel = CVarRadialDamageDebug.GetValueOnGameThread();
        if (DebugLevel > 0)
        {
            UE_LOG(LogTemp, Display, TEXT("[RadialDamage] Dealer=%s Origin=%s Radius=%.2f Requested=%.2f ComponentOverlaps=%d Owner=%s Instigator=%s"),
                *Dealer.GetName(), *Dealer.GetActorLocation().ToCompactString(), Radius, DamageAmount,
                Overlaps.Num(), *GetNameSafe(Dealer.GetOwner()), *GetNameSafe(Dealer.GetInstigator()));
        }
        if (DebugLevel > 1)
        {
            DrawDebugSphere(World, Dealer.GetActorLocation(), Radius, 32, FColor::Cyan, false, 6.f);
        }

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
                FVector HitLocation;
                const float CollisionDistance = HitComponent->GetClosestPointOnCollision(RadialEvent.Origin, HitLocation);
                if (!ensureAlwaysMsgf(CollisionDistance >= 0.f,
                    TEXT("Rhya::DealRadialDamage: closest-point query failed for component %s (dealer %s)."),
                    *HitComponent->GetPathName(), *Dealer.GetPathName()))
                {
                    continue;
                }
                Damaged.Add(HitActor);
                const FVector HitNormal = (RadialEvent.Origin - HitLocation).GetSafeNormal();
                RadialEvent.ComponentHits = { FHitResult(HitActor, HitComponent, HitLocation, HitNormal) };
                const float AppliedDamage = HitActor->TakeDamage(DamageAmount, RadialEvent, InstigatorController, &Dealer);
                if (DebugLevel > 0)
                {
                    const FVector Offset = HitLocation - RadialEvent.Origin;
                    UE_LOG(LogTemp, Display, TEXT("[RadialDamage] Dealer=%s Target=%s Component=%s Overlap=true HitPoint=%s XYDistance=%.2f ZOffset=%.2f HitDistance3D=%.2f Radius=%.2f RadialScale=%.2f Requested=%.2f Applied=%.2f"),
                        *Dealer.GetName(), *HitActor->GetName(), *HitComponent->GetName(), *HitLocation.ToCompactString(),
                        Offset.Size2D(), Offset.Z, Offset.Size(), Radius,
                        RadialEvent.Params.GetDamageScale(static_cast<float>(Offset.Size())), DamageAmount, AppliedDamage);
                    if (DebugLevel > 1)
                    {
                        const FColor Color = AppliedDamage > 0.f ? FColor::Green : FColor::Orange;
                        DrawDebugLine(World, RadialEvent.Origin, HitLocation, Color, false, 6.f, 0, 2.f);
                        DrawDebugPoint(World, HitLocation, 12.f, Color, false, 6.f);
                        DrawDebugString(World, HitLocation + FVector(0.f, 0.f, 30.f),
                            FString::Printf(TEXT("Radial: %.0f applied / %.0f requested"), AppliedDamage, DamageAmount),
                            nullptr, Color, 6.f);
                    }
                }
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
