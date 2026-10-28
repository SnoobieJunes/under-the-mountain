// Copyright Under the Mountain contributors. [2026-10-27 T-002]
// Module rules for the single runtime gameplay module. One module on purpose:
// systems split into classes/subsystems inside it (docs/PLAN.md architecture),
// not into premature extra modules that would slow iteration in a hackathon.

using UnrealBuildTool;

public class UnderTheMountain : ModuleRules
{
	public UnderTheMountain(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Niagara",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
