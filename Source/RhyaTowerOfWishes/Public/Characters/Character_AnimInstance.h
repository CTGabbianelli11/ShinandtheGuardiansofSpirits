// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/CharacterTypes.h"
#include "Character_AnimInstance.generated.h"




UCLASS()
class RHYATOWEROFWISHES_API UCharacter_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()


public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float deltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	class ACombatPlayerCharacter* character;
	
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	class UCharacterMovementComponent* characterMovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float GroundSpeed;
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool IsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Movement | Character State")
	ECharacterState characterState;

protected:
	//UMovementComponent* movementComponent;
	
};
