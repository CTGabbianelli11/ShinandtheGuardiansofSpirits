#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PillarField.generated.h"

class UPillarFieldComponent;

/**
 * Placeable anchor whose root is a UPillarFieldComponent - drop one in a level, author its
 * PillarPoints, and drive it with DoPillarAttack. The PillarField UPROPERTY is load-bearing: the
 * editor visualizer builds its FComponentPropertyPath from this property name (survives Blueprint
 * reconstruction), not from a raw pointer.
 */
UCLASS()
class RHYATOWEROFWISHES_API APillarField : public AActor
{
    GENERATED_BODY()

public:
    APillarField();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pillar Field")
    UPillarFieldComponent* PillarField;
};
