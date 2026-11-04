// Copyright Under the Mountain contributors.
// Purpose: T-131 session-cadence contract — the pure incursion
// scheduler's spec ranges (90-150 s gaps, 60-120 s incursion
// intervals, 1-2 incursions per gap, max 2 concurrent, 20 s wave
// warning, 4-minute wave cap) and the stateful gap/wave clocks
// the wave subsystem ticks, plus the wave subsystem's sandbox
// freeze and T-131 ForceFlee on the thief brain. All headless:
// rolls are injected units and clocks are fed exact deltas, so
// boundaries are proven, not sampled.
// Created 2026-11-04 (T-131).

#include "Misc/AutomationTest.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"
#include "Waves/IncursionScheduler.h"
#include "Waves/WaveSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-04 T-131] Cadence rules, verbatim from GAMEPLAY.md
// section 1: every mapping clamps into its spec range, the spawn
// gate honours plan/concurrency/sandbox, and the warning/cap
// boundaries are inclusive exactly where the spec says.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIncursionCadenceTest, "UnderTheMountain.Waves.IncursionCadence",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIncursionCadenceTest::RunTest(const FString& Parameters)
{
	// --- Gap mapping: 90 at roll 0, 150 at roll 1, clamped wild ---
	TestEqual(TEXT("gap roll 0 is 90s"), FIncursionScheduler::GapSecondsForUnit(0.0f), 90.0f);
	TestEqual(TEXT("gap roll 1 is 150s"), FIncursionScheduler::GapSecondsForUnit(1.0f), 150.0f);
	TestEqual(TEXT("gap roll half is 120s"), FIncursionScheduler::GapSecondsForUnit(0.5f), 120.0f);
	TestEqual(TEXT("gap roll under clamps to 90s"), FIncursionScheduler::GapSecondsForUnit(-3.0f),
			  90.0f);
	TestEqual(TEXT("gap roll over clamps to 150s"), FIncursionScheduler::GapSecondsForUnit(7.0f),
			  150.0f);

	// --- Incursion interval mapping: 60-120 s ---
	TestEqual(TEXT("interval roll 0 is 60s"),
			  FIncursionScheduler::IncursionIntervalSecondsForUnit(0.0f), 60.0f);
	TestEqual(TEXT("interval roll 1 is 120s"),
			  FIncursionScheduler::IncursionIntervalSecondsForUnit(1.0f), 120.0f);
	TestEqual(TEXT("interval roll half is 90s"),
			  FIncursionScheduler::IncursionIntervalSecondsForUnit(0.5f), 90.0f);
	TestEqual(TEXT("interval roll over clamps to 120s"),
			  FIncursionScheduler::IncursionIntervalSecondsForUnit(2.0f), 120.0f);

	// --- Incursions planned per gap: 1 in the lower half, 2 above ---
	TestEqual(TEXT("count roll 0 plans 1"), FIncursionScheduler::PlannedIncursionCountForUnit(0.0f),
			  1);
	TestEqual(TEXT("count roll just under half plans 1"),
			  FIncursionScheduler::PlannedIncursionCountForUnit(0.49f), 1);
	TestEqual(TEXT("count roll half plans 2"), FIncursionScheduler::PlannedIncursionCountForUnit(0.5f),
			  2);
	TestEqual(TEXT("count roll 1 plans 2"), FIncursionScheduler::PlannedIncursionCountForUnit(1.0f),
			  2);
	TestEqual(TEXT("count wild roll clamps to 2"),
			  FIncursionScheduler::PlannedIncursionCountForUnit(9.0f), 2);
	TestEqual(TEXT("count wild low roll clamps to 1"),
			  FIncursionScheduler::PlannedIncursionCountForUnit(-9.0f), 1);

	// --- Spawn gate: plan, concurrency, sandbox ---
	TestTrue(TEXT("first incursion may spawn"), FIncursionScheduler::CanSpawnIncursion(0, 0, 1, false));
	TestTrue(TEXT("second concurrent may spawn"), FIncursionScheduler::CanSpawnIncursion(1, 1, 2, false));
	TestFalse(TEXT("third concurrent is refused"), FIncursionScheduler::CanSpawnIncursion(2, 1, 2, false));
	TestFalse(TEXT("plan exhausted is refused"), FIncursionScheduler::CanSpawnIncursion(0, 2, 2, false));
	TestFalse(TEXT("sandbox refuses any spawn"), FIncursionScheduler::CanSpawnIncursion(0, 0, 2, true));

	// --- Warning boundary: at most 20 s out, not yet due ---
	TestTrue(TEXT("exactly 20s out warns"), FIncursionScheduler::ShouldWarnForWave(20.0f));
	TestFalse(TEXT("20.1s out does not warn"), FIncursionScheduler::ShouldWarnForWave(20.1f));
	TestTrue(TEXT("1s out warns"), FIncursionScheduler::ShouldWarnForWave(1.0f));
	TestFalse(TEXT("due wave is past warning"), FIncursionScheduler::ShouldWarnForWave(0.0f));

	// --- Wave cap boundary: 240 s inclusive ---
	TestFalse(TEXT("239.9s has not capped"), FIncursionScheduler::HasWaveCapExpired(239.9f));
	TestTrue(TEXT("exactly 240s has capped"), FIncursionScheduler::HasWaveCapExpired(240.0f));
	TestTrue(TEXT("past cap stays capped"), FIncursionScheduler::HasWaveCapExpired(600.0f));
	return true;
}

