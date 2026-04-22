#include "Controllers/TOWPlayerController.h"

#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

ATOWPlayerController::ATOWPlayerController()
{
	bIsMuted = false; // Default to sound on (eventually use settings?)
	MusicVolume = 1.0;
	SfxVolume = 1.0;
}

void ATOWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("TOWPC: Setup Binding"))
		EnhancedInputComponent->BindAction(MuteAction, ETriggerEvent::Triggered, this, &ATOWPlayerController::ToggleMute);
	}
}

void ATOWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (TestMusic)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), TestMusic);
	}
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		UE_LOG(LogTemp, Warning, TEXT("TOWPC: Setup Input Context"))
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
}

void ATOWPlayerController::ToggleMute()
{
	UE_LOG(LogTemp, Warning, TEXT("[Toggle Mute]: Current = %s"), (bIsMuted?TEXT("Muted"):TEXT("Not Muted")));

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

	UE_LOG(LogTemp, Warning, TEXT("Volume: %f"), MusicVolume);
	
	// Mute all sound
	auto AudioDevice = GetWorld()->GetAudioDevice();
	if (AudioDevice)
	{
		AudioDevice->SetTransientPrimaryVolume( bIsMuted ? 1.0 : 0.0); // This will change once we are using mixers / busses
	}
	
	bIsMuted = !bIsMuted;
}
