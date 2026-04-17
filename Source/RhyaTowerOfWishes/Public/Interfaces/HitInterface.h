// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HitInterface.generated.h"

// This class does not need to be modified (But, I'm gonna do it anyway).
UINTERFACE(BlueprintType,meta = (CannotImplementInterfaceInBlueprint))
class UHitInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class RHYATOWEROFWISHES_API IHitInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable)
	virtual void GetHit(const FVector& impactPoint, const FVector& impactDirection) = 0;
};
