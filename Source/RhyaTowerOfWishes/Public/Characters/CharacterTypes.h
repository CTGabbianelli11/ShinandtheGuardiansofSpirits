#pragma once
UENUM(BlueprintType)
enum class ECharacterState : uint8 {
	ECS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECS_EquippedOneHandedWeapon UMETA(DisplayName = "Equipped One Handed Weapon"),
	ECS_EquippedTwoHandedWeapon UMETA(DisplayName = "Equipped Two Handed Weapon")
};

// None returned by ConsumeAnimNotifyStateInterruptReason means the window completed normally.
UENUM(BlueprintType)
enum class EGameplayNotifyInterruptReason : uint8
{
    None,
    Staggered,
    ActionCancelled,
    Death
};
UENUM(BlueprintType)
enum class EactionState : uint8
{
	EAS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	EAS_Attacking UMETA(DisplayName = "Attacking"),
	EAS_Comboing UMETA(DisplayName = "Comboing"),
	EAS_Dodging UMETA(DisplayName = "Dodging"),
	EAS_Blocking UMETA(DisplayName = "Blocking"),
	EAS_Healing UMETA(DisplayName = "Healing"),
	EAS_Staggered UMETA(DisplayName = "Staggered"),

};
