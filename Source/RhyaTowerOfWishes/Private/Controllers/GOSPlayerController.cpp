#include "Controllers/GOSPlayerController.h"

#include "Settings/RhyaGameUserSettings.h"
// EnhancedInput inline code narrows double->float; engine-owned, exempt from UnsafeTypeCastWarningLevel.
PRAGMA_DISABLE_UNSAFE_TYPECAST_WARNINGS
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
PRAGMA_RESTORE_UNSAFE_TYPECAST_WARNINGS
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
