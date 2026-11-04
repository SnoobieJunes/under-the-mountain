// Copyright Under the Mountain contributors.
// Purpose: the pure session-cadence rules from docs/GAMEPLAY.md
// section 1 — free-play gaps between waves, single-thief incursions
// inside those gaps, the wave warning, and the wave duration cap —
// with no world, actor, or clock dependency, so automation tests
// exercise the rhythm directly (Tests/IncursionSchedulerTest.cpp).
// Created 2026-11-04 (T-131).
// Invariants: every duration the scheduler emits stays inside its
// spec range however the injected unit roll arrives (rolls are
// clamped, never trusted); at most MaxConcurrentIncursions thieves
// are ever live from incursions, and at most PlannedIncursions are
// spawned per gap; sandbox suppresses every cadence verb. Randomness
// rides in as unit rolls ([0,1]) from the caller, the FScentMath
// injected-seam pattern, so tests drive exact boundary rolls.

#pragma once

#include "CoreMinimal.h"

// [2026-11-04 T-131] One tick's cadence verdicts. All false is the
// common tick; the wave subsystem acts on (and logs) each true flag
// exactly once because the scheduler latches them internally.
struct UNDERTHEMOUNTAIN_API FIncursionCadenceDecision
{
	bool bSpawnIncursion = false;
	bool bWaveWarningStarted = false;
	bool bWaveDue = false;
	bool bWaveCapReached = false;
};

// [2026-11-04 T-131] Namespace-as-struct of pure cadence rules plus
// the small stateful gap/wave clock the wave subsystem ticks. The
// static functions are the spec table; the instance half only
// remembers elapsed time and which one-shot verdicts already fired.
struct UNDERTHEMOUNTAIN_API FIncursionScheduler
{
	// [2026-11-04 T-131] Spec ranges (GAMEPLAY.md section 1):
	// free-play gaps 90-150 s, incursion spawn interval 60-120 s,
	// 1-2 incursions per gap, at most 2 concurrent, 20 s warning,
	// 4-minute wave cap. Named constants so tuning edits one site.
	static constexpr float MinGapSeconds = 90.0f;
	static constexpr float MaxGapSeconds = 150.0f;
	static constexpr float MinIncursionIntervalSeconds = 60.0f;
	static constexpr float MaxIncursionIntervalSeconds = 120.0f;
	static constexpr int32 MinIncursionsPerGap = 1;
	static constexpr int32 MaxIncursionsPerGap = 2;
	static constexpr int32 MaxConcurrentIncursions = 2;
	static constexpr float WaveWarningSeconds = 20.0f;
	static constexpr float WaveCapSeconds = 240.0f;

	// [2026-11-04 T-131] Maps a unit roll to a gap length inside
	// [90, 150]: 0 is the shortest breather, 1 the longest, and
	// out-of-range rolls clamp to the nearest end (same inclusive
	// boundary contract as the hoard classifier).
	static float GapSecondsForUnit(float Unit);
	// [2026-11-04 T-131] Maps a unit roll to the wait before the
	// next incursion inside [60, 120]; clamps like the gap above.
	static float IncursionIntervalSecondsForUnit(float Unit);
	// [2026-11-04 T-131] Maps a unit roll to how many single-thief
	// incursions a gap plans: the lower half of the roll plans 1,
	// the upper half plans 2, never outside [1, 2].
	static int32 PlannedIncursionCountForUnit(float Unit);
	// [2026-11-04 T-131] The concurrency/planning gate for one
	// spawn: a gap may spawn only while it still owes incursions,
	// fewer than 2 incursion thieves are live, and the run is not
	// sandbox (T-138's rule, enforced here as well as upstream).
	static bool CanSpawnIncursion(int32 ActiveIncursions, int32 SpawnedThisGap,
								  int32 PlannedThisGap, bool bSandbox);
	// [2026-11-04 T-131] The warning fires when the wave is at
	// most WaveWarningSeconds away (and not already due): exactly
	// 20.0 s remaining warns, 20.1 s does not.
	static bool ShouldWarnForWave(float SecondsUntilWave);
	// [2026-11-04 T-131] The wave cap: at 240 s elapsed the wave
	// is expired and its survivors must flee — the anti-soft-lock
	// rule for a wave the dragon ignores.
	static bool HasWaveCapExpired(float WaveElapsedSeconds);

	// [2026-11-04 T-131] Opens a free-play gap: rolls the gap
	// length, the planned incursion count, and the wait before the
	// first incursion. All three rolls are units; see the statics.
	void BeginGap(float GapUnit, float IntervalUnit, float CountUnit);
	// [2026-11-04 T-131] Opens a wave: starts the cap clock and
	// re-arms the one-shot cap verdict for this wave.
	void BeginWave();
	// [2026-11-04 T-131] Advances the gap clock. NextIntervalUnit
	// is the roll for the wait after a spawn this tick (ignored on
	// ticks that spawn nothing). Sandbox freezes every verdict —
	// the clock does not even advance, so leaving sandbox (a fresh
	// run in practice) never inherits a half-spent gap.
	FIncursionCadenceDecision TickGap(float DeltaSeconds, int32 ActiveIncursions,
									  float NextIntervalUnit, bool bSandbox);
	// [2026-11-04 T-131] Advances the wave cap clock; the cap
	// verdict fires exactly once per BeginWave. Sandbox freezes it
	// for the same reason as TickGap.
	FIncursionCadenceDecision TickWave(float DeltaSeconds, bool bSandbox);

	// [2026-11-04 T-131] Gap readouts for HUD/log callers: total
	// rolled length, time left until the wave is due, whether the
	// 20 s warning window is open, and the incursion ledger.
	float GetGapSeconds() const;
	float GetSecondsUntilWave() const;
	bool IsWarningActive() const;
	int32 GetPlannedIncursions() const;
	int32 GetSpawnedIncursions() const;
	// [2026-11-04 T-131] Seconds the current wave has been running
	// (the cap clock TickWave advances; 0 before the first wave).
	float GetWaveElapsedSeconds() const;

  private:
	float GapSeconds = 120.0f;
	float GapElapsedSeconds = 0.0f;
	float NextIncursionAtSeconds = 60.0f;
	int32 PlannedIncursions = 1;
	int32 SpawnedIncursions = 0;
	bool bWarningFired = false;
	float WaveElapsedSeconds = 0.0f;
	bool bWaveCapFired = false;
};
