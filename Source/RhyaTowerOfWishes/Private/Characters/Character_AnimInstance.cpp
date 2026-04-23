#include "Characters/Character_AnimInstance.h"
#include "Characters/CombatPlayerCharacter.h"
#include"GameFramework/CharacterMovementComponent.h"
#include"Kismet/KismetMathLibrary.h"

void UCharacter_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// TODO: We are not using the return value here, do we need this?
	TryGetPawnOwner();

	character = Cast<ACombatPlayerCharacter>(TryGetPawnOwner());

	if (character)
		characterMovementComponent = character->GetCharacterMovement();
}

void UCharacter_AnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (characterMovementComponent)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(characterMovementComponent->Velocity);
		IsFalling = characterMovementComponent->IsFalling();
		characterState = character->GetCharacterState();
	}
}
