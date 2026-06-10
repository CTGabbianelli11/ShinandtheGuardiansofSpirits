#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/RhyaGameUserSettings.h" // EVolumeCategory
#include "RhyaVolumeSlider.generated.h"

class USlider;
class UTextBlock;

/**
 * Reusable, self-contained volume slider row.
 *
 * One widget drives any audio category: set Category per instance (Master/Music/SFX)
 * and the slider reads/writes that category through URhyaGameUserSettings. The view is
 * a Blueprint child (WBP_VolumeSlider) that supplies an AnalogSlider named "VolumeSlider"
 * and, optionally, a "Label" text block (auto-set to the category name) and a "ValueText"
 * block (auto-set to the live 0-100 value). USlider/UTextBlock are the bind types so any
 * subclass (e.g. the CommonUI AnalogSlider / CommonTextBlock) satisfies them without pulling
 * CommonUI into this module.
 *
 * SetVolume applies live; volumes are persisted on NativeDestruct (menu close).
 */
UCLASS()
class RHYATOWEROFWISHES_API URhyaVolumeSlider : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which audio category this slider controls. Set per instance in the menu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	EVolumeCategory Category = EVolumeCategory::Master;

protected:
	virtual void NativeConstruct() override;

	/** Persist volumes when the owning menu closes (the sliders destruct then). */
	virtual void NativeDestruct() override;

	/** The slider this widget drives. Bound from WBP_VolumeSlider (an AnalogSlider). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> VolumeSlider;

	/** Category name label (e.g. "Master"). Optional; auto-set from Category when present. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Label;

	/** Live value readout (0-100). Optional; auto-updated on construct + slider change. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

private:
	/** Push slider movement to the matching category's bus (live, unsaved). */
	UFUNCTION()
	void HandleValueChanged(float NewValue);

	/** Display name for the current Category (e.g. "Master"/"Music"/"SFX"). */
	FText GetCategoryDisplayName() const;

	/** Refresh the value readout from a 0..1 volume (shown as a 0-100 integer). */
	void UpdateValueText(float Volume01) const;
};
