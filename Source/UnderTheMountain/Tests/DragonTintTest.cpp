// Copyright Under the Mountain contributors.
// Purpose: T-027 scale-tint contract tests — clamping, the material
// parameter name, and the save/load round-trip, exercised directly
// against UDragonAppearanceComponent with no world spawned (the
// component is documented to store the tint with no owner/mesh).

#include "Misc/AutomationTest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dragon/DragonAppearanceComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkeletalMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-28 T-027] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonTintTest, "UnderTheMountain.Dragon.Tint",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonTintTest::RunTest(const FString& Parameters)
{
	UDragonAppearanceComponent* Appearance = NewObject<UDragonAppearanceComponent>();

	// [2026-10-28 T-039] Production default vs test slot: a normally-
	// constructed component uses the player's 'DragonScaleTint' slot,
	// while a testing component reports the separate test slot. Every
	// component in this suite that saves/loads is pointed at the test
	// slot first, so the suite NEVER writes the player's slot (the
	// old test overwrote it and 'restored' white, destroying any
	// real saved tint; an aborted run left its probe colour behind).
	TestEqual(TEXT("production default slot is DragonScaleTint"), Appearance->GetSaveSlotName(),
			  FString(TEXT("DragonScaleTint")));
	TestEqual(TEXT("production default matches UDragonTintSaveGame::SaveSlotName"),
			  Appearance->GetSaveSlotName(), UDragonTintSaveGame::SaveSlotName);
	Appearance->SetSaveSlotForTesting(UDragonAppearanceComponent::TestSaveSlotName());
	TestEqual(TEXT("testing component reports the test slot"), Appearance->GetSaveSlotName(),
			  UDragonAppearanceComponent::TestSaveSlotName());
	TestEqual(TEXT("test slot is DragonScaleTintTest"), UDragonAppearanceComponent::TestSaveSlotName(),
			  FString(TEXT("DragonScaleTintTest")));
	TestTrue(TEXT("test slot differs from production slot"),
			 UDragonAppearanceComponent::TestSaveSlotName() != UDragonTintSaveGame::SaveSlotName);

	// [2026-10-28 T-027] Out-of-range channels clamp to [0, 1] and
	// alpha is forced to 1. bSave=false keeps this probe out of the
	// player's slot.
	Appearance->SetScaleTint(FLinearColor(2.0f, -1.0f, 0.5f, 0.0f), false);
	const FLinearColor Clamped = Appearance->GetScaleTint();
	TestEqual(TEXT("red clamps to 1"), Clamped.R, 1.0f);
	TestEqual(TEXT("green clamps to 0"), Clamped.G, 0.0f);
	TestEqual(TEXT("blue passes through"), Clamped.B, 0.5f);
	TestEqual(TEXT("alpha forced to 1"), Clamped.A, 1.0f);
	TestTrue(TEXT("clamped tint equals (1, 0, 0.5, 1)"),
			 Clamped == FLinearColor(1.0f, 0.0f, 0.5f, 1.0f));

	// [2026-10-28 T-027] The C++ side of the M_DragonScales contract:
	// the VectorParameter the material exposes is named exactly this.
	TestTrue(TEXT("material parameter is ScaleTint"),
			 UDragonAppearanceComponent::TintParameterName() == FName(TEXT("ScaleTint")));

	// [2026-10-28 T-039] Save/load round-trip through the TEST slot
	// (writes Saved/SaveGames/DragonScaleTintTest.sav, which AGENTS.md
	// permits tests to do inside the project's Saved/ folder) — never
	// the player's 'DragonScaleTint' slot. A second, fresh component
	// (also pointed at the test slot) must read back what the first saved.
	const FLinearColor SavedTint(0.25f, 0.5f, 0.75f, 1.0f);
	Appearance->SetScaleTint(SavedTint, true);

	UDragonAppearanceComponent* Reloaded = NewObject<UDragonAppearanceComponent>();
	Reloaded->SetSaveSlotForTesting(UDragonAppearanceComponent::TestSaveSlotName());
	Reloaded->LoadTint();
	const FLinearColor LoadedTint = Reloaded->GetScaleTint();
	TestEqual(TEXT("loaded red matches saved"), LoadedTint.R, SavedTint.R);
	TestEqual(TEXT("loaded green matches saved"), LoadedTint.G, SavedTint.G);
	TestEqual(TEXT("loaded blue matches saved"), LoadedTint.B, SavedTint.B);
	TestEqual(TEXT("loaded alpha is 1"), LoadedTint.A, 1.0f);

	// [2026-10-28 T-027] The actual parameter write: a mesh carrying
	// M_DragonScales must report back the tint through its dynamic
	// material instance. This is the seam the picker lives on.
	UMaterialInterface* ScalesMaterial =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Meshy/Dragon/M_DragonScales.M_DragonScales"));
	TestNotNull(TEXT("M_DragonScales loads"), ScalesMaterial);
	if (ScalesMaterial != nullptr)
	{
		// [2026-10-28 T-027] The real dragon mesh supplies slot 0;
		// a bare component has no slots to write to.
		USkeletalMesh* DragonMesh = LoadObject<USkeletalMesh>(
			nullptr, TEXT("/Game/Meshy/Dragon/SK_Dragon_v4.SK_Dragon_v4"));
		TestNotNull(TEXT("SK_Dragon_v4 loads"), DragonMesh);
		USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>();
		Mesh->SetSkeletalMeshAsset(DragonMesh);
		Mesh->SetMaterial(0, ScalesMaterial);
		Appearance->SetScaleTint(FLinearColor(0.2f, 0.4f, 0.6f, 1.0f), false);
		Appearance->ApplyTintToMesh(Mesh);
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		TestNotNull(TEXT("apply creates a dynamic instance"), MID);
		if (MID != nullptr)
		{
			FLinearColor Written;
			TestTrue(TEXT("ScaleTint parameter is set on the MID"),
					 MID->GetVectorParameterValue(FName(TEXT("ScaleTint")), Written));
			TestTrue(TEXT("MID tint equals component tint"), Written == Appearance->GetScaleTint());
		}
	}

	// [2026-10-28 T-039] Reset only the TEST slot to canonical white;
	// the player's real slot was never touched by this suite, so there
	// is nothing to 'restore' there (the old reset-to-white wrote the
	// real slot and destroyed any actual saved tint).
	Appearance->SetScaleTint(FLinearColor::White, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
