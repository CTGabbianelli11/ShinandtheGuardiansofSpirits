#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "RhyaGameUserSettings.generated.h"

class USoundControlBus;

/**
 * Audio volume categories. Each maps to its own control bus, which modulates the
 * matching submix's output volume (Master -> SSM_Master_Bus, Music -> SSM_Music,
 * SFX -> SSM_SFX). Master rides everything; Music/SFX ride their submix groups.
 */
UENUM(BlueprintType)
enum class EVolumeCategory : uint8
{
	Master,
	Music,
	SFX,

	/** Sentinel for compile-time exhaustiveness checks — not a real category, keep last. */
	Count UMETA(Hidden)
};

/**
 * Project-wide user settings.
 *
 * Extends UGameUserSettings — the engine's config-backed, auto-loaded-at-startup
 * settings singleton (persisted to GameUserSettings.ini). Adds the game's own
 * preferences; currently per-category audio volumes (Master/Music/SFX), each driven
 * onto its control bus via Audio Modulation.
 *
 * Registered as the active settings class via GameUserSettingsClassName in
 * Config/DefaultEngine.ini, so GEngine->GetGameUserSettings() returns this type.
 *
 * The volume API is category-generic (GetVolume/SetVolume take an EVolumeCategory)
 * so one reusable slider widget can drive any category and adding a category later
 * is one enum entry + one bus.
 */
UCLASS()
class RHYATOWEROFWISHES_API URhyaGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	URhyaGameUserSettings();

	/** Typed accessor for the engine's GameUserSettings singleton (null until the class is registered). */
	UFUNCTION(BlueprintPure, Category = "Settings")
	static URhyaGameUserSettings* GetRhyaGameUserSettings();

	/** Current volume for a category, 0..1. */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetVolume(EVolumeCategory Category) const;

	/**
	 * Set a category's volume (clamped 0..1) and apply it live to that category's bus.
	 * Does NOT persist — call SaveSettings() when the options screen closes, so a
	 * dragged slider doesn't rewrite the ini every frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio", meta = (WorldContext = "WorldContextObject"))
	void SetVolume(const UObject* WorldContextObject, EVolumeCategory Category, float InVolume);

	/** Push every category's saved volume onto its bus. Call once on startup (after world/audio exist). */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio", meta = (WorldContext = "WorldContextObject"))
	void ApplyAllVolumes(const UObject* WorldContextObject) const;

	/** Drive the master bus to silence (muted) or the saved master volume, without changing the saved value. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio", meta = (WorldContext = "WorldContextObject"))
	void ApplyMuted(const UObject* WorldContextObject, bool bMuted) const;

	//~ Begin UGameUserSettings interface
	virtual void SetToDefaults() override;
	//~ End UGameUserSettings interface

private:
	/** Resolve a category's control bus and drive it to an explicit 0..1 value. */
	void ApplyVolumeToBus(const UObject* WorldContextObject, EVolumeCategory Category, float Value) const;

	/** The control bus that drives a given category. */
	const TSoftObjectPtr<USoundControlBus>& BusForCategory(EVolumeCategory Category) const;

	/** Persisted per-category volumes, 0..1. Saved to GameUserSettings.ini. */
	UPROPERTY(config)
	float MasterVolume;

	UPROPERTY(config)
	float MusicVolume;

	UPROPERTY(config)
	float SfxVolume;

	/** Control bus each category drives. Master = CB_Volume (kept as-is to preserve its
	 *  existing SSM_Master_Bus modulation wire); Music/SFX = CB_Music / CB_SFX. */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TSoftObjectPtr<USoundControlBus> MasterVolumeBus;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TSoftObjectPtr<USoundControlBus> MusicVolumeBus;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TSoftObjectPtr<USoundControlBus> SfxVolumeBus;
};
