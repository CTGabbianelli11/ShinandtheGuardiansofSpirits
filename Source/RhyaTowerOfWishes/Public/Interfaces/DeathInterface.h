// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DeathInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UDeathInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class RHYATOWEROFWISHES_API IDeathInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable)
    virtual void CharacterDied() = 0;
};
