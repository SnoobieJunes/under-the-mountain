// Copyright Under the Mountain contributors.
// Purpose: pure sleep rules (T-084, docs/GAMEPLAY.md S9) — whether
// a sleep may start, what one generation of sleep does to
// Notoriety, the hoard and the dragon, and how it ends. No world,
// no actors: the PIE glue (hoard mound trigger, Sleep/Wake clips
// from T-068, applying outcomes through UNotorietySubsystem::
// ApplySleepDecay and the hoard/stats owners) calls ResolveSleep
// once and applies the returned outcome verbatim. Created
// 2026-11-04 (T-084).
// Delegated owner decisions (T-084 was Gated: owner; S9 numbers
// confirmed, the open items decided here): one sleep skips to the
// next dawn on the T-092 world clock (a "generation" in flavour;
// the clock skip is what the world simulates), so the skip is
// 0-24 game hours and always > 0. Bulk risk is 15% OF each bulk
// stack's gold (S9's Risk%), rounded to the nearest gold, not a
// 15% chance to lose the stack; the 5% deep-plunder roll is the
// only random event and it can take at most one UNEQUIPPED
// artifact. HP on wake restores 50% of missing HP and stamina
// refills fully (S9); magic is untouched (T-080 decision: time is
// its regen, and a dawn of sleep already passes through
// RegenMagic's rate in real play — ResolveSleep itself moves no
// magic so the rule stays testable without a clock).
// Cannot soft-lock — wake conditions, enumerated: sleep here is
// an atomic skip, not a state the dragon sits in, so there is no
// sleeping state to get stuck in. A started sleep ALWAYS ends in
// exactly one of: (1) SkipComplete — the clock reaches next dawn
// (guaranteed: HoursUntilNextDawn is in (0, 24]); (2)
// DamageInterrupt or (3) ThiefIntrusion — reserved for the PIE
// glue if it ever plays the skip over real time; either forces
// an immediate wake (decay/risk are computed once, at wake, from
// pre-sleep state — never partially applied mid-skip);
// (4) PlayerCancel before the skip starts is just CanSleep
// returning to the caller — no state was entered. Back-to-back
// sleeps are allowed (S9): CanSleep has no cooldown, the hoard
// risk roll is the cost curve, and each repeat still advances a
// full (0, 24] skip, so repeat-sleep can never spin in place.

#pragma once

#include "CoreMinimal.h"

// [2026-11-04 T-084] Why a sleep may not start (S9 trigger rule:
// at the hoard mound, only between waves, no live thief in lair).
// None means it may.
enum class ESleepRefusal : uint8
{
	None,
	NotAtHoardMound,
	WaveInProgress,
	ThiefInLair
};

// [2026-11-04 T-084] Everything one sleep needs to know, gathered
// by the caller from the single-writer owners (notoriety points
// from UNotorietySubsystem, gold/artifact state from the hoard,
// dragon vitals from UDragonStatsComponent). Random01 is the
// injected randomness seam: the caller supplies a roll in [0, 1),
// tests supply exact values, so no rule here touches FMath::Rand.
struct UNDERTHEMOUNTAIN_API FSleepRequest
{
	int32 NotorietyPoints = 0;
	// [2026-11-04 T-084] Total gold in bulk stacks only (Trinket-
	// Prize brackets, <400 gold per item per S3); artifact-class
	// gold is never in this number (deep plunder handles it).
	int64 BulkGold = 0;
	bool bHasUnequippedArtifact = false;
	float CurrentHP = 100.0f;
	float MaxHP = 100.0f;
	float MaxStamina = 100.0f;
	float Random01 = 1.0f;
};

// [2026-11-04 T-084] What one sleep did. The caller applies it
// through the owners: NotorietyPoints via UNotorietySubsystem::
// ApplySleepDecay (which recomputes this same value — the field
// exists so tests and UI can state the outcome without a
// subsystem), BulkGoldLost via the hoard owner, HP/Stamina via
// UDragonStatsComponent's clamped verbs.
struct UNDERTHEMOUNTAIN_API FSleepOutcome
{
	int32 NotorietyPoints = 0;
	int32 Stars = 0;
	int64 BulkGoldLost = 0;
	bool bArtifactTaken = false;
	float HPOnWake = 0.0f;
	float StaminaOnWake = 0.0f;
};

// [2026-11-04 T-084] Namespace-as-struct of static pure functions,
// matching FScentMath/FWorldClock's owner-free contract.
struct UNDERTHEMOUNTAIN_API FSleepRules
{
	// [2026-11-04 T-084] S9 tuning as named constants so a PT-WAVE
	// retune is one edit and tests pin the shipped values.
	static constexpr float BulkRiskFraction = 0.15f;
	static constexpr float DeepPlunderChance = 0.05f;

	// [2026-11-04 T-084] The trigger gate. Checked in S9's order
	// (place, then wave state, then thief presence) so the refusal
	// a caller logs names the first broken condition.
	static ESleepRefusal CanSleep(bool bAtHoardMound, bool bWaveInProgress, bool bLiveThiefInLair);

	// [2026-11-04 T-084] The pawn's grounded verb glue reads this:
	// "on/near the hoard mound" is a 2D distance from the mound
	// actor (height on the pile must not refuse a sleep), inside
	// HoardMoundSleepRadiusCm; a non-positive radius never sleeps.
	static constexpr float HoardMoundSleepRadiusCm = 1500.0f;
	static bool IsNearHoardMound(const FVector& DragonLocation, const FVector& MoundLocation, float RadiusCm);

	// [2026-11-04 T-084] Gold lost from ONE bulk stack: 15% of its
	// gold, rounded to nearest, never negative and never more
	// than the stack holds. Per-stack (S9: rolled per bracket-
	// stack, not per item), so a hoard of small stacks loses what
	// one merged stack would, +/- rounding per stack.
	static int64 BulkGoldLostForStack(int64 StackGold);

	// [2026-11-04 T-084] The deep-plunder roll: true only when the
	// injected roll lands under 5% AND an unequipped artifact
	// exists to take. Equipped artifacts are never taken (S6/S9),
	// so a dragon wearing everything it owns is plunder-proof.
	static bool DeepPlunderTakesArtifact(float Random01, bool bHasUnequippedArtifact);

	// [2026-11-04 T-084] S9 wake HP: current + 50% of missing,
	// clamped to [0, MaxHP]; a non-positive max means no pool to
	// restore into and yields 0 rather than dividing by it.
	static float HPOnWake(float CurrentHP, float MaxHP);

	// [2026-11-04 T-084] One full sleep from a request. Notoriety
	// goes through UNotorietySubsystem's pure decay statics (the
	// single writer owns the band math; this never re-derives it).
	static FSleepOutcome ResolveSleep(const FSleepRequest& Request);
};
