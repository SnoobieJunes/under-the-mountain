// Copyright Under the Mountain contributors. [2026-10-27 T-002]
// Editor target: Unreal Editor with the UnderTheMountain module loaded.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnderTheMountainEditorTarget : TargetRules
{
	public UnderTheMountainEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("UnderTheMountain");
	}
}