// [2026-11-04 T-131] The stateful clocks: a rolled gap spawns its
// planned incursions on interval, warns once at 20 s, comes due on
// time; the wave clock fires its cap once; sandbox freezes both.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIncursionSchedulerClockTest,
								 "UnderTheMountain.Waves.IncursionScheduler",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIncursionSchedulerClockTest::RunTest(const FString& Parameters)
{
	// --- Gap clock: 90 s gap, first incursion at 60 s, plan of 1 ---
	{
		FIncursionScheduler Scheduler;
		Scheduler.BeginGap(0.0f, 0.0f, 0.0f);
		TestEqual(TEXT("gap rolled to 90s"), Scheduler.GetGapSeconds(), 90.0f);
		TestEqual(TEXT("gap plans 1 incursion"), Scheduler.GetPlannedIncursions(), 1);
		FIncursionCadenceDecision Early = Scheduler.TickGap(59.0f, 0, 0.0f, false);
		TestFalse(TEXT("no incursion before its interval"), Early.bSpawnIncursion);
		TestFalse(TEXT("no warning at 31s out"), Early.bWaveWarningStarted);
		FIncursionCadenceDecision Due = Scheduler.TickGap(1.0f, 0, 0.0f, false);
		TestTrue(TEXT("incursion spawns at its interval"), Due.bSpawnIncursion);
		TestEqual(TEXT("spawn ledger advances"), Scheduler.GetSpawnedIncursions(), 1);
		// Plan exhausted: even with nobody active, no second spawn.
		FIncursionCadenceDecision Late = Scheduler.TickGap(10.0f, 0, 0.0f, false);
		TestFalse(TEXT("plan of 1 spawns no second incursion"), Late.bSpawnIncursion);
		TestTrue(TEXT("warning fires at 20s out"), Late.bWaveWarningStarted);
		TestTrue(TEXT("warning window reads active"), Scheduler.IsWarningActive());
		FIncursionCadenceDecision Again = Scheduler.TickGap(1.0f, 0, 0.0f, false);
		TestFalse(TEXT("warning fires only once"), Again.bWaveWarningStarted);
		FIncursionCadenceDecision End = Scheduler.TickGap(19.0f, 0, 0.0f, false);
		TestTrue(TEXT("wave comes due at gap end"), End.bWaveDue);
		TestEqual(TEXT("no time remains at due"), Scheduler.GetSecondsUntilWave(), 0.0f);
	}

	// --- Concurrency: a due incursion waits while 2 are live ---
	{
		FIncursionScheduler Scheduler;
		Scheduler.BeginGap(1.0f, 0.0f, 1.0f); // 150 s gap, plan 2, first at 60 s
		TestEqual(TEXT("gap plans 2 incursions"), Scheduler.GetPlannedIncursions(), 2);
		FIncursionCadenceDecision Blocked = Scheduler.TickGap(60.0f, 2, 0.0f, false);
		TestFalse(TEXT("two live incursions block the spawn"), Blocked.bSpawnIncursion);
		TestEqual(TEXT("blocked spawn is not spent"), Scheduler.GetSpawnedIncursions(), 0);
		FIncursionCadenceDecision Freed = Scheduler.TickGap(0.1f, 1, 0.0f, false);
		TestTrue(TEXT("spawn proceeds once a slot frees"), Freed.bSpawnIncursion);
	}

	// --- Sandbox freezes the gap clock entirely ---
	{
		FIncursionScheduler Scheduler;
		Scheduler.BeginGap(0.0f, 0.0f, 1.0f);
		FIncursionCadenceDecision Frozen = Scheduler.TickGap(1000.0f, 0, 0.0f, true);
		TestFalse(TEXT("sandbox gap spawns nothing"), Frozen.bSpawnIncursion);
		TestFalse(TEXT("sandbox gap warns nothing"), Frozen.bWaveWarningStarted);
		TestFalse(TEXT("sandbox gap never comes due"), Frozen.bWaveDue);
		TestEqual(TEXT("sandbox gap clock did not advance"), Scheduler.GetSecondsUntilWave(), 90.0f);
	}

	// --- Wave clock: cap fires once at 240 s, re-arms per wave ---
	{
		FIncursionScheduler Scheduler;
		Scheduler.BeginWave();
		TestFalse(TEXT("cap silent at 239s"), Scheduler.TickWave(239.0f, false).bWaveCapReached);
		TestTrue(TEXT("cap fires at 240s"), Scheduler.TickWave(1.0f, false).bWaveCapReached);
		TestFalse(TEXT("cap fires only once"), Scheduler.TickWave(60.0f, false).bWaveCapReached);
		TestEqual(TEXT("wave clock keeps elapsed"), Scheduler.GetWaveElapsedSeconds(), 300.0f);
		Scheduler.BeginWave();
		TestEqual(TEXT("new wave resets the cap clock"), Scheduler.GetWaveElapsedSeconds(), 0.0f);
		TestFalse(TEXT("sandbox wave clock is frozen"),
				  Scheduler.TickWave(1000.0f, true).bWaveCapReached);
		TestEqual(TEXT("sandbox wave clock did not advance"), Scheduler.GetWaveElapsedSeconds(), 0.0f);
	}
	return true;
}

