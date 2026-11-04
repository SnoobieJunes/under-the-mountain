// Copyright Under the Mountain contributors.
// [2026-11-04 T-084] Pure sleep rules; see SleepRules.h for the
// delegated decisions and the enumerated wake conditions.

#include "World/SleepRules.h"
#include "Notoriety/NotorietySubsystem.h"

ESleepRefusal FSleepRules::CanSleep(bool bAtHoardMound, bool bWaveInProgress, bool bLiveThiefInLair)
{
	// [2026-11-04 T-084] S9 trigger: sleeping mid-burglary is a
	// forfeit button, not a choice, and the mound is the only bed.
	if (!bAtHoardMound)
	{
		return ESleepRefusal::NotAtHoardMound;
	}
	if (bWaveInProgress)
	{
		return ESleepRefusal::WaveInProgress;
	}
	if (bLiveThiefInLair)
	{
		return ESleepRefusal::ThiefInLair;
	}
	return ESleepRefusal::None;
}

int64 FSleepRules::BulkGoldLostForStack(int64 StackGold)
{
	// [2026-11-04 T-084] 15% of the stack, rounded to nearest gold.
	// Clamped to [0, StackGold] so a tiny or negative stack can
	// never lose gold it does not have (or gain by losing).
	if (StackGold <= 0)
	{
		return 0;
	}
	const int64 Lost = FMath::RoundToInt64(static_cast<double>(StackGold) * BulkRiskFraction);
	return FMath::Clamp<int64>(Lost, 0, StackGold);
}

bool FSleepRules::DeepPlunderTakesArtifact(float Random01, bool bHasUnequippedArtifact)
{
	// [2026-11-04 T-084] The roll is clamped into [0, 1] first: an
	// out-of-range injected roll must not silently widen or kill
	// the 5% chance. Exactly-at-chance does NOT take (under, S9).
	const float Roll = FMath::Clamp(Random01, 0.0f, 1.0f);
	return bHasUnequippedArtifact && Roll < DeepPlunderChance;
}

float FSleepRules::HPOnWake(float CurrentHP, float MaxHP)
{
	// [2026-11-04 T-084] S9: restore 50% of missing HP. Current is
	// clamped into the pool first so pre-sleep over-max or
	// negative values cannot inflate or invert the restore.
	if (MaxHP <= 0.0f)
	{
		return 0.0f;
	}
	const float ClampedCurrent = FMath::Clamp(CurrentHP, 0.0f, MaxHP);
	return ClampedCurrent + 0.5f * (MaxHP - ClampedCurrent);
}

FSleepOutcome FSleepRules::ResolveSleep(const FSleepRequest& Request)
{
	// [2026-11-04 T-084] Composes the statics above plus the
	// notoriety single writer's decay math. BulkGold in the
	// request is treated as one aggregated bulk stack (the PIE
	// glue may call BulkGoldLostForStack per stack instead when
	// it wants per-stack rounding; the fraction is identical).
	FSleepOutcome Outcome;
	Outcome.NotorietyPoints = UNotorietySubsystem::PointsAfterSleepDecay(Request.NotorietyPoints);
	Outcome.Stars = UNotorietySubsystem::StarsForPoints(Outcome.NotorietyPoints);
	Outcome.BulkGoldLost = BulkGoldLostForStack(Request.BulkGold);
	Outcome.bArtifactTaken = DeepPlunderTakesArtifact(Request.Random01, Request.bHasUnequippedArtifact);
	Outcome.HPOnWake = HPOnWake(Request.CurrentHP, Request.MaxHP);
	// [2026-11-04 T-084] S9: stamina full on wake; MaxStamina is
	// clamped at 0 so a pool-less request wakes at 0, not negative.
	Outcome.StaminaOnWake = FMath::Max(0.0f, Request.MaxStamina);
	return Outcome;
}
