#pragma once

#include "Characters/CombatPlayerCharacter.h"
#include "Engine/DamageEvents.h"
#include "UObject/Class.h"
#include "StrikeHitTestCharacter.generated.h"

// Native receiver for strike integration tests; no Blueprint assets or animation setup required.
UCLASS(NotBlueprintable, Transient)
class AStrikeHitTestCharacter : public ACombatPlayerCharacter
{
    GENERATED_BODY()

public:
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer) override
    {
        ++DamageCalls;
        bPointDamage = Event.IsOfType(FPointDamageEvent::ClassID);
        bRadialDamage = Event.IsOfType(FRadialDamageEvent::ClassID);
        AppliedDamage = Super::TakeDamage(Amount, Event, EventInstigator, Causer);
        if (bDestroyOnDamage) Destroy();
        return AppliedDamage;
    }

    virtual void GetHit_Implementation(const FVector& Point, const FVector& Direction, const float& Amount) override
    {
        ++HitCalls;
        bDamageBeforeHit = DamageCalls == HitCalls;
        HitPoint = Point;
        HitDirection = Direction;
        HitDamage = Amount;
        Super::GetHit_Implementation(Point, Direction, Amount);
    }

    virtual void ProcessEvent(UFunction* Function, void* Parameters) override
    {
        if (Function->GetFName() == TEXT("CharacterBlockedHit")) bLastHitBlocked = true;
        if (Function->GetFName() == TEXT("CharacterHit")) bLastHitBlocked = false;
        Super::ProcessEvent(Function, Parameters);
    }

    int32 DamageCalls = 0;
    int32 HitCalls = 0;
    float AppliedDamage = 0.f;
    float HitDamage = 0.f;
    bool bDamageBeforeHit = false;
    bool bLastHitBlocked = false;
    bool bDestroyOnDamage = false;
    bool bPointDamage = false;
    bool bRadialDamage = false;
    FVector HitPoint = FVector::ZeroVector;
    FVector HitDirection = FVector::ZeroVector;
};
