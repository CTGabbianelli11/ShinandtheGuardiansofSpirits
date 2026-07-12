#include "Combat/PillarField.h"
#include "Combat/PillarFieldComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

APillarField::APillarField()
{
    PrimaryActorTick.bCanEverTick = false;

    PillarField = CreateDefaultSubobject<UPillarFieldComponent>(TEXT("PillarField"));
    RootComponent = PillarField;

#if WITH_EDITORONLY_DATA
    UBillboardComponent* Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
    if (Sprite && !IsRunningCommandlet())
    {
        struct FConstructorStatics
        {
            ConstructorHelpers::FObjectFinderOptional<UTexture2D> TargetIcon;
            FName ID_PillarField;
            FText NAME_PillarField;
            FConstructorStatics()
                : TargetIcon(TEXT("/Engine/EditorResources/S_Emitter"))
                , ID_PillarField(TEXT("PillarField"))
                , NAME_PillarField(NSLOCTEXT("SpriteCategory", "PillarField", "Pillar Fields"))
            {
            }
        };
        static FConstructorStatics ConstructorStatics;

        Sprite->Sprite = ConstructorStatics.TargetIcon.Get();
        Sprite->SpriteInfo.Category = ConstructorStatics.ID_PillarField;
        Sprite->SpriteInfo.DisplayName = ConstructorStatics.NAME_PillarField;
        Sprite->bIsScreenSizeScaled = true;
        Sprite->SetupAttachment(RootComponent);
    }
#endif
}
