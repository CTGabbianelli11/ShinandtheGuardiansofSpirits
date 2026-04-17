// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/DeathInterface.h"	
#include "Enemy.generated.h"

class UAnimMontage;
class UAttributeComponent;
class UNiagaraSystem;
class UCrowdExcitementComponent;
class UAC_HitStop;

UCLASS()
class RHYATOWEROFWISHES_API AEnemy : public ACharacter,public IHitInterface,public IDeathInterface
{
	GENERATED_BODY()

public:
	AEnemy();

	UPROPERTY(EditAnywhere)
	UClass* CurrencyToDrop;
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetHit(const FVector& impactPoint, const FVector& impactDirection) override;
	virtual void CharacterDied() override;

	void DirectionalHitReact(const FVector& impactPoint,const FVector impactDirection);
	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)override;
	UFUNCTION(BlueprintCallable)
	void EnableRagdoll();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnCharacterDied();
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnCharacterHit(FVector ImpactPoint,FVector ImpactDirection);

	UFUNCTION(BlueprintCallable)

	void DropCurrency();

	UAttributeComponent* GetAttributes() { return attributes; }
	/**
Play montage functions
*/
	void PlayHitReactMontage(const FName& sectionName);
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere,blueprintReadWrite)
	UAttributeComponent* attributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	float DamageMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	float BaseDamage;

private:
	/*
	*Animation Montages
	*/

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* HitReactMontage;

	/*
	VFX
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Particle System")
	UNiagaraSystem* HitSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Stop System")
	UAC_HitStop* hitStopComponent;

	

	UPROPERTY(EditDefaultsOnly, Category = "Sound Effects")
	USoundWave* hitSound;

public:	

};
