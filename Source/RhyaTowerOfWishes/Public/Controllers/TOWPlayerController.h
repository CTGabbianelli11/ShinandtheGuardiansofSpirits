#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "TOWPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class RHYATOWEROFWISHES_API ATOWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// -- Functions -----------------------------------------------------------
	ATOWPlayerController();
	virtual void SetupInputComponent() override;

	// -- Properties ----------------------------------------------------------

	// -- Variables -----------------------------------------------------------

protected:
	// -- Functions -----------------------------------------------------------
	virtual void BeginPlay() override;
	void ToggleMute();

	// -- Properties ----------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> MuteAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsMuted;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float MusicVolume;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float SfxVolume;
	
	UPROPERTY(EditDefaultsOnly,Category = "DEBUG")
	TObjectPtr<USoundBase> TestMusic;
	
	// -- Variables -----------------------------------------------------------

};