// [2026-11-04 T-131] ForceFlee: a live thief runs for the exit
// keeping its hands; a dead thief ignores the cap entirely.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefForceFleeTest, "UnderTheMountain.Thief.ForceFlee",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefForceFleeTest::RunTest(const FString& Parameters)
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1200;
	Crown.Location = FVector(2000.0f, 0.0f, 0.0f);
	Registry->Register(Crown);

	UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
	Config->MaxHP = 20.0f;
	UThiefBrain* Brain = NewObject<UThiefBrain>();
	Brain->Init(Config, Registry, FName(TEXT("CapThief")));
	Brain->SetExitLocation(FVector(18700.0f, 0.0f, 160.0f));
	TestTrue(TEXT("thief starts sneaking"), Brain->State() == EThiefState::Sneak);
	Brain->ForceFlee();
	TestTrue(TEXT("cap sends the thief fleeing"), Brain->State() == EThiefState::Flee);
	TestTrue(TEXT("flee destination is the exit"),
			 Brain->CurrentDestination().Equals(FVector(18700.0f, 0.0f, 160.0f)));

	UThiefBrain* Dead = NewObject<UThiefBrain>();
	Dead->Init(Config, Registry, FName(TEXT("DeadThief")));
	Dead->ApplyDamage(999.0f, FVector::ZeroVector);
	TestTrue(TEXT("lethal damage kills"), Dead->State() == EThiefState::Dead);
	Dead->ForceFlee();
	TestTrue(TEXT("dead thief ignores the cap"), Dead->State() == EThiefState::Dead);
	return true;
}

// [2026-11-04 T-138] The wave subsystem's own sandbox seam: a
// frozen machine never leaves its first intermission, however
// long it is ticked; lifting the gate lets the cadence resume.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveSandboxTest, "UnderTheMountain.Wave.Sandbox",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveSandboxTest::RunTest(const FString& Parameters)
{
	UWaveSubsystem* Waves = NewObject<UWaveSubsystem>();
	Waves->SetSandboxEnabled(true);
	TestTrue(TEXT("wave sandbox flag reads back"), Waves->IsSandboxEnabled());
	Waves->TickWave(1000.0f);
	TestTrue(TEXT("sandbox wave machine stays in intermission"),
			 Waves->GetWaveState() == EWaveState::Intermission);
	TestEqual(TEXT("sandbox wave machine never starts a wave"), Waves->GetCurrentWave(), 0);
	TestFalse(TEXT("sandbox session is not over"), Waves->IsSessionOver());
	TestFalse(TEXT("sandbox shows no wave warning"), Waves->IsWaveWarningActive());

	// Incursion counting through the test seam: a null slot is a
	// resolved spawn (T-104's rule), never a live incursion.
	Waves->TestRecordIncursionThief(nullptr);
	TestEqual(TEXT("missing incursion thief counts resolved"), Waves->CountActiveIncursions(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
