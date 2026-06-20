#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatHudWidget.generated.h"

/** Native base for WBP_HUD. The bars self-bind to the player's AttributeComponent in the
 *  Blueprint on Construct (pull model); this class is the C++ home for future HUD logic. */
UCLASS()
class RHYATOWEROFWISHES_API UCombatHudWidget : public UUserWidget
{
	GENERATED_BODY()
};
