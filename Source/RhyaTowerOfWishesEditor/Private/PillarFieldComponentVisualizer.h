#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class UPillarFieldComponent;

/** Clickable grab handle for one PillarPoints entry; PointIndex is the array index it edits. */
struct HPillarPointProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HPillarPointProxy(const UActorComponent* InComponent, int32 InPointIndex)
		: HComponentVisProxy(InComponent, HPP_Wireframe)
		, PointIndex(InPointIndex)
	{}

	int32 PointIndex;
};

/** Interactive visualizer for UPillarFieldComponent: click-select, drag, and delete authored points. */
class FPillarFieldComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual void TrackingStopped(FEditorViewportClient* InViewportClient, bool bInDidMove) override;
	virtual void EndEditing() override;
	virtual UActorComponent* GetEditedComponent() const override;

private:
	UPillarFieldComponent* GetEditedPillarField() const;
	static FProperty* GetPillarPointsProperty();

	FComponentPropertyPath ComponentPropertyPath;
	int32 SelectedPointIndex = INDEX_NONE;
};
