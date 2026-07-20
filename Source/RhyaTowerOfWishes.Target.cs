// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RhyaTowerOfWishesTarget : TargetRules
{
	public RhyaTowerOfWishesTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("RhyaTowerOfWishes");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// MSVC only emits C4305 (literal double->float truncation) under /fp:precise,
			// so this is inert in modules that keep the /fp:fast default; the game modules
			// opt into precise FP in their Build.cs, which scopes the error to them and
			// matches the clang narrowing errors Mac builds already get.
			bOverrideBuildEnvironment = true;
			AdditionalCompilerArguments = "/we4305";
		}
	}
}
