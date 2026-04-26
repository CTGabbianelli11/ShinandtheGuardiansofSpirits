// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatData.generated.h"

/**
 * 
 */


UENUM(BlueprintType)
enum EAttackHitDirection : uint8
{
	Fwd UMETA(DisplayName = "Forward"),
	Rt UMETA(DisplayName = "Right"),
	Back UMETA(DisplayName = "Back"),
	Lt UMETA(DisplayName = "Left")
};

UENUM(BlueprintType)
enum EAIStates : uint8
{
	GetInRange,
	GetBack,
	Wait,
	Look,
	Strafe,
	WasHit,
	Attack
};

//USTRUCT(BlueprintType)
//struct FPlayerAttackAnimData
//	{
//		GENERATED_BODY() 
//
//		UPROPERTY(EditAnywhere, BlueprintReadWrite)
//		UAnimMontage* AttackMontage;
//
//		UPROPERTY(EditAnywhere, BlueprintReadWrite)
//		float PlayRate;
//
//		UPROPERTY(EditAnywhere, BlueprintReadWrite)
//		float Offset;
//
//		FPlayerAttackAnimData()
//		{
//			PlayRate = 1.f;
//			Offset = 100.f;
//		}
//	};
	/**
	 *
	 */
	 //UCLASS()
	 //class CRAZYARENACHAOS_API UFFC_Data : public UObject
	 //{
	 //	GENERATED_BODY()
	 //	
	 //};

