// Copyright Under the Mountain contributors.
// [2026-11-04 T-092] FWorldClock pure math first (headless-tested),
// then the thin UTickableWorldSubsystem that ticks one per world.

#include "World/WorldClock.h"

namespace
{
// [2026-11-04 T-092] Dawn's hour. Named once here because both the
// phase table and the sleep jump measure against it; FSleepRules
// (T-084) states its skip in these same terms through the clock.
constexpr float DawnHour = 6.0f;
} // namespace

float FWorldClock::WrapHour(float InHour)
{
	// [2026-11-04 T-092] Fmod keeps the sign of the dividend, so a
	// negative hour is lifted by a full day after the mod; the
	// result is always in [0, 24).
	float Wrapped = FMath::Fmod(InHour, 24.0f);
	if (Wrapped < 0.0f)
	{
		Wrapped += 24.0f;
	}
	return Wrapped;
}

ETimeOfDay FWorldClock::TimeOfDayForHour(float InHour)
{
	// [2026-11-04 T-092] Phase table from the header decision:
	// Dawn 05-08, Day 08-17, Dusk 17-20, Night otherwise. Dawn
	// starts an hour before the 06:00 sunrise point so the sky
	// can brighten into the moment SleepToNextDawn lands on.
	const float H = WrapHour(InHour);
	if (H >= 5.0f && H < 8.0f)
	{
		return ETimeOfDay::Dawn;
	}
	if (H >= 8.0f && H < 17.0f)
	{
		return ETimeOfDay::Day;
	}
	if (H >= 17.0f && H < 20.0f)
	{
		return ETimeOfDay::Dusk;
	}
	return ETimeOfDay::Night;
}

ETimeOfDay FWorldClock::TimeOfDay() const
{
	return TimeOfDayForHour(Hour);
}

float FWorldClock::Advance(float DeltaSeconds)
{
	// [2026-11-04 T-092] A non-positive cycle length freezes the
	// clock instead of dividing by it; negative delta never runs
	// time backwards (callers wanting a jump use AdvanceHours).
	if (DayLengthSeconds <= 0.0f)
	{
		return Hour;
	}
	const float GameHours = FMath::Max(0.0f, DeltaSeconds) * (24.0f / DayLengthSeconds);
	return AdvanceHours(GameHours);
}

float FWorldClock::AdvanceHours(float GameHours)
{
	Hour = WrapHour(Hour + GameHours);
	return Hour;
}

float FWorldClock::HoursUntilNextDawn(float InHour)
{
	// [2026-11-04 T-092] In (0, 24] by construction: the raw
	// distance is wrapped, and an exact hit on dawn wraps to a
	// full day rather than reporting a zero-length skip.
	const float Distance = WrapHour(DawnHour - WrapHour(InHour));
	return Distance > 0.0f ? Distance : 24.0f;
}

float FWorldClock::AdvanceToNextDawn()
{
	const float Skipped = HoursUntilNextDawn(Hour);
	AdvanceHours(Skipped);
	return Skipped;
}

void UWorldClockSubsystem::Tick(float DeltaTime)
{
	Clock.Advance(DeltaTime);
}

TStatId UWorldClockSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldClockSubsystem, STATGROUP_Tickables);
}

float UWorldClockSubsystem::GetHour() const
{
	return Clock.Hour;
}

ETimeOfDay UWorldClockSubsystem::GetTimeOfDay() const
{
	return Clock.TimeOfDay();
}

const FWorldClock& UWorldClockSubsystem::GetClock() const
{
	return Clock;
}

float UWorldClockSubsystem::SleepToNextDawn()
{
	// [2026-11-04 T-092] Delegates to the pure clock so the tested
	// math and the shipped jump are the same code path.
	return Clock.AdvanceToNextDawn();
}

void UWorldClockSubsystem::SetHour(float InHour)
{
	Clock.Hour = FWorldClock::WrapHour(InHour);
}
