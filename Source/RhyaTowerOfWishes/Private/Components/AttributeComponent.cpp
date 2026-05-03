#include "Components/AttributeComponent.h"
#include "Interfaces/DeathInterface.h"

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
	GEngine->AddOnScreenDebugMessage(0, 2.f, FColor::Red, FString("Hit"));

	health = FMath::Clamp(health - _damage, 0, maxHealth);
	
	OnHealthPercentUpdateDelegate.Broadcast(GetHealthPercentage());

	if (Cast<IDeathInterface>(GetOwner()) && health <= 0)
	{
		Cast<IDeathInterface>(GetOwner())->CharacterDied();
	}
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

void UAttributeComponent::AddMagic(float amount)
{
	magic = FMath::Clamp(magic + amount, 0, maxMagic);
	
}

bool UAttributeComponent::RemoveMagic(float amount)
{
	if (magic - amount < 0)
		return false;

	magic -= amount;
	return true;
}

void UAttributeComponent::ApplyMagicMultiplier()
{
	maxMagic *= magicMultiplier;
	magic = maxMagic;
}

float UAttributeComponent::GetMagicPercentage()
{
	return magic / maxMagic;
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
