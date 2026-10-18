#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/StrikeHitTestCharacter.h"
#include "GameplayNotifyTestState.generated.h"

// Test notify with shared configuration; per-player interruption state lives on the character.
UCLASS(NotBlueprintable, Transient)
class UGameplayNotifyTestState : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    bool bAirStall = false;
    bool bWeaponWindow = false;
    bool bFinishAction = false;

    virtual void NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
        float Duration, const FAnimNotifyEventReference& Reference) override
    {
        auto* Player = Cast<AStrikeHitTestCharacter>(Mesh->GetOwner());
        if (!Player) return;
        Player->RegisterAnimNotifyState(this, Reference);
        ++Player->GameplayNotifyBegins;
        if (bAirStall) Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        if (bWeaponWindow) Player->SetWeaponCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    virtual void NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& Reference) override
    {
        auto* Player = Cast<AStrikeHitTestCharacter>(Mesh->GetOwner());
        if (!Player) return;
        const auto Reason = Player->ConsumeAnimNotifyStateInterruptReason(this, Reference);
        Player->GameplayNotifyEndReasons.Add(Reason);
        Player->GameplayNotifyEndStates.Add(Player->GetActionState());

        // Cleanup belongs here for both normal completion and interruption.
        if (bWeaponWindow) Player->SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
        if (bAirStall && Reason != EGameplayNotifyInterruptReason::Death
            && Player->GetCharacterMovement()->MovementMode == MOVE_Flying)
            Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);

        switch (Reason)
        {
        case EGameplayNotifyInterruptReason::None:
            ++Player->NormalNotifyCompletions;
            if (bFinishAction) Player->SetActionState(EactionState::EAS_Unoccupied);
            break;
        case EGameplayNotifyInterruptReason::Staggered:
        case EGameplayNotifyInterruptReason::ActionCancelled:
        case EGameplayNotifyInterruptReason::Death:
            break;
        default:
            checkNoEntry();
            break;
        }
    }
};
