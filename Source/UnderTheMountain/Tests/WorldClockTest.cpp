// Copyright Under the Mountain contributors.
// Purpose: T-092 world-clock tests — phase buckets, wrapping,
// advance rates and the next-dawn skip sleep (T-084) relies on,
// exercised against the pure FWorldClock and a headless subsystem
// object with no world spawned.

#include "Misc/AutomationTest.h"
#include "World/WorldClock.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-04 T-092] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldClockTest, "UnderTheMountain.World.Clock",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldClockTest::RunTest(const FString& Parameters)
{
	// --- Phase buckets (decision: Night 20-05, Dawn 05-08, Day 08-17, Dusk 17-20) ---
	TestTrue(TEXT("00:00 is night"), FWorldClock::TimeOfDayForHour(0.0f) == ETimeOfDay::Night);
	TestTrue(TEXT("04:59 is night"), FWorldClock::TimeOfDayForHour(4.99f) == ETimeOfDay::Night);
	TestTrue(TEXT("05:00 is dawn"), FWorldClock::TimeOfDayForHour(5.0f) == ETimeOfDay::Dawn);
	TestTrue(TEXT("06:00 (dawn point) is dawn"), FWorldClock::TimeOfDayForHour(6.0f) == ETimeOfDay::Dawn);
	TestTrue(TEXT("08:00 is day"), FWorldClock::TimeOfDayForHour(8.0f) == ETimeOfDay::Day);
	TestTrue(TEXT("16:59 is day"), FWorldClock::TimeOfDayForHour(16.99f) == ETimeOfDay::Day);
	TestTrue(TEXT("17:00 is dusk"), FWorldClock::TimeOfDayForHour(17.0f) == ETimeOfDay::Dusk);
	TestTrue(TEXT("20:00 is night"), FWorldClock::TimeOfDayForHour(20.0f) == ETimeOfDay::Night);
	TestTrue(TEXT("25:00 wraps to night"), FWorldClock::TimeOfDayForHour(25.0f) == ETimeOfDay::Night);
	TestTrue(TEXT("-1:00 wraps to night"), FWorldClock::TimeOfDayForHour(-1.0f) == ETimeOfDay::Night);

	// --- Wrapping keeps the hour in [0, 24) ---
	TestEqual(TEXT("24 wraps to 0"), FWorldClock::WrapHour(24.0f), 0.0f);
	TestEqual(TEXT("25 wraps to 1"), FWorldClock::WrapHour(25.0f), 1.0f);
	TestEqual(TEXT("-1 wraps to 23"), FWorldClock::WrapHour(-1.0f), 23.0f);
	TestEqual(TEXT("in-range hour unchanged"), FWorldClock::WrapHour(12.5f), 12.5f);

	// --- Advance: 600 s cycle = 25 s per game hour ---
	FWorldClock Clock;
	TestEqual(TEXT("clock starts at dawn 06:00"), Clock.Hour, 6.0f);
	Clock.Advance(25.0f);
	TestEqual(TEXT("25 s advances one game hour"), Clock.Hour, 7.0f);
	Clock.Advance(600.0f);
	TestEqual(TEXT("a full cycle returns to the same hour"), Clock.Hour, 7.0f);
	Clock.Advance(-50.0f);
	TestEqual(TEXT("negative time never runs the clock backwards"), Clock.Hour, 7.0f);
	Clock.DayLengthSeconds = 0.0f;
	Clock.Advance(25.0f);
	TestEqual(TEXT("zero cycle length freezes the clock"), Clock.Hour, 7.0f);

	// --- Next dawn: always in (0, 24], at dawn means tomorrow ---
	TestEqual(TEXT("at dawn the next dawn is a full day away"), FWorldClock::HoursUntilNextDawn(6.0f),
			  24.0f);
	TestEqual(TEXT("05:00 is one hour before dawn"), FWorldClock::HoursUntilNextDawn(5.0f), 1.0f);
	TestEqual(TEXT("07:00 waits 23 hours for dawn"), FWorldClock::HoursUntilNextDawn(7.0f), 23.0f);
	TestEqual(TEXT("midnight waits 6 hours"), FWorldClock::HoursUntilNextDawn(0.0f), 6.0f);

	FWorldClock SleepClock;
	SleepClock.Hour = 20.0f;
	const float Skipped = SleepClock.AdvanceToNextDawn();
	TestEqual(TEXT("sleeping at 20:00 skips 10 hours"), Skipped, 10.0f);
	TestEqual(TEXT("sleep lands exactly on dawn"), SleepClock.Hour, 6.0f);
	TestTrue(TEXT("sleep lands in the dawn phase"), SleepClock.TimeOfDay() == ETimeOfDay::Dawn);
	const float SkippedAgain = SleepClock.AdvanceToNextDawn();
	TestEqual(TEXT("back-to-back sleep still skips a full day (no zero-skip loop)"), SkippedAgain,
			  24.0f);

	// --- Subsystem seam: same math behind the ticking owner ---
	UWorldClockSubsystem* Subsystem = NewObject<UWorldClockSubsystem>();
	TestEqual(TEXT("subsystem starts at dawn"), Subsystem->GetHour(), 6.0f);
	Subsystem->SetHour(17.5f);
	TestTrue(TEXT("subsystem reports dusk"), Subsystem->GetTimeOfDay() == ETimeOfDay::Dusk);
	Subsystem->Tick(25.0f);
	TestEqual(TEXT("subsystem tick advances one game hour per 25 s"), Subsystem->GetHour(), 18.5f);
	const float SubsystemSkipped = Subsystem->SleepToNextDawn();
	TestTrue(TEXT("subsystem sleep skip is positive"), SubsystemSkipped > 0.0f);
	TestEqual(TEXT("subsystem sleep lands on dawn"), Subsystem->GetHour(), 6.0f);
	Subsystem->SetHour(30.0f);
	TestEqual(TEXT("subsystem SetHour wraps"), Subsystem->GetHour(), 6.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
