#include "Settings/RhyaGameUserSettings.h"

#include "AudioModulationStatics.h"
#include "Engine/Engine.h"
#include "SoundControlBus.h"

/** Fade applied to bus mix changes so slider drags and mute transitions don't pop. */
static constexpr float VolumeFadeSeconds = 0.1f;

URhyaGameUserSettings::URhyaGameUserSettings()
{
	static_assert(static_cast<uint8>(EVolumeCategory::Count) == 3,
		"New EVolumeCategory entry: add its default volume and bus here.");
	MasterVolume = 1.0f;
	MusicVolume = 1.0f;
	SfxVolume = 1.0f;

	MasterVolumeBus = TSoftObjectPtr<USoundControlBus>(
		FSoftObjectPath(TEXT("/Game/Audio/Control_Mixes/CB_Volume.CB_Volume")));
	MusicVolumeBus = TSoftObjectPtr<USoundControlBus>(
		FSoftObjectPath(TEXT("/Game/Audio/Control_Mixes/CB_Music.CB_Music")));
	SfxVolumeBus = TSoftObjectPtr<USoundControlBus>(
		FSoftObjectPath(TEXT("/Game/Audio/Control_Mixes/CB_SFX.CB_SFX")));
}

URhyaGameUserSettings* URhyaGameUserSettings::GetRhyaGameUserSettings()
{
	return GEngine ? Cast<URhyaGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void URhyaGameUserSettings::SetToDefaults()
{
	static_assert(static_cast<uint8>(EVolumeCategory::Count) == 3,
		"New EVolumeCategory entry: reset its default volume here.");
	Super::SetToDefaults();
	MasterVolume = 1.0f;
	MusicVolume = 1.0f;
	SfxVolume = 1.0f;
}

float URhyaGameUserSettings::GetVolume(EVolumeCategory Category) const
{
	static_assert(static_cast<uint8>(EVolumeCategory::Count) == 3,
		"New EVolumeCategory entry: add its volume property to this switch.");
	switch (Category)
	{
	case EVolumeCategory::Master:
		return MasterVolume;
	case EVolumeCategory::Music:
		return MusicVolume;
	case EVolumeCategory::SFX:
		return SfxVolume;
	default:
		checkNoEntry();
		return MasterVolume;
	}
}

void URhyaGameUserSettings::SetVolume(const UObject* WorldContextObject, EVolumeCategory Category, float InVolume)
{
	static_assert(static_cast<uint8>(EVolumeCategory::Count) == 3,
		"New EVolumeCategory entry: add its volume property to this switch.");
	const float Clamped = FMath::Clamp(InVolume, 0.0f, 1.0f);
	switch (Category)
	{
	case EVolumeCategory::Master:
		MasterVolume = Clamped;
		break;
	case EVolumeCategory::Music:
		MusicVolume = Clamped;
		break;
	case EVolumeCategory::SFX:
		SfxVolume = Clamped;
		break;
	default:
		checkNoEntry();
		return;
	}
	ApplyVolumeToBus(WorldContextObject, Category, Clamped);
}

void URhyaGameUserSettings::ApplyAllVolumes(const UObject* WorldContextObject) const
{
	for (uint8 Index = 0; Index < static_cast<uint8>(EVolumeCategory::Count); ++Index)
	{
		const EVolumeCategory Category = static_cast<EVolumeCategory>(Index);
		ApplyVolumeToBus(WorldContextObject, Category, GetVolume(Category));
	}
}

void URhyaGameUserSettings::ApplyMuted(const UObject* WorldContextObject, bool bMuted) const
{
	// Mute rides the master bus to silence (everything funnels through master),
	// without changing the saved per-category values.
	ApplyVolumeToBus(WorldContextObject, EVolumeCategory::Master, bMuted ? 0.0f : MasterVolume);
}

const TSoftObjectPtr<USoundControlBus>& URhyaGameUserSettings::BusForCategory(EVolumeCategory Category) const
{
	static_assert(static_cast<uint8>(EVolumeCategory::Count) == 3,
		"New EVolumeCategory entry: add its control bus to this switch.");
	switch (Category)
	{
	case EVolumeCategory::Master:
		return MasterVolumeBus;
	case EVolumeCategory::Music:
		return MusicVolumeBus;
	case EVolumeCategory::SFX:
		return SfxVolumeBus;
	default:
		checkNoEntry();
		return MasterVolumeBus;
	}
}

void URhyaGameUserSettings::ApplyVolumeToBus(const UObject* WorldContextObject, EVolumeCategory Category, float Value) const
{
	if (!ensure(WorldContextObject))
	{
		return;
	}

	// LoadSynchronous is fine here: the buses are tiny and this runs on settings
	// changes / startup, not per-frame.
	USoundControlBus* Bus = BusForCategory(Category).LoadSynchronous();
	if (!ensureMsgf(Bus, TEXT("Control bus for %s failed to load (%s)"),
			*UEnum::GetValueAsString(Category), *BusForCategory(Category).ToString()))
	{
		return;
	}

	UAudioModulationStatics::SetGlobalBusMixValue(WorldContextObject, Bus, Value, VolumeFadeSeconds);
}
