#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatHudWidget.generated.h"

class UAttributeComponent;

/**
 * Base class for the gameplay HUD (WBP_HUD). Uses a "push" model: the owning player
 * calls InitializeHud once on BeginPlay, handing the widget the AttributeComponent to
 * read from. The Blueprint implementation subscribes its bars to that component's
 * OnHealthPercentUpdateDelegate / OnMagicPercentUpdateDelegate and seeds their initial
 * values — wiring authored through the MCP bridge's bind_event_to_delegate action.
 */
UCLASS()
class RHYATOWEROFWISHES_API UCombatHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void InitializeHud(UAttributeComponent* Attributes);
};
