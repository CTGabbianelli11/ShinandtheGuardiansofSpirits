#include "UI/RhyaVolumeSlider.h"

#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Settings/RhyaGameUserSettings.h"

void URhyaVolumeSlider::NativeConstruct()
{
	Super::NativeConstruct();

	if (Label)
	{
		Label->SetText(GetCategoryDisplayName());
	}

	const URhyaGameUserSettings* Settings = URhyaGameUserSettings::GetRhyaGameUserSettings();
	ensureMsgf(Settings, TEXT("URhyaGameUserSettings is not the active settings class; check GameUserSettingsClassName in DefaultEngine.ini"));
	const float Volume = Settings ? Settings->GetVolume(Category) : 1.0f;

	UpdateValueText(Volume);

	if (VolumeSlider)
	{
		// Seed the slider from the saved value without re-broadcasting OnValueChanged.
		VolumeSlider->SetValue(Volume);
		// AddUnique so a reconstruct (widget re-added to a menu) doesn't stack bindings.
		VolumeSlider->OnValueChanged.AddUniqueDynamic(this, &URhyaVolumeSlider::HandleValueChanged);
	}
}

void URhyaVolumeSlider::NativeDestruct()
{
	// The owning menu is closing (its sliders are being torn down) — commit the live
	// volumes to disk so they survive an app restart. SaveSettings is cheap and
	// idempotent, so redundant writes from sibling sliders closing together are fine.
	if (URhyaGameUserSettings* Settings = URhyaGameUserSettings::GetRhyaGameUserSettings())
	{
		Settings->SaveSettings();
	}

	Super::NativeDestruct();
}

void URhyaVolumeSlider::HandleValueChanged(float NewValue)
{
	if (URhyaGameUserSettings* Settings = URhyaGameUserSettings::GetRhyaGameUserSettings())
	{
		Settings->SetVolume(this, Category, NewValue);
	}

	UpdateValueText(NewValue);
}

FText URhyaVolumeSlider::GetCategoryDisplayName() const
{
	static_assert(static_cast<uint8>(EVolumeCategory::Count) == 3,
		"New EVolumeCategory entry: add its display name to this switch.");
	switch (Category)
	{
	case EVolumeCategory::Master:
		return NSLOCTEXT("RhyaVolume", "CategoryMaster", "Master");
	case EVolumeCategory::Music:
		return NSLOCTEXT("RhyaVolume", "CategoryMusic", "Music");
	case EVolumeCategory::SFX:
		return NSLOCTEXT("RhyaVolume", "CategorySFX", "SFX");
	default:
		checkNoEntry();
		return FText::GetEmpty();
	}
}

void URhyaVolumeSlider::UpdateValueText(float Volume01) const
{
	if (ValueText)
	{
		const int32 Percent = FMath::RoundToInt(FMath::Clamp(Volume01, 0.0f, 1.0f) * 100.0f);
		ValueText->SetText(FText::AsNumber(Percent));
	}
}
