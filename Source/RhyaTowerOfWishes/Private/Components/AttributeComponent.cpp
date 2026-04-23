#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAttributeComponent::ReceiveDamage(float _damage)
{
	health = FMath::Clamp(health - _damage, 0, maxHealth);
	
	OnHealthPercentUpdateDelegate.Broadcast(GetHealthPercentage());
}

float UAttributeComponent::GetHealthPercentage()
{
	return health / maxHealth;
}

bool UAttributeComponent::IsAlive()
{
	return health > 0;
}

void UAttributeComponent::ApplyHealthMultiplier()
{
	maxHealth *= healthMultiplier;
	health = maxHealth;
}

void UAttributeComponent::AddCurrency(int32 AmountOfCurrency)
{
	Currency += AmountOfCurrency;
}

bool UAttributeComponent::RemoveCurrency(int32 AmountOfCurrency)
{
	if (Currency - AmountOfCurrency < 0)
		return false;

	Currency -= AmountOfCurrency;
	return true;
}
