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

		// [2026-10-27 T-022] Opt-in coverage instrumentation, off unless
		// UTM_COVERAGE=1 (only Scripts/coverage.sh sets it; normal builds are
		// untouched). bCodeCoverage is Epic's per-module coverage switch
		// (UnrealBuildTool ModuleRules.cs:945 in the UE 5.8.4 source tree,
		// "Enable code coverage compilation/linking support"); setting it also
		// forces OptimizeCode.Never for this module (ModuleRules.cs
		// OptimizeCode getter), which keeps line mapping honest.
		// Foot-gun, verified by reading that engine source on 2026-10-27: on
		// Mac this switch alone emits NO clang flags. MacToolChain,
		// AppleToolChain and ClangToolChain contain zero bCodeCoverage
		// handling (only LinuxToolChain.cs:322 and the Windows-clang path in
		// VCToolChain.cs:885 turn it into -fprofile-instr-generate
		// -fcoverage-mapping), and UEBuildModuleCPP's
		// CreateModuleCompileEnvironment overwrites the module value with
		// Target.bCodeCoverage anyway. ModuleRules has no per-module
		// AdditionalCompilerArguments in 5.8.4 (that pair exists only on
		// TargetRules.cs:2914,2921), so Scripts/coverage.sh passes the clang
		// flags through the target-level -CompilerArguments/-LinkerArguments
		// channel that MacToolChain actually consumes (its compile args at
		// MacToolChain.cs:168 and its clang-driver link at MacToolChain.cs:788)
		// and restricts the llvm-cov report to this module's sources.
		// bCodeCoverage stays set here so the intent is recorded module-locally
		// and takes effect as-is on platforms whose toolchain honours it.
		if (System.Environment.GetEnvironmentVariable("UTM_COVERAGE") == "1")
		{
			bCodeCoverage = true;
		}

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
