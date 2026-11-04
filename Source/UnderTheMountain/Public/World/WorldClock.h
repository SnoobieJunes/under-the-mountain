// Copyright Under the Mountain contributors.
// Purpose: the world clock (T-092) — one seam owning time-of-day so
// sleep (T-084), incursion cadence and sky code read the same time
// instead of each inventing a sun. Created 2026-11-04 (T-092).
// Delegated owner decisions (T-092 was Gated: owner): one full
// cycle is 600 real seconds (10 minutes) for 24 game hours, so one
// game hour is 25 s — fast enough that a playtest sees dusk, slow
// enough that a wave (<=4 min, GAMEPLAY.md S1) is not chased by
// nightfall. Dawn is 06:00; phases are Night 20:00-05:00, Dawn
// 05:00-08:00, Day 08:00-17:00, Dusk 17:00-20:00. Sleep jumps to
// the NEXT dawn (see FSleepRules), never a zero-length skip.
// Invariants: FWorldClock's hour is always in [0, 24) after any of
// its methods; HoursUntilNextDawn is always in (0, 24], so a sleep
// skip always advances time and can never loop in place.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldClock.generated.h"

// [2026-11-04 T-092] Queryable time-of-day bucket. Cadence and
// schedule code switch on this, never on raw hour comparisons
// scattered per call site.
UENUM()
enum class ETimeOfDay : uint8
{
	Night,
	Dawn,
	Day,
	Dusk
};

// [2026-11-04 T-092] Pure clock state + math, owner-free like
// FScentMath: same inputs, same outputs, no world. The subsystem
// below only advances one of these per tick and exposes it.
struct UNDERTHEMOUNTAIN_API FWorldClock
{
	// [2026-11-04 T-092] Real seconds for one 24 h cycle (decision
	// in the file header). A tuning field on the clock, not a
	// subsystem constant, so tests can run a fast clock.
	float DayLengthSeconds = 600.0f;

	// [2026-11-04 T-092] Hour of day in [0, 24). Starts at dawn:
	// the intro wakes the dragon on its hoard (GAMEPLAY.md S7).
	float Hour = 6.0f;

	// [2026-11-04 T-092] Normalises any hour (negative, >24) into
	// [0, 24) by wrapping full days away.
	static float WrapHour(float InHour);

	// [2026-11-04 T-092] The phase bucket for an hour (wrapped
	// first, so 25:00 reads as 01:00 Night, never garbage).
	static ETimeOfDay TimeOfDayForHour(float InHour);

	ETimeOfDay TimeOfDay() const;

	// [2026-11-04 T-092] Advances by real seconds at the cycle
	// rate; negative time and a non-positive cycle length are free
	// by construction (the clock never runs backwards). Returns
	// the new hour.
	float Advance(float DeltaSeconds);

	// [2026-11-04 T-092] Advances by game hours (sleep's unit),
	// wrapping past midnight. Negative hours wrap too, through
	// WrapHour, though no caller should pass one.
	float AdvanceHours(float GameHours);

	// [2026-11-04 T-092] Hours from InHour until the next 06:00,
	// always in (0, 24]: exactly-at-dawn means a full day to the
	// NEXT dawn. This is the anti-soft-lock guarantee sleep
	// relies on (a zero skip could re-offer sleep forever).
	static float HoursUntilNextDawn(float InHour);

	// [2026-11-04 T-092] Jumps to the next dawn and returns how
	// many game hours were skipped (HoursUntilNextDawn of the
	// pre-jump hour).
	float AdvanceToNextDawn();
};

// [2026-11-04 T-092] The ticking owner of one FWorldClock per
// world. T-052's sky reads GetHour()/GetTimeOfDay() from here when
// it is wired; nothing else may keep its own time-of-day state.
UCLASS()
class UNDERTHEMOUNTAIN_API UWorldClockSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-11-04 T-092] Ticks the clock by real delta time. The
	// engine calls this (UTickableWorldSubsystem); gameplay code
	// never calls it directly.
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	float GetHour() const;
	ETimeOfDay GetTimeOfDay() const;
	const FWorldClock& GetClock() const;

	// [2026-11-04 T-092] Sleep's entry point (T-084): jumps to the
	// next dawn and returns game hours skipped (always > 0).
	float SleepToNextDawn();

	// [2026-11-04 T-092] Test/PIE seam: sets the hour directly
	// (wrapped). Production time travel goes through
	// SleepToNextDawn; this exists so a test or console check can
	// stage a dusk without waiting a cycle out.
	void SetHour(float InHour);

  private:
	FWorldClock Clock;
};
