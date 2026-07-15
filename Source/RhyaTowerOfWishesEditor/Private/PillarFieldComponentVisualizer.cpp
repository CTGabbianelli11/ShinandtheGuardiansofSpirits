#include "PillarFieldComponentVisualizer.h"
#include "CanvasTypes.h"
#include "Combat/CombatUtils.h"
#include "Combat/PillarFieldComponent.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "PrimitiveDrawingUtils.h"
#include "ScopedTransaction.h"

IMPLEMENT_HIT_PROXY(HPillarPointProxy, HComponentVisProxy);

#define LOCTEXT_NAMESPACE "PillarFieldComponentVisualizer"

UPillarFieldComponent* FPillarFieldComponentVisualizer::GetEditedPillarField() const
{
	return Cast<UPillarFieldComponent>(ComponentPropertyPath.GetComponent());
}

FProperty* FPillarFieldComponentVisualizer::GetPillarPointsProperty()
{
	return FindFProperty<FProperty>(UPillarFieldComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UPillarFieldComponent, PillarPoints));
}

void FPillarFieldComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UPillarFieldComponent* Field = Cast<const UPillarFieldComponent>(Component);
	if (!Field)
	{
		return;
	}

	UWorld* World = Field->GetWorld();
	if (!World)
	{
		return;
	}

	const UPillarFieldComponent* EditedField = GetEditedPillarField();
	const FTransform LocalToWorld = Field->GetComponentTransform();

	const FLinearColor NoFloorColor = FLinearColor::Red;
	const FLinearColor SelectedColor = FLinearColor::White;
	const FLinearColor LavaColor(1.f, 0.4f, 0.1f);

	for (int32 Index = 0; Index < Field->PillarPoints.Num(); ++Index)
	{
		const FVector WorldPoint = LocalToWorld.TransformPosition(Field->PillarPoints[Index]);

		// Runs every frame while the field is selected; a missing floor is drawn red rather than
		// ensure-spammed (runtime DoPillarAttack still ensures on a real attack).
		FVector FloorPoint = WorldPoint;
		const bool bHitFloor = Rhya::SnapToFloor(*World, WorldPoint, Field->GetOwner(), FloorPoint);

		FLinearColor Color = LavaColor;
		if (!bHitFloor)
		{
			Color = NoFloorColor;
		}
		else if (Field == EditedField && Index == SelectedPointIndex)
		{
			Color = SelectedColor;
		}

		PDI->SetHitProxy(new HPillarPointProxy(Component, Index));
		PDI->DrawPoint(WorldPoint, Color, 14.f, SDPG_Foreground);
		DrawCircle(PDI, FloorPoint + FVector(0.f, 0.f, 1.f), FVector::XAxisVector, FVector::YAxisVector, Color, Field->Radius, 32, SDPG_World, 1.5f);
		PDI->SetHitProxy(nullptr);

		if (!WorldPoint.Equals(FloorPoint))
		{
			PDI->DrawLine(WorldPoint, FloorPoint, Color, SDPG_World, 0.5f);
		}
	}
}

void FPillarFieldComponentVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	const UPillarFieldComponent* Field = Cast<const UPillarFieldComponent>(Component);
	if (!Field)
	{
		return;
	}

	const FTransform LocalToWorld = Field->GetComponentTransform();
	for (int32 Index = 0; Index < Field->PillarPoints.Num(); ++Index)
	{
		const FVector WorldPoint = LocalToWorld.TransformPosition(Field->PillarPoints[Index]);
		FVector2D Pixel;
		// ScreenToPixel returns false behind the camera; skipping avoids mirrored ghost labels.
		if (View->ScreenToPixel(View->WorldToScreen(WorldPoint + FVector(0.f, 0.f, 30.f)), Pixel))
		{
			// ScreenToPixel yields real viewport pixels; the canvas draws in DPI-scaled units.
			Pixel /= Canvas->GetDPIScale();
			Canvas->DrawShadowedString(Pixel.X + 8.0, Pixel.Y - 8.0, *FString::FromInt(Index), GEngine->GetLargeFont(), FLinearColor::White);
		}
	}
}

