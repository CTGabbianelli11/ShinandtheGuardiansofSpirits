#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DeathInterface.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UDeathInterface : public UInterface
{
	GENERATED_BODY()
};

class RHYATOWEROFWISHES_API IDeathInterface
{
	GENERATED_BODY()

public:
	// Death NOTIFICATION, not a kill command: invoked only by UAttributeComponent's
	// alive->dead health edge, exactly once per life. 
	UFUNCTION(BlueprintCallable)
	virtual void CharacterDied() = 0;
};
