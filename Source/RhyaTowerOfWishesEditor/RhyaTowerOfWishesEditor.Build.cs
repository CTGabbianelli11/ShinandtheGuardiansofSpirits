// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RhyaTowerOfWishesEditor : ModuleRules
{
	public RhyaTowerOfWishesEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Errors on implicit int<->float narrowing (MSVC C4244/C4838); UE's default is Off.
		CppCompileWarningSettings.UnsafeTypeCastWarningLevel = WarningLevel.Error;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "UnrealEd", "RhyaTowerOfWishes" });
	}
}
