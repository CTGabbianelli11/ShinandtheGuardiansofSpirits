// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AC_HitStop.generated.h"

class USkeletalMeshComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RHYATOWEROFWISHES_API UAC_HitStop : public UActorComponent
{
	GENERATED_BODY()

public:	

	
	// Sets default values for this component's properties
	UAC_HitStop();

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* skeletalMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitShake")
	FVector MeshRelativeLocation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitShake")
	UCurveFloat* ShakeIntensityCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	UMaterialInterface* materialInterface;
protected:
	ACharacter* CharacterActor;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;



	FTimerDynamicDelegate hitStopTimerEvent;
	FTimerDynamicDelegate MeshShakeStepTimerEvent;
	FTimerHandle timerHandle;
	FTimerHandle hitStopTimerHandle;

	float Duration = 1;
	float ShakeAmplitude = 20;
	float offsetDirection = 1;
	float TimeDialation = 1;
	float StartTimeDialation = 1;
	float ShakeSpeed = 1;

	UMaterialInstanceDynamic* materialInstance;
public:	

	UFUNCTION(BlueprintCallable)
	void SetStartTimeDilation(float startTimeDialation);
	UFUNCTION(BlueprintCallable)
	void BeginHitStop(float duration, float TimeDialation, float ShakeSpeed, float ShakeAmplitude,bool applyMaterial);
	UFUNCTION(BlueprintCallable)
	void EndHitStop();
	UFUNCTION(BlueprintCallable)
	void ApplyMeshShakeStep();
	float GetFloatFromCurve();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
