#include "Combat/HomingStrike.h"
#include "Combat/CombatUtils.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void AHomingStrike::Tick(float DeltaSeconds)
{
    // Steer before the base sweep so this frame's step already flies the corrected heading.
    if (const AActor* Target = HomingTarget.Get())
    {
        const FVector Desired = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        if (!Desired.IsNearlyZero())
        {
            const FVector Forward = GetActorForwardVector();
            const float MaxStepDeg = TurnRate * DeltaSeconds;
            const double Dot = FMath::Clamp(FVector::DotProduct(Forward, Desired), -1.0, 1.0);
            const double AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
            if (AngleDeg <= MaxStepDeg)
            {
                SetActorRotation(Desired.Rotation());
            }
            else
            {
                FVector Axis = FVector::CrossProduct(Forward, Desired).GetSafeNormal();
                if (Axis.IsNearlyZero())
                {
                    // Target dead astern: no unique turn plane, so commit to a flat turn.
                    Axis = FVector::UpVector;
                }
                SetActorRotation(Forward.RotateAngleAxis(MaxStepDeg, Axis).Rotation());
            }
        }
    }

    Super::Tick(DeltaSeconds);
}

void AHomingStrike::OnPawnHit(AActor* OtherActor, const FHitResult& SweepResult)
{
    Detonate();
}

void AHomingStrike::OnFlightEnded()
{
    Detonate();
}

void AHomingStrike::Detonate()
{
    OnDetonated();
    Rhya::DealRadialDamage(*this, Damage, ExplosionRadius);
    Destroy();
}

void AHomingStrike::OnWeaponHit(AActor* WeaponActor, const FHitResult& SweepResult)
{
    // A level-placed firer (gym dummy) has no instigator; the attack component always passes the
    // firing actor as Owner, so that's the fallback target.
    AActor* Firer = GetInstigator() ? static_cast<AActor*>(GetInstigator()) : GetOwner();
    // Same fallback on the weapon side: Equip sets both, but an owned-only weapon still
    // identifies its wielder.
    APawn* Deflector = WeaponActor ? WeaponActor->GetInstigator() : nullptr;
    if (!Deflector && WeaponActor)
    {
        Deflector = Cast<APawn>(WeaponActor->GetOwner());
    }
    if (!Firer || !Deflector || Firer == Deflector)
    {
        return;
    }

    // Chase the firer, and snap the heading so the parry reads instantly instead of TurnRate
    // slowly hauling the shot around.
    HomingTarget = Firer;
    const FVector Back = (Firer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    if (!Back.IsNearlyZero())
    {
        SetActorRotation(Back.Rotation());
    }

    // Re-attribute the shot to the deflector: the overlap guard and the radial-damage ignore
    // list key off Owner/Instigator, so this one swap flips who is protected and who is a target.
    if (AActor* OldOwner = GetOwner())
    {
        Sphere->IgnoreActorWhenMoving(OldOwner, false);
    }
    if (APawn* OldInstigator = GetInstigator())
    {
        Sphere->IgnoreActorWhenMoving(OldInstigator, false);
    }
    SetOwner(Deflector);
    SetInstigator(Deflector);
    Sphere->IgnoreActorWhenMoving(Deflector, true);

    RestartFlight();
    OnReflected();
}

AHomingStrike* AHomingStrike::SpawnConfigured(UWorld* World, TSubclassOf<AHomingStrike> Class, const FTransform& Transform, float InSpeed, float InLifeSeconds, float InTurnRate, AActor* Target, AActor* Owner, APawn* Instigator)
{
    if (!ensureMsgf(World && Class, TEXT("AHomingStrike::SpawnConfigured: missing World or Class")))
    {
        return nullptr;
    }
    if (!ensureMsgf(InSpeed > 0.f && InLifeSeconds > 0.f && InTurnRate >= 0.f, TEXT("AHomingStrike::SpawnConfigured: bad flight parameters")))
    {
        return nullptr;
    }
    if (!ensureMsgf(Target, TEXT("AHomingStrike::SpawnConfigured: missing Target")))
    {
        return nullptr;
    }

    AHomingStrike* Homing = World->SpawnActorDeferred<AHomingStrike>(Class, Transform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ensureMsgf(Homing, TEXT("AHomingStrike::SpawnConfigured: failed to spawn %s"), *Class->GetName()))
    {
        return nullptr;
    }

    Homing->Speed = InSpeed;
    // Constant speed makes lifetime and range the same check; the base range test is the timer.
    Homing->MaxRange = InSpeed * InLifeSeconds;
    Homing->TurnRate = InTurnRate;
    Homing->HomingTarget = Target;
    Homing->FinishSpawning(Transform);
    return Homing;
}
