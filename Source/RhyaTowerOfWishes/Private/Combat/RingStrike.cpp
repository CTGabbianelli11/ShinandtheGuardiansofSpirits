#include "Combat/RingStrike.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

ARingStrike::ARingStrike()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Effect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ImpactEffect"));
    Effect->SetupAttachment(GetRootComponent());
    Effect->SetAutoActivate(false);
}

void ARingStrike::Configure(const FRadiatingAttackSettings& InSettings, float InInnerRadius, float InOuterRadius)
{
    Settings = InSettings;
    InnerRadius = InInnerRadius;
    Radius = InOuterRadius;
}

void ARingStrike::BeginPlay()
{
    Super::BeginPlay();
    if (!ensureMsgf(FMath::IsFinite(Radius) && FMath::IsFinite(InnerRadius)
        && Radius > InnerRadius && InnerRadius >= 0.f
        && FMath::IsFinite(Settings.DamageHalfHeight) && Settings.DamageHalfHeight > 0.f
        && FMath::IsFinite(Settings.Damage) && Settings.Damage >= 0.f
        && FMath::IsFinite(Settings.ImpactLifetime) && Settings.ImpactLifetime > 0.f,
        TEXT("%s: invalid ring strike configuration"), *GetName()))
    {
        Destroy();
        return;
    }

    SetLifeSpan(Settings.ImpactLifetime);
    ApplyRingDamage();
    if (Settings.ImpactEffect)
    {
        Effect->SetAsset(Settings.ImpactEffect);
        Effect->SetVariableFloat(TEXT("User.InnerRadius"), InnerRadius);
        Effect->SetVariableFloat(TEXT("User.OuterRadius"), Radius);
        Effect->SetVariableFloat(TEXT("User.ImpactLifetime"), Settings.ImpactLifetime);
        Effect->Activate(true);
    }
    if (Settings.ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, Settings.ImpactSound, GetActorLocation());
    }
}

void ARingStrike::ApplyRingDamage()
{
    if (Settings.Damage == 0.f)
    {
        return;
    }
    const FVector Center = GetActorLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RingStrike), false, this);
    if (GetOwner()) Query.AddIgnoredActor(GetOwner());
    if (GetInstigator()) Query.AddIgnoredActor(GetInstigator());

    // A box broadphase preserves the full ring radius at every height in the slab.
    TArray<FOverlapResult> Overlaps;
    GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        FCollisionShape::MakeBox(FVector(Radius, Radius, Settings.DamageHalfHeight)), Query);

    TSet<AActor*> Damaged;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        APawn* Pawn = Cast<APawn>(Overlap.GetActor());
        UPrimitiveComponent* Component = Overlap.GetComponent();
        if (!IsValid(Pawn) || !Component || Damaged.Contains(Pawn)) continue;

        const double DistanceSquared = FVector::DistSquaredXY(Center, Pawn->GetActorLocation());
        // Shared boundaries belong to the inner pulse; the first pulse includes the center.
        if (DistanceSquared > FMath::Square(static_cast<double>(Radius))
            || (InnerRadius > 0.f && DistanceSquared <= FMath::Square(static_cast<double>(InnerRadius)))) continue;

        Damaged.Add(Pawn);
        // Keep the radial event's hit inside the horizontal footprint, including for tall pawns.
        const FVector HitPoint(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, Center.Z);
        FRadialDamageEvent Event;
        Event.Origin = Center;
        Event.Params = FRadialDamageParams(Settings.Damage, Radius, Radius, 1.f);
        Event.ComponentHits = { FHitResult(Pawn, Component, HitPoint, (Center - HitPoint).GetSafeNormal()) };
        Pawn->TakeDamage(Settings.Damage, Event, GetInstigatorController(), this);
    }
}
