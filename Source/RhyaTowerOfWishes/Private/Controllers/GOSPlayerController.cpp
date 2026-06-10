#include "Controllers/GOSPlayerController.h"

#include "Settings/RhyaGameUserSettings.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AGOSPlayerController::AGOSPlayerController()
{
	bIsMuted = false; // Session-only by design; persisted volumes live in URhyaGameUserSettings.
}

void AGOSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MuteAction, ETriggerEvent::Triggered, this, &AGOSPlayerController::ToggleMute);
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &AGOSPlayerController::TogglePause);
	}
}

void AGOSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (TestMusic)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), TestMusic);
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}

	// Push the saved per-category volumes onto their buses now that the world +
	// audio device exist.
	if (URhyaGameUserSettings* Settings = URhyaGameUserSettings::GetRhyaGameUserSettings())
	{
		Settings->ApplyAllVolumes(this);
	}
}

void AGOSPlayerController::ToggleMute()
{
	bIsMuted = !bIsMuted;

	if (URhyaGameUserSettings* Settings = URhyaGameUserSettings::GetRhyaGameUserSettings())
	{
		Settings->ApplyMuted(this, bIsMuted);
	}
}

void AGOSPlayerController::TogglePause()
{
	if (IsPaused())
	{
		bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		SetInputMode(InputMode);

		// Remove the pause screen and unpause. Destroying it (instead of just
		// hiding) means the next pause recreates a fresh CommonActivatableWidget,
		// which re-activates and restores gamepad focus (DesiredFocusWidget).
		if (PauseScreenWidget)
		{
			PauseScreenWidget->RemoveFromParent();
			PauseScreenWidget = nullptr;
		}
	}
	else
	{
		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);

		// Create the pause screen fresh so the CommonActivatableWidget auto-activates
		// and grabs gamepad focus. It is destroyed again on unpause (above).
		PauseScreenWidget = CreateWidget(this, PauseScreen);
		if (PauseScreenWidget)
		{
			PauseScreenWidget->AddToViewport();
		}
	}

	UGameplayStatics::SetGamePaused(GetWorld(), !IsPaused());
}
