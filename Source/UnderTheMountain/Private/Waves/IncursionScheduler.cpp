// Copyright Under the Mountain contributors.
// [2026-11-04 T-131] FIncursionScheduler implementation: the
// section 1 cadence table as clamped unit-roll mappings, plus the
// gap/wave clocks. Every range is enforced here, not by callers,
// so no roll (or tuning edit) can produce an out-of-spec rhythm.

#include "Waves/IncursionScheduler.h"

namespace
{
// [2026-11-04 T-131] One clamp for every roll: a unit is [0,1],
// and anything outside is caller error read as the nearest end.
float ClampUnit(float Unit)
{
	return FMath::Clamp(Unit, 0.0f, 1.0f);
}
} // namespace

float FIncursionScheduler::GapSecondsForUnit(float Unit)
{
	return MinGapSeconds + ClampUnit(Unit) * (MaxGapSeconds - MinGapSeconds);
}

float FIncursionScheduler::IncursionIntervalSecondsForUnit(float Unit)
{
	return MinIncursionIntervalSeconds +
		   ClampUnit(Unit) * (MaxIncursionIntervalSeconds - MinIncursionIntervalSeconds);
}

int32 FIncursionScheduler::PlannedIncursionCountForUnit(float Unit)
{
	// [2026-11-04 T-131] Lower half plans 1, upper half plans 2;
	// the clamp keeps a wild roll inside the [1, 2] spec band.
	return ClampUnit(Unit) < 0.5f ? MinIncursionsPerGap : MaxIncursionsPerGap;
}

bool FIncursionScheduler::CanSpawnIncursion(int32 ActiveIncursions, int32 SpawnedThisGap,
											int32 PlannedThisGap, bool bSandbox)
{
	if (bSandbox)
	{
		return false;
	}
	if (SpawnedThisGap >= PlannedThisGap)
	{
		return false;
	}
	return ActiveIncursions < MaxConcurrentIncursions;
}

bool FIncursionScheduler::ShouldWarnForWave(float SecondsUntilWave)
{
	return SecondsUntilWave > 0.0f && SecondsUntilWave <= WaveWarningSeconds;
}

bool FIncursionScheduler::HasWaveCapExpired(float InWaveElapsedSeconds)
{
	return InWaveElapsedSeconds >= WaveCapSeconds;
}

void FIncursionScheduler::BeginGap(float GapUnit, float IntervalUnit, float CountUnit)
{
	GapSeconds = GapSecondsForUnit(GapUnit);
	PlannedIncursions = PlannedIncursionCountForUnit(CountUnit);
	NextIncursionAtSeconds = IncursionIntervalSecondsForUnit(IntervalUnit);
	GapElapsedSeconds = 0.0f;
	SpawnedIncursions = 0;
	bWarningFired = false;
}

void FIncursionScheduler::BeginWave()
{
	WaveElapsedSeconds = 0.0f;
	bWaveCapFired = false;
}

FIncursionCadenceDecision FIncursionScheduler::TickGap(float DeltaSeconds,
													   int32 ActiveIncursions,
													   float NextIntervalUnit, bool bSandbox)
{
	FIncursionCadenceDecision Decision;
	if (bSandbox || DeltaSeconds <= 0.0f)
	{
		return Decision;
	}
	GapElapsedSeconds += DeltaSeconds;
	// [2026-11-04 T-131] Spawn before warning/due verdicts so a
	// thief owed on the wave's doorstep still enters — the wave
	// itself, not the scheduler, resolves overlap from there.
	if (GapElapsedSeconds >= NextIncursionAtSeconds &&
		CanSpawnIncursion(ActiveIncursions, SpawnedIncursions, PlannedIncursions, false))
	{
		Decision.bSpawnIncursion = true;
		++SpawnedIncursions;
		NextIncursionAtSeconds =
			GapElapsedSeconds + IncursionIntervalSecondsForUnit(NextIntervalUnit);
	}
	if (!bWarningFired && ShouldWarnForWave(GetSecondsUntilWave()))
	{
		Decision.bWaveWarningStarted = true;
		bWarningFired = true;
	}
	if (GapElapsedSeconds >= GapSeconds)
	{
		Decision.bWaveDue = true;
	}
	return Decision;
}

FIncursionCadenceDecision FIncursionScheduler::TickWave(float DeltaSeconds, bool bSandbox)
{
	FIncursionCadenceDecision Decision;
	if (bSandbox || DeltaSeconds <= 0.0f)
	{
		return Decision;
	}
	WaveElapsedSeconds += DeltaSeconds;
	if (!bWaveCapFired && HasWaveCapExpired(WaveElapsedSeconds))
	{
		Decision.bWaveCapReached = true;
		bWaveCapFired = true;
	}
	return Decision;
}

float FIncursionScheduler::GetGapSeconds() const
{
	return GapSeconds;
}

float FIncursionScheduler::GetSecondsUntilWave() const
{
	return FMath::Max(0.0f, GapSeconds - GapElapsedSeconds);
}

bool FIncursionScheduler::IsWarningActive() const
{
	return ShouldWarnForWave(GetSecondsUntilWave());
}

int32 FIncursionScheduler::GetPlannedIncursions() const
{
	return PlannedIncursions;
}

int32 FIncursionScheduler::GetSpawnedIncursions() const
{
	return SpawnedIncursions;
}

float FIncursionScheduler::GetWaveElapsedSeconds() const
{
	return WaveElapsedSeconds;
}
