// Copyright Under the Mountain contributors.
// Purpose: T-083 XP contract — the three GAMEPLAY.md section 8 award
// rules as pure statics, plus the idempotence locks: re-claiming the
// same thief or wave pays nothing, so eat-after-kill reprocessing the
// same corpse cannot double-award. Headless, no world spawned.
// Created 2026-11-03 (T-083).

#include "Misc/AutomationTest.h"
#include "Progression/XPSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-03 T-083] Pure award rules: inside kills +10, lootless
// scares +5, wave gold +1 per 25 (integer division, floor at zero for
// shortfalls and net losses).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXPAwardRulesTest, "UnderTheMountain.XP.Rules",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXPAwardRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("inside kill is worth 10"), UXPSubsystem::AwardForKill(true), 10);
	TestEqual(TEXT("outside kill is worth nothing"), UXPSubsystem::AwardForKill(false), 0);
	TestEqual(TEXT("lootless scare is worth 5"), UXPSubsystem::AwardForScare(true), 5);
	TestEqual(TEXT("looted escape is worth nothing"), UXPSubsystem::AwardForScare(false), 0);
	TestEqual(TEXT("24 gold is worth nothing"), UXPSubsystem::AwardForWaveGold(24), 0);
	TestEqual(TEXT("25 gold is worth 1"), UXPSubsystem::AwardForWaveGold(25), 1);
	TestEqual(TEXT("99 gold is worth 3"), UXPSubsystem::AwardForWaveGold(99), 3);
	TestEqual(TEXT("net loss is worth nothing"), UXPSubsystem::AwardForWaveGold(-500), 0);
	return true;
}

// [2026-11-03 T-083] Idempotence: the first claim for an ID (or wave)
// pays, every repeat pays 0. This is the eat-after-kill lock — the
// audit reprocesses the same corpse on later ticks (and eat touches
// it again), but the second claim is already spent.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXPIdempotenceTest, "UnderTheMountain.XP.Idempotence",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXPIdempotenceTest::RunTest(const FString& Parameters)
{
	UXPSubsystem* XP = NewObject<UXPSubsystem>();
	TestNotNull(TEXT("XP subsystem constructs headless"), XP);
	if (XP == nullptr)
	{
		return false;
	}
	const FName Burglar(TEXT("Burglar_0"));
	TestEqual(TEXT("first inside kill pays 10"), XP->ClaimKill(Burglar, true), 10);
	TestEqual(TEXT("total tracks the kill"), XP->GetTotalXP(), 10);
	// Eat-after-kill: the same corpse reprocessed pays nothing.
	TestEqual(TEXT("reprocessing the corpse pays 0"), XP->ClaimKill(Burglar, true), 0);
	TestEqual(TEXT("total unchanged by the reprocessing"), XP->GetTotalXP(), 10);
	// A second thief is a second award; outside kills never pay.
	TestEqual(TEXT("second inside kill pays again"), XP->ClaimKill(FName(TEXT("Burglar_1")), true), 10);
	TestEqual(TEXT("outside kill pays nothing"), XP->ClaimKill(FName(TEXT("Burglar_2")), false), 0);
	TestEqual(TEXT("total counts only inside kills"), XP->GetTotalXP(), 20);

	const FName Runner(TEXT("Runner_0"));
	TestEqual(TEXT("first lootless scare pays 5"), XP->ClaimScare(Runner, true), 5);
	TestEqual(TEXT("repeat scare pays 0"), XP->ClaimScare(Runner, true), 0);
	TestEqual(TEXT("looted escape pays 0"), XP->ClaimScare(FName(TEXT("Runner_1")), false), 0);
	TestEqual(TEXT("total adds the scare"), XP->GetTotalXP(), 25);

	TestEqual(TEXT("wave gold pays by bracket"), XP->ClaimWaveGold(1, 130), 5);
	TestEqual(TEXT("repeat wave claim pays 0"), XP->ClaimWaveGold(1, 130), 0);
	TestEqual(TEXT("short wave pays 0"), XP->ClaimWaveGold(2, 10), 0);
	TestEqual(TEXT("total adds the wave"), XP->GetTotalXP(), 30);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
