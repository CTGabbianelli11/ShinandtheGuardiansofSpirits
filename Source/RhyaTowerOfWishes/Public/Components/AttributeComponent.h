#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FOnHealthPercentUpdateSignature, UAttributeComponent, OnHealthPercentUpdateDelegate, float, Percentage);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

	UPROPERTY(BlueprintAssignable)
	FOnHealthPercentUpdateSignature OnHealthPercentUpdateDelegate;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Attributes")
	float healthMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Attributes")
	float magicMultiplier = 1.0f;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float speed = 1;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int health = 100;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int maxHealth = 100;
	
	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
    int magic = 100;
    
    UPROPERTY(EditAnywhere, Category = "Actor Attributes")
    int maxMagic = 100;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float size = 1;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float damage = 1;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int32 Currency = 0;

public:
	UFUNCTION(BlueprintCallable)
	void ReceiveDamage(float _damage);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetHealthPercentage();
	float GetSpeed() { return speed; };
	bool IsAlive();
	
	UFUNCTION(BlueprintCallable)
	void AddMagic(float amount);
	UFUNCTION(BlueprintCallable)
	bool RemoveMagic(float amount);
	void ApplyMagicMultiplier();
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetMagicPercentage();
	

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetSize() const { return size; };

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetDamage() const { return damage; };

	UFUNCTION(BlueprintCallable)
	void ApplyHealthMultiplier();

	void AddCurrency(int32 AmountOfCurrency);
	bool RemoveCurrency(int32 AmountOfCurrency);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetCurrency() const { return Currency; }
};
