// [2026-11-04 T-081] Artifact catalogue + registry-truth effect tests.
#include "Misc/AutomationTest.h"
#include "Artifacts/ArtifactDefs.h"
#include "Treasure/TreasureRegistry.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtifactCatalogTest, "UnderTheMountain.Artifacts.Catalog",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArtifactCatalogTest::RunTest(const FString& P)
{
	TestEqual(TEXT("all S6 index rows present"), FArtifactCatalog::All().Num(), 11);
	TestEqual(TEXT("S6 starting artifact gold sums to 6700"), FArtifactCatalog::TotalStartingGold(), 6700);
	for (const FArtifactDefinition& D : FArtifactCatalog::All())
	{
		TestTrue(*FString::Printf(TEXT("%s has slot+value"), *D.DisplayName), !D.Slot.IsNone() && D.GoldValue > 0);
		if (D.Kind == EArtifactEffectKind::DataOnlyNamed)
			TestTrue(*FString::Printf(TEXT("data-only %s is named in EffectSummary"), *D.DisplayName), !D.EffectSummary.IsEmpty());
	}
	TestNotNull(TEXT("seeing stone found"), FArtifactCatalog::Find(TEXT("SeeingStone")));
	TestNull(TEXT("unknown artifact fails closed"), FArtifactCatalog::Find(TEXT("Nope")));
	// Ring cap + slot exclusivity
	TestFalse(TEXT("crown blocks helm (Head)"), FArtifactRules::CanWearTogether({TEXT("MindControlCrown")}, TEXT("TreasureSightHelm")));
	TestTrue(TEXT("scales+armor stack (Body)"), FArtifactRules::CanWearTogether({TEXT("HardenedScales")}, TEXT("StrengthArmor")));
	TestFalse(TEXT("third ring refused"), FArtifactRules::CanWearTogether({TEXT("RingOfPower"), TEXT("FlameRing")}, TEXT("ElementalRings")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtifactEffectTest, "UnderTheMountain.Artifacts.Effects",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArtifactEffectTest::RunTest(const FString& P)
{
	UTreasureRegistry* R = NewObject<UTreasureRegistry>();
	FTreasureEntry Stone;
	Stone.Id = TEXT("SeeingStone");
	Stone.GoldValue = 1200;
	Stone.bIsSeeingStone = true;
	R->Register(Stone);
	TestTrue(TEXT("stone active in hoard"), FArtifactRules::IsActiveFor(R, TEXT("SeeingStone")));
	TestTrue(TEXT("true-sight allowed"), FArtifactRules::CanUseTrueSight(R->Find(TEXT("SeeingStone"))));
	R->SetCarried(TEXT("SeeingStone"), true, TEXT("Thief1"));
	TestFalse(TEXT("off the tick carried out"), FArtifactRules::IsActiveFor(R, TEXT("SeeingStone")));
	TestFalse(TEXT("true-sight refused while carried"), FArtifactRules::CanUseTrueSight(R->Find(TEXT("SeeingStone"))));
	R->SetCarried(TEXT("SeeingStone"), false, NAME_None);
	TestTrue(TEXT("recovered on drop"), FArtifactRules::IsActiveFor(R, TEXT("SeeingStone")));
	TestFalse(TEXT("missing artifact inactive"), FArtifactRules::IsActiveFor(R, TEXT("FlameRing")));
	// Hardened scales
	TestEqual(TEXT("scales -30%"), FArtifactRules::DamageAfterHardenedScales(100.f, true, false), 70.f);
	TestEqual(TEXT("pit capped at 50% reduction"), FArtifactRules::DamageAfterHardenedScales(100.f, true, true), 50.f);
	TestEqual(TEXT("inactive scales no reduction"), FArtifactRules::DamageAfterHardenedScales(100.f, false, true), 100.f);
	// Flame ring / ring of power / claws
	TestEqual(TEXT("flame burn on"), FArtifactRules::FlameRingBurnDps(true), 6.f);
	TestEqual(TEXT("flame burn off"), FArtifactRules::FlameRingBurnDps(false), 0.f);
	TestEqual(TEXT("ring fire +25%"), FArtifactRules::FireDamageWithRingOfPower(100.f, true), 125.f);
	TestEqual(TEXT("ring magic +25"), FArtifactRules::MaxMagicWithRingOfPower(100.f, true), 125.f);
	TestEqual(TEXT("claws 25->40"), FArtifactRules::ClawDamageWithEnhancedClaws(25.f, true), 40.f);
	TestEqual(TEXT("claws off stays 25"), FArtifactRules::ClawDamageWithEnhancedClaws(25.f, false), 25.f);
	return true;
}
#endif
