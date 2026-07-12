#include "Modules/ModuleManager.h"
#include "ComponentVisualizer.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Combat/PillarFieldComponent.h"
#include "PillarFieldComponentVisualizer.h"

class FRhyaTowerOfWishesEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (!ensureMsgf(GUnrealEd, TEXT("RhyaTowerOfWishesEditor: GUnrealEd is null in StartupModule - module LoadingPhase must be PostEngineInit")))
		{
			return;
		}

		TSharedPtr<FComponentVisualizer> Visualizer = MakeShared<FPillarFieldComponentVisualizer>();
		GUnrealEd->RegisterComponentVisualizer(UPillarFieldComponent::StaticClass()->GetFName(), Visualizer);
		Visualizer->OnRegister();
	}

	virtual void ShutdownModule() override
	{
		if (GUnrealEd)
		{
			GUnrealEd->UnregisterComponentVisualizer(UPillarFieldComponent::StaticClass()->GetFName());
		}
	}
};

IMPLEMENT_MODULE(FRhyaTowerOfWishesEditorModule, RhyaTowerOfWishesEditor)
