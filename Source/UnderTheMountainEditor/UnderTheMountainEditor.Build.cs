// Copyright Under the Mountain contributors. [2026-11-01 T-049]
// Editor module rules: editor-only tooling (the terrain commandlet).
// Kept out of the runtime module on purpose: UHT generates a UCLASS's
// reflection code for every target, so an editor-only UCLASS inside
// the game module breaks the game build (proven in T-049) - an
// editor module is the sanctioned home and matches the layout
// AGENTS.md already describes.

using UnrealBuildTool;

public class UnderTheMountainEditor : ModuleRules
{
	public UnderTheMountainEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"Landscape",
			// [2026-11-04 T-139] ABPAuthor commandlet: AnimGraph node
			// classes + AnimBlueprint factory/compile/save plumbing,
			// K2 variable/enum nodes for transition rules, and the
			// runtime module for UDragonAnimInstance / EDragonAnimState.
			"AnimGraph",
			"AnimGraphRuntime",
			"AnimationBlueprintEditor",
			"AssetRegistry",
			"BlueprintGraph",
			"KismetCompiler",
			"UnderTheMountain"
		});
	}
}
