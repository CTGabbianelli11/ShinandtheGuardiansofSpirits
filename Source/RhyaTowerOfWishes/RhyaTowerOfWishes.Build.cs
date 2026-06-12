// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RhyaTowerOfWishes : ModuleRules
{
	public RhyaTowerOfWishes(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Errors on implicit int<->float narrowing (MSVC C4244/C4838); UE's default is Off.
		CppCompileWarningSettings.UnsafeTypeCastWarningLevel = WarningLevel.Error;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "AudioModulation" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