bool FPillarFieldComponentVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	if (VisProxy && VisProxy->Component.IsValid() && VisProxy->IsA(HPillarPointProxy::StaticGetType()))
	{
		const HPillarPointProxy* PointProxy = static_cast<HPillarPointProxy*>(VisProxy);

		ComponentPropertyPath = FComponentPropertyPath(VisProxy->Component.Get());
		if (ComponentPropertyPath.IsValid())
		{
			SelectedPointIndex = PointProxy->PointIndex;
			return true;
		}
	}

	ComponentPropertyPath.Reset();
	SelectedPointIndex = INDEX_NONE;
	return false;
}

bool FPillarFieldComponentVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
	const UPillarFieldComponent* Field = GetEditedPillarField();
	if (Field && Field->PillarPoints.IsValidIndex(SelectedPointIndex))
	{
		OutLocation = Field->GetComponentTransform().TransformPosition(Field->PillarPoints[SelectedPointIndex]);
		return true;
	}
	return false;
}

bool FPillarFieldComponentVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
	// Re-resolve the component every call: NotifyPropertyModified reruns construction scripts on
	// Blueprint owners, which can replace the instance - a cached pointer would dangle.
	UPillarFieldComponent* Field = GetEditedPillarField();
	if (!Field || !Field->PillarPoints.IsValidIndex(SelectedPointIndex))
	{
		return false;
	}

	if (!DeltaTranslate.IsZero())
	{
		// No FScopedTransaction here: the viewport already opened its "Transform Elements" tracking
		// transaction on mouse-down. A per-delta transaction would fragment undo into one step per
		// mouse-move; TrackingStopped issues the single settling notify.
		Field->Modify();

		const FTransform LocalToWorld = Field->GetComponentTransform();
		FVector& LocalPoint = Field->PillarPoints[SelectedPointIndex];
		LocalPoint = LocalToWorld.InverseTransformPosition(LocalToWorld.TransformPosition(LocalPoint) + DeltaTranslate);

		NotifyPropertyModified(Field, GetPillarPointsProperty(), EPropertyChangeType::Interactive);
		GEditor->RedrawLevelEditingViewports(true);
	}

	return true;
}

void FPillarFieldComponentVisualizer::TrackingStopped(FEditorViewportClient* InViewportClient, bool bInDidMove)
{
	if (bInDidMove)
	{
		UPillarFieldComponent* Field = GetEditedPillarField();
		if (Field && Field->PillarPoints.IsValidIndex(SelectedPointIndex))
		{
			// Final settle so details panels/construction scripts update and one Ctrl+Z undoes the whole drag.
			NotifyPropertyModified(Field, GetPillarPointsProperty(), EPropertyChangeType::ValueSet);
		}
	}
}

bool FPillarFieldComponentVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Key == EKeys::Delete && Event == IE_Pressed)
	{
		UPillarFieldComponent* Field = GetEditedPillarField();
		if (Field && Field->PillarPoints.IsValidIndex(SelectedPointIndex))
		{
			const FScopedTransaction Transaction(LOCTEXT("DeletePillarPoint", "Delete Pillar Point"));
			Field->Modify();
			Field->PillarPoints.RemoveAt(SelectedPointIndex);
			SelectedPointIndex = INDEX_NONE;
			NotifyPropertyModified(Field, GetPillarPointsProperty(), EPropertyChangeType::ArrayRemove);
			GEditor->RedrawLevelEditingViewports(true);
			// Returning true consumes Delete before the editor's delete-actor action fires.
			return true;
		}
	}

	return false;
}

void FPillarFieldComponentVisualizer::EndEditing()
{
	ComponentPropertyPath.Reset();
	SelectedPointIndex = INDEX_NONE;
}

UActorComponent* FPillarFieldComponentVisualizer::GetEditedComponent() const
{
	return ComponentPropertyPath.GetComponent();
}

#undef LOCTEXT_NAMESPACE
