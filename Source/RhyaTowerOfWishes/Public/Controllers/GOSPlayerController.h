#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GOSPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class RHYATOWEROFWISHES_API AGOSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// -- Functions -----------------------------------------------------------
	AGOSPlayerController();
	virtual void SetupInputComponent() override;

	// -- Properties ----------------------------------------------------------

	// -- Variables -----------------------------------------------------------

protected:
	// -- Functions -----------------------------------------------------------
	virtual void BeginPlay() override;
	void ToggleMute();

	/**
	 * Toggle the pause screen + game pause. Exposed to Blueprint so the pause menu's
	 * Resume button (and its CommonUI back handler) can close the menu through the
	 * same path the Pause input uses — restoring input mode and clearing the widget,
	 * which a plain self-RemoveFromParent in the widget would not do.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void TogglePause();

	// -- Properties ----------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> MuteAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseScreen;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsMuted;

	UPROPERTY(EditDefaultsOnly, Category = "DEBUG")
	TObjectPtr<USoundBase> TestMusic;

	// -- Variables -----------------------------------------------------------
	UPROPERTY()
	TObjectPtr<UUserWidget> PauseScreenWidget;
};
