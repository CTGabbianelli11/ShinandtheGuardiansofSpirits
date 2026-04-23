#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RHYATOWEROFWISHES_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Attributes")
	float healthMultiplier = 1.0f;

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
	float size = 1;
	
	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float damage = 1;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int32 Currency = 0;

public:
	void RecieveDamage(float _damage);
	UFUNCTION(BlueprintCallable, BlueprintPure)

	float GetHealthPercentage();
	float GetSpeed() { return speed; };
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetSize() const { return size; };
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetDamage() const { return damage; };
	bool IsAlive();
	UFUNCTION(BlueprintCallable)
	void ApplyHealthMultiplier();

	void AddCurrency(int32 AmountOfCurrency);
	bool RemoveCurrency(int32 AmountOfCurrency);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetCurrency() const { return Currency; }
};
