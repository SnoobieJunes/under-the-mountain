// Copyright Under the Mountain contributors.
// Purpose: T-011 flight-economy contract tests — landing-damage curve, HP
// clamps, exhaustion drain and the new flap cost, exercised directly
// against UDragonStatsComponent with no world spawned (GAMEPLAY.md s2).
// Created 2026-10-28 (T-011). The pawn-side wiring (Landed impact capture,
// per-tick glide/hover/dive selection) is PIE/PT-STAM territory; the math
// it delegates to is proven here.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonStatsComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-28 T-011] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonFlightTest, "UnderTheMountain.Dragon.Flight",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonFlightTest::RunTest(const FString& Parameters)
{
	// [2026-10-28 T-011] Defaults first, on a fresh component, before any
	// test-local tuning overwrites them: these lock the GAMEPLAY.md s2
	// numbers (flap 3/beat, glide 1/s, hover 8/s, exhaustion 2 HP/s) and
	// the landing tuning picked in the header (800..2200 cm/s -> 5..25).
	UDragonStatsComponent* Defaults = NewObject<UDragonStatsComponent>();
	TestEqual(TEXT("flap cost default is 3 per beat"), Defaults->FlapStaminaCost, 3.0f);
	TestEqual(TEXT("glide drain default is 1/s"), Defaults->GlideDrainPerSecond, 1.0f);
	TestEqual(TEXT("hover drain default is 8/s"), Defaults->HoverDrainPerSecond, 8.0f);
	TestEqual(TEXT("exhaustion HP drain default is 2/s"), Defaults->ExhaustionHPDrainPerSecond, 2.0f);
	TestEqual(TEXT("landing threshold default is 800 cm/s"), Defaults->LandingSpeedThreshold, 800.0f);
	TestEqual(TEXT("landing max-damage speed default is 2200 cm/s"), Defaults->LandingSpeedMaxDamage, 2200.0f);
	TestEqual(TEXT("HP starts at max"), Defaults->HP, Defaults->MaxHP);

	// [2026-10-28 T-011] Flap math at the new cost: exactly one beat's
	// worth of stamina buys exactly one flap and no more.
	Defaults->Stamina = Defaults->FlapStaminaCost;
	TestTrue(TEXT("flap allowed at exactly the cost"), Defaults->CanFlap());
	TestTrue(TEXT("flap consumes at exactly the cost"),
			 Defaults->TryConsumeStamina(Defaults->FlapStaminaCost));
	TestEqual(TEXT("flap cost subtracted to empty"), Defaults->Stamina, 0.0f);
	TestFalse(TEXT("no second flap when empty"), Defaults->CanFlap());

	// [2026-10-28 T-011] Landing curve against explicitly set tuning, so
	// the shape (not today's numbers) is what this block locks.
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->LandingSpeedThreshold = 800.0f;
	Stats->LandingSpeedMaxDamage = 2200.0f;
	Stats->MinLandingDamage = 5.0f;
	Stats->MaxLandingDamage = 25.0f;

	TestEqual(TEXT("no landing damage at zero speed"), Stats->LandingDamageForImpactSpeed(0.0f), 0.0f);
	TestEqual(TEXT("no landing damage below threshold"), Stats->LandingDamageForImpactSpeed(799.0f), 0.0f);
	TestEqual(TEXT("no landing damage at threshold"), Stats->LandingDamageForImpactSpeed(800.0f), 0.0f);
	TestTrue(TEXT("minimum damage just above threshold"),
			 Stats->LandingDamageForImpactSpeed(801.0f) >= 5.0f);
	TestEqual(TEXT("max landing damage at max-damage speed"),
			  Stats->LandingDamageForImpactSpeed(2200.0f), 25.0f);
	TestEqual(TEXT("landing damage clamped above max-damage speed"),
			  Stats->LandingDamageForImpactSpeed(5000.0f), 25.0f);

	// [2026-10-28 T-011] Monotone between threshold and cap: faster must
	// never hurt less, and a midpoint impact lands strictly inside 5..25.
	float PreviousDamage = Stats->LandingDamageForImpactSpeed(800.0f);
	for (float Speed = 900.0f; Speed <= 2200.0f; Speed += 100.0f)
	{
		const float Damage = Stats->LandingDamageForImpactSpeed(Speed);
		TestTrue(FString::Printf(TEXT("landing damage monotone at %.0f cm/s"), Speed),
				 Damage >= PreviousDamage);
		PreviousDamage = Damage;
	}
	const float MidDamage = Stats->LandingDamageForImpactSpeed(1500.0f);
	TestTrue(TEXT("mid-range landing damage strictly inside 5..25"), MidDamage > 5.0f && MidDamage < 25.0f);

	// [2026-10-28 T-011] HP pool: damage subtracts, clamps at 0, and a
	// negative "damage" can never heal through the damage verb.
	Stats->MaxHP = 100.0f;
	Stats->HP = 100.0f;
	Stats->ApplyHPDamage(30.0f);
	TestEqual(TEXT("HP damage subtracted"), Stats->HP, 70.0f);
	Stats->ApplyHPDamage(-50.0f);
	TestEqual(TEXT("negative damage does not heal"), Stats->HP, 70.0f);
	Stats->ApplyHPDamage(1000.0f);
	TestEqual(TEXT("HP clamps at 0"), Stats->HP, 0.0f);

	// [2026-10-28 T-011] Exhaustion: 2 HP/s off an explicitly set rate,
	// and clamped by the same HP floor.
	Stats->HP = 100.0f;
	Stats->ExhaustionHPDrainPerSecond = 2.0f;
	Stats->DrainHPForExhaustion(2.5f);
	TestEqual(TEXT("exhaustion drains 2 HP per second"), Stats->HP, 95.0f);
	Stats->DrainHPForExhaustion(1000.0f);
	TestEqual(TEXT("exhaustion drain clamps HP at 0"), Stats->HP, 0.0f);
	Stats->HP = 50.0f;
	Stats->DrainHPForExhaustion(-5.0f);
	TestEqual(TEXT("negative exhaustion time drains nothing"), Stats->HP, 50.0f);

	// [2026-10-28 T-011] Explicit-rate flight drain: glide rate for 10 s
	// costs 10, a zero rate (dive) is free.
	Stats->MaxStamina = 100.0f;
	Stats->Stamina = 100.0f;
	Stats->DrainForFlight(10.0f, Stats->GlideDrainPerSecond);
	TestEqual(TEXT("glide-rate drain over 10 s"), Stats->Stamina, 90.0f);
	Stats->DrainForFlight(10.0f, 0.0f);
	TestEqual(TEXT("zero-rate (dive) drain is free"), Stats->Stamina, 90.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
