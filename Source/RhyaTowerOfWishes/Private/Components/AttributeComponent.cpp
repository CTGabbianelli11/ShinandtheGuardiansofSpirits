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
	const bool bWasAlive = IsAlive();

	// Truncation is the established behavior; attributes are ints, damage is float.
	// bNeverDie floors at 1 so training dummies keep taking hits but never reach the death path.
	const int32 MinHealth = bNeverDie ? 1 : 0;
	health = FMath::Clamp((int32)(health - _damage), MinHealth, maxHealth);

	OnHealthPercentUpdateDelegate.Broadcast(GetHealthPercentage());

	// Only on the alive->dead transition — corpse hits used to re-fire CharacterDied
	// (re-ragdoll, repeat death events) on every subsequent damage event.
	if (bWasAlive && !IsAlive() && Cast<IDeathInterface>(GetOwner()))
	{
		Cast<IDeathInterface>(GetOwner())->CharacterDied();
	}
}

void UAttributeComponent::AddHealth(float amount)
{
	health = FMath::Clamp((int32)(health + amount), 0, maxHealth);

	OnHealthPercentUpdateDelegate.Broadcast(GetHealthPercentage());
}

float UAttributeComponent::GetHealthPercentage()
{
	return maxHealth > 0 ? (float)health / (float)maxHealth : 0.f;
}

bool UAttributeComponent::IsAlive()
{
	return health > 0;
}

void UAttributeComponent::ApplyHealthMultiplier()
{
	maxHealth = (int32)(maxHealth * healthMultiplier);
	health = maxHealth;
}

void UAttributeComponent::AddMagic(float amount)
{
	magic = FMath::Clamp((int32)(magic + amount), 0, maxMagic);

	OnMagicPercentUpdateDelegate.Broadcast(GetMagicPercentage());
}

bool UAttributeComponent::RemoveMagic(float amount)
{
	if (magic - amount < 0)
		return false;

	magic = (int32)(magic - amount);

	OnMagicPercentUpdateDelegate.Broadcast(GetMagicPercentage());
	return true;
}

void UAttributeComponent::ApplyMagicMultiplier()
{
	maxMagic = (int32)(maxMagic * magicMultiplier);
	magic = maxMagic;
}

float UAttributeComponent::GetMagicPercentage()
{
	// magic/maxMagic are ints — cast to avoid integer division.
	return maxMagic > 0 ? (float)magic / (float)maxMagic : 0.f;
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
