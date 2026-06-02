#include "Controllers/GOSPlayerController.h"

#include "AudioDevice.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AGOSPlayerController::AGOSPlayerController()
{
	bIsMuted = false; // Default to sound on (eventually use settings?)
	MusicVolume = 1.0;
	SfxVolume = 1.0;
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
}

void AGOSPlayerController::ToggleMute()
{
	// TODO: if we use mixers, will need to read those settings here - Not currenlty used, but will be with the busses
	MusicVolume = bIsMuted ? 1.0 : 0.0;
	SfxVolume = bIsMuted ? 1.0 : 0.0;

	// TODO: if we are using mixers we would need to add those here with a prop

	/*
	 This was the original BP style code, using mixers and sound overriders
	// Music
	UGameplayStatics::SetSoundMixClassOverride(
		GetWorld(),
		nullptr,
		nullptr,
		MusicVolume
	);

	// SFX
	UGameplayStatics::SetSoundMixClassOverride(
		GetWorld(),
		nullptr,
		nullptr,
		SfxVolume
	);
	*/

	// Mute all sound
	auto AudioDevice = GetWorld()->GetAudioDevice();
	if (AudioDevice)
	{
		AudioDevice->SetTransientPrimaryVolume(bIsMuted ? 1.0 : 0.0); // This will change once we are using mixers / busses
	}

	bIsMuted = !bIsMuted;
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
