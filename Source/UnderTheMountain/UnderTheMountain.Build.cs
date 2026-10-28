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

		// [2026-10-28 T-041] No coverage opt-in lives in this file.
		// Scripts/coverage.sh ships coverage by module-only rsp-relink:
		// it replays UBT's own response files for this module with
		// -fprofile-instr-generate -fcoverage-mapping and relinks the
		// module dylib, without UTM_COVERAGE, bCodeCoverage, or any
		// Build.cs flag (on Mac UE 5.8.4 the toolchain never turned
		// bCodeCoverage into clang flags — see coverage.sh's header).
		// The former UTM_COVERAGE=1 -> bCodeCoverage block here was
		// dead (nothing sets that variable) and has been removed.

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
