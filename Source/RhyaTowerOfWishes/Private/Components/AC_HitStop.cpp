// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AC_HitStop.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UAC_HitStop::UAC_HitStop()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	CharacterActor = Cast<ACharacter>(GetOwner());

	if (CharacterActor)
		skeletalMesh = CharacterActor->GetMesh();

	if (skeletalMesh)
		MeshRelativeLocation = skeletalMesh->GetRelativeLocation();

	hitStopTimerEvent.BindUFunction(this, FName("EndHitStop"));
}


// Called when the game starts
void UAC_HitStop::BeginPlay()
{
	Super::BeginPlay();

	if (skeletalMesh)
		MeshRelativeLocation = skeletalMesh->GetRelativeLocation();
	// ...	
}

void UAC_HitStop::SetStartTimeDilation(float startTimeDialation)
{
	StartTimeDialation = startTimeDialation;
}

void UAC_HitStop::BeginHitStop(float duration,float timeDialation,float shakeSpeed,float shakeAmplitude, bool applyMaterial)
{	

	Duration = duration;
	TimeDialation = timeDialation;
	ShakeSpeed = 1.f / shakeSpeed;
	ShakeAmplitude = shakeAmplitude;
	
	//slow down speed up or stop character
	if (CharacterActor)
		CharacterActor->CustomTimeDilation = TimeDialation;

	MeshShakeStepTimerEvent.BindUFunction(this, FName("ApplyMeshShakeStep"));

	hitStopTimerHandle =UKismetSystemLibrary::K2_SetTimerDelegate(hitStopTimerEvent, duration,false,false);
	timerHandle = UKismetSystemLibrary::K2_SetTimerDelegate(MeshShakeStepTimerEvent, ShakeSpeed,true,false);

	//Set overlay material and give flash
	UWorld* world = GetWorld();
	if (world && applyMaterial)
	{
		materialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(world, materialInterface);
		if(materialInstance!=NULL)
		materialInstance->SetScalarParameterValue(TEXT("Speed"), 5.f);

		CharacterActor->GetMesh()->SetOverlayMaterial(materialInstance);
	}
}

void UAC_HitStop::EndHitStop()
{
	if (CharacterActor)
	{
		GetWorld()->GetTimerManager().ClearTimer(timerHandle);

		FHitResult SweepHitResult;
		skeletalMesh->K2_SetRelativeLocation(MeshRelativeLocation, false, SweepHitResult, true);

		CharacterActor->CustomTimeDilation = StartTimeDialation;

		CharacterActor->GetMesh()->SetOverlayMaterial(NULL);
	}
}

void UAC_HitStop::ApplyMeshShakeStep()
{
	UWorld* world = GetWorld();
	if (world)
	{
		FVector inverseTransformDirection;
		APlayerController* playerController = UGameplayStatics::GetPlayerController(world,0);
		if (playerController)
		{
			//align to camera
			inverseTransformDirection.X = UKismetMathLibrary::InverseTransformDirection(CharacterActor->GetActorTransform(), playerController->PlayerCameraManager->GetActorRightVector()).X;
			inverseTransformDirection.Y = UKismetMathLibrary::InverseTransformDirection(CharacterActor->GetActorTransform(), playerController->PlayerCameraManager->GetActorRightVector()).Y;

			FHitResult SweepHitResult;

			//Set shake intensity progress/duration * curve
			skeletalMesh->K2_SetRelativeLocation(MeshRelativeLocation + inverseTransformDirection * (ShakeAmplitude * offsetDirection)* GetFloatFromCurve(), false, SweepHitResult, true);
		}

	}



	offsetDirection *= -1;
}

float UAC_HitStop::GetFloatFromCurve()
{
	UWorld* world = GetWorld();
	if (!world)
		return 1.f;

	if (ShakeIntensityCurve)
	{
		float percentageOfTimePassed = UKismetSystemLibrary::K2_GetTimerElapsedTimeHandle(world, hitStopTimerHandle) / Duration;
		float shakeFromCurve = ShakeIntensityCurve->GetFloatValue(percentageOfTimePassed);

		return shakeFromCurve;
	}
	return 1.0f;
}


// Called every frame
void UAC_HitStop::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

