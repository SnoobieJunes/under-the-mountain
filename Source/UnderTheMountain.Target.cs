// Copyright Under the Mountain contributors. [2026-10-27 T-002]
// Game target: packaged Under the Mountain (macOS, Apple Silicon).

using UnrealBuildTool;
using System.Collections.Generic;

public class UnderTheMountainTarget : TargetRules
{
	public UnderTheMountainTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("UnderTheMountain");
	}
}
