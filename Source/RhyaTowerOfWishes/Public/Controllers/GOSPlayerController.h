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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float MusicVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float SfxVolume;

	UPROPERTY(EditDefaultsOnly, Category = "DEBUG")
	TObjectPtr<USoundBase> TestMusic;

	// -- Variables -----------------------------------------------------------
	UUserWidget* PauseScreenWidget;
};
