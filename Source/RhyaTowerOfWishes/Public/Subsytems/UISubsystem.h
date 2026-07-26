#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UISubsystem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class RHYATOWEROFWISHES_API UUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//Overridable event for Blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Subsystem", meta = (DisplayName = "On Subsystem Initialize"))
	void K2_Initialize();
};
