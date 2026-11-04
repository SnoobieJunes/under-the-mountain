// Copyright Under the Mountain contributors.
// Purpose: T-090/T-091 world-life tests — the S2 food ladder
// (wildlife cheapest at zero notoriety, ordering across kinds),
// the wildlife flee radius, and the water decision (wade, fire
// extinguished to steam, never drown), all pure and headless.

#include "Misc/AutomationTest.h"
#include "World/WorldLifeRules.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-04 T-090] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldLifeRulesTest, "UnderTheMountain.World.Life",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldLifeRulesTest::RunTest(const FString& Parameters)
{
	// --- S2 wildlife row verbatim, and zero notoriety is structural ---
	const FFoodValues Wildlife = FWorldLifeRules::WildlifeFood();
	TestEqual(TEXT("wildlife restores 25 stamina"), Wildlife.Stamina, 25.0f);
	TestEqual(TEXT("wildlife restores 5 HP"), Wildlife.HP, 5.0f);
	TestEqual(TEXT("wildlife costs zero notoriety"), Wildlife.NotorietyPoints, 0);
	TestFalse(TEXT("wildlife notoriety is not conditional on witnesses"),
			  Wildlife.bNotorietyConditional);
	TestEqual(TEXT("wildlife kind row matches the named row"),
			  FWorldLifeRules::FoodForKind(EFoodKind::Wildlife).Stamina, Wildlife.Stamina);

	// --- The ladder ordering must hold in both restore and cost ---
	const FFoodValues Livestock = FWorldLifeRules::FoodForKind(EFoodKind::Livestock);
	const FFoodValues PersonIn = FWorldLifeRules::FoodForKind(EFoodKind::PersonInsideLair);
	const FFoodValues PersonOut = FWorldLifeRules::FoodForKind(EFoodKind::PersonOutsideLair);
	TestEqual(TEXT("livestock restores 50 stamina"), Livestock.Stamina, 50.0f);
	TestEqual(TEXT("livestock restores 10 HP"), Livestock.HP, 10.0f);
	TestEqual(TEXT("livestock is worth 3 points when charged"), Livestock.NotorietyPoints, 3);
	TestTrue(TEXT("livestock notoriety is the conditional row"), Livestock.bNotorietyConditional);
	TestEqual(TEXT("person inside restores 60 stamina"), PersonIn.Stamina, 60.0f);
	TestEqual(TEXT("person inside costs zero notoriety"), PersonIn.NotorietyPoints, 0);
	TestEqual(TEXT("person outside restores 75 stamina"), PersonOut.Stamina, 75.0f);
	TestEqual(TEXT("person outside restores 20 HP"), PersonOut.HP, 20.0f);
	TestEqual(TEXT("person outside costs 10 points"), PersonOut.NotorietyPoints, 10);
	TestTrue(TEXT("stamina ordering wildlife < livestock < person outside"),
			 Wildlife.Stamina < Livestock.Stamina && Livestock.Stamina < PersonOut.Stamina);
	TestTrue(TEXT("HP ordering wildlife < livestock < person outside"),
			 Wildlife.HP < Livestock.HP && Livestock.HP < PersonOut.HP);

	// --- Flee radius: graze far, flee near (inclusive boundary) ---
	TestFalse(TEXT("wildlife grazes far from the dragon"), FWorldLifeRules::ShouldWildlifeFlee(5000.0f));
	TestFalse(TEXT("wildlife grazes just outside the flee radius"),
			  FWorldLifeRules::ShouldWildlifeFlee(FWorldLifeRules::WildlifeFleeRadiusCm + 1.0f));
	TestTrue(TEXT("wildlife flees at the flee radius"),
			 FWorldLifeRules::ShouldWildlifeFlee(FWorldLifeRules::WildlifeFleeRadiusCm));
	TestTrue(TEXT("wildlife flees on top of the dragon"), FWorldLifeRules::ShouldWildlifeFlee(0.0f));
	TestTrue(TEXT("a negative distance still flees"), FWorldLifeRules::ShouldWildlifeFlee(-100.0f));

	// --- Water: dry and fly-over are untouched by the rules ---
	const FWaterDecision Dry = FWorldLifeRules::DecideWaterBehaviour(0.0f, false);
	TestTrue(TEXT("no water is dry"), Dry.Behaviour == EWaterBehaviour::Dry);
	TestTrue(TEXT("dry ground allows fire"), Dry.bFireAllowed);
	TestFalse(TEXT("dry ground makes no steam"), Dry.bSteam);
	TestEqual(TEXT("dry ground is full speed"), Dry.MoveSpeedMultiplier, 1.0f);

	const FWaterDecision FlyOver = FWorldLifeRules::DecideWaterBehaviour(500.0f, true);
	TestTrue(TEXT("flying over deep water is fly-over"), FlyOver.Behaviour == EWaterBehaviour::FlyOver);
	TestTrue(TEXT("fly-over allows fire"), FlyOver.bFireAllowed);
	TestFalse(TEXT("fly-over makes no steam"), FlyOver.bSteam);
	TestEqual(TEXT("fly-over is full speed"), FlyOver.MoveSpeedMultiplier, 1.0f);
	TestFalse(TEXT("fly-over cannot drown"), FlyOver.bCanDrown);

	// --- Water: wading puts fire out as steam, deeper is slower ---
	const FWaterDecision Shallow = FWorldLifeRules::DecideWaterBehaviour(50.0f, false);
	TestTrue(TEXT("shallow water wades shallow"), Shallow.Behaviour == EWaterBehaviour::WadeShallow);
	TestFalse(TEXT("wading extinguishes fire breath"), Shallow.bFireAllowed);
	TestTrue(TEXT("extinguished fire vents as steam"), Shallow.bSteam);
	TestEqual(TEXT("shallow wade is 0.8x speed"), Shallow.MoveSpeedMultiplier, 0.8f);
	TestFalse(TEXT("shallow wading cannot drown"), Shallow.bCanDrown);

	const FWaterDecision Deep =
		FWorldLifeRules::DecideWaterBehaviour(FWorldLifeRules::DeepWadeDepthCm, false);
	TestTrue(TEXT("at the deep threshold the wade is deep"), Deep.Behaviour == EWaterBehaviour::WadeDeep);
	TestFalse(TEXT("deep wading also extinguishes fire"), Deep.bFireAllowed);
	TestTrue(TEXT("deep wading also steams"), Deep.bSteam);
	TestEqual(TEXT("deep wade is 0.5x speed"), Deep.MoveSpeedMultiplier, 0.5f);
	TestFalse(TEXT("deep wading still cannot drown (no drown at any depth)"), Deep.bCanDrown);

	const FWaterDecision Abyss = FWorldLifeRules::DecideWaterBehaviour(10000.0f, false);
	TestTrue(TEXT("extreme depth is still just a deep wade"), Abyss.Behaviour == EWaterBehaviour::WadeDeep);
	TestFalse(TEXT("extreme depth still cannot drown"), Abyss.bCanDrown);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
