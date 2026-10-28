// Copyright Under the Mountain contributors.
// Purpose: T-003 stamina contract tests — drain, flap cost, regen and clamps
// exercised directly against UDragonStatsComponent with no world spawned,
// so the flight tuning in T-011 changes numbers under a safety net.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonStatsComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-27 T-003] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonStatsTest, "UnderTheMountain.Dragon.Stats",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonStatsTest::RunTest(const FString& Parameters)
{
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->MaxStamina = 100.0f;
	Stats->Stamina = 100.0f;
	Stats->FlyingDrainPerSecond = 10.0f;
	Stats->GroundRegenPerSecond = 20.0f;
	Stats->FlapStaminaCost = 10.0f;

	Stats->DrainForFlight(2.0f);
	TestEqual(TEXT("flight drains stamina per second"), Stats->Stamina, 80.0f);

	TestTrue(TEXT("flap allowed when stamina covers cost"), Stats->CanFlap());
	TestTrue(TEXT("flap consumes stamina"), Stats->TryConsumeStamina(Stats->FlapStaminaCost));
	TestEqual(TEXT("flap cost subtracted"), Stats->Stamina, 70.0f);

	Stats->RegenOnGround(10.0f);
	TestEqual(TEXT("ground regen clamps at max"), Stats->Stamina, 100.0f);

	Stats->ApplyStaminaDelta(-1000.0f);
	TestEqual(TEXT("stamina never goes negative"), Stats->Stamina, 0.0f);
	TestFalse(TEXT("flap refused when exhausted"), Stats->CanFlap());
	TestFalse(TEXT("consume refused when exhausted"), Stats->TryConsumeStamina(1.0f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
