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

// [2026-11-04 T-080] Magic pool contract: the S2 100 pool drains,
// clamps at both ends, refuses overspend, and regens over time at
// the S6 base rate — same headless owner-free pattern as above.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonMagicTest, "UnderTheMountain.Dragon.Magic",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonMagicTest::RunTest(const FString& Parameters)
{
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	TestEqual(TEXT("magic pool is the S2 100"), Stats->MaxMagic, 100.0f);
	TestEqual(TEXT("magic starts full"), Stats->Magic, 100.0f);
	TestEqual(TEXT("base regen is the S6 0.5/s"), Stats->MagicRegenPerSecond, 0.5f);
	TestTrue(TEXT("a full pool has magic"), Stats->HasMagic());

	// Drain: the true-sight verb at S3's 3/s for 10 s costs 30.
	TestTrue(TEXT("draining with magic left reports true"), Stats->DrainMagic(10.0f, 3.0f));
	TestEqual(TEXT("10 s at 3/s drains 30 magic"), Stats->Magic, 70.0f);

	// Discrete spends refuse cleanly instead of going negative.
	TestTrue(TEXT("an affordable spend succeeds"), Stats->TryConsumeMagic(20.0f));
	TestEqual(TEXT("the spend subtracts"), Stats->Magic, 50.0f);
	TestFalse(TEXT("a spend above the pool is refused"), Stats->TryConsumeMagic(51.0f));
	TestEqual(TEXT("a refused spend leaves the pool untouched"), Stats->Magic, 50.0f);
	TestFalse(TEXT("a negative cost is refused"), Stats->TryConsumeMagic(-1.0f));
	TestEqual(TEXT("a refused negative cost leaves the pool untouched"), Stats->Magic, 50.0f);

	// Clamp at empty: the draining tick that empties the pool is
	// the caller's shut-off signal (false), and it stays clamped.
	TestFalse(TEXT("draining past empty reports false (shut-off signal)"),
			  Stats->DrainMagic(100.0f, 3.0f));
	TestEqual(TEXT("magic never goes negative"), Stats->Magic, 0.0f);
	TestFalse(TEXT("an empty pool has no magic"), Stats->HasMagic());
	TestFalse(TEXT("any spend at empty is refused"), Stats->TryConsumeMagic(1.0f));

	// Negative time/rates are free, never regen in disguise.
	Stats->ApplyMagicDelta(10.0f);
	TestTrue(TEXT("a zero-rate drain keeps magic"), Stats->DrainMagic(10.0f, 0.0f));
	TestEqual(TEXT("a zero-rate drain changes nothing"), Stats->Magic, 10.0f);
	Stats->DrainMagic(-5.0f, 3.0f);
	TestEqual(TEXT("negative time changes nothing"), Stats->Magic, 10.0f);

	// Regen over time, clamped at the max.
	Stats->RegenMagic(10.0f);
	TestEqual(TEXT("10 s of regen restores 5 magic"), Stats->Magic, 15.0f);
	Stats->RegenMagic(1000.0f);
	TestEqual(TEXT("regen clamps at max"), Stats->Magic, 100.0f);
	Stats->ApplyMagicDelta(1000.0f);
	TestEqual(TEXT("a huge positive delta clamps at max"), Stats->Magic, 100.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
