// Copyright Under the Mountain contributors.
// Purpose: dragon resources (stamina first) behind one component so flight,
// combat and UI never reach into each other. Created 2026-10-27 (T-003).
// [2026-10-28 T-011] Extended with the HP pool, flight-mode drain rates
// (glide/hover/dives are classified by the pawn), exhaustion HP drain and
// the hard-landing damage curve from docs/GAMEPLAY.md section 2.
// Invariants: stamina is clamped to [0, MaxStamina] and HP to
// [0, MaxHP] when written through this component's methods.
// [2026-11-04 T-080] Magic joins as the third pool on the same
// contract: clamped to [0, MaxMagic] through this component's
// methods. Delegated owner decision (T-080 was Gated: owner): pool
// 100 per GAMEPLAY.md S2's percent-scale table; base regen 0.5/s
// over time per S6's Magic Pendant line ("base regen 0.5/s", the
// pendant's +1/s is T-081 artifact work, not baked in here); no
// hoard-proximity or food source in M1, so time is the only regen
// source and sleeping (T-084) does not refill magic. True-sight is
// the only M1 drain (S3: 3/s, held by UScentComponent).
// [2026-10-28 T-041] Honest contract, not encapsulation: Stamina, HP
// and the tuning fields below are public UPROPERTYs, so the editor
// and tests can (and tests do) write them directly, bypassing the
// clamping. In production code paths this component's methods are
// the only writers of either pool — the pawn asks through them and
// never assigns the pools directly (PLAN.md seams).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DragonStatsComponent.generated.h"

// [2026-11-04 T-074] Hit/defeat notifications. Plain (non-dynamic)
// multicast delegates: C++ consumers (T-068 hit reactions, GameFlow
// defeat) bind lambdas; damage amount + HP-after are the payload.
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDragonHit, float, float);
DECLARE_MULTICAST_DELEGATE(FOnDragonDefeated);

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonStatsComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UDragonStatsComponent();

	// [2026-10-27 T-003] Flight/flap tuning lives on the component so T-011
	// feel work tweaks data, not pawn code.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MaxStamina = 100.0f;

	// [2026-10-28 T-011] FlyingDrainPerSecond is kept as the fallback rate
	// for the legacy DrainForFlight(dt) overload only: the pawn now picks
	// glide/hover per tick (below) and calls DrainForFlight(dt, rate).
	// The field is retained so existing tests/callers that set it keep
	// working; its default follows glide, the free unpowered travel.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float FlyingDrainPerSecond = 0.0f;

	// [2026-10-28 T-011] GAMEPLAY.md section 2 (all [tuning: PT-STAM]):
	// [2026-11-03 T-062] glide 1/s -> 0/s: unpowered travel is free.
	// [2026-11-04 T-124] hover 8/s -> 3/s: the playtest dragon could not
	// stay airborne (8/s empties the pool in 12.5 s); 3/s prices the
	// loiter without grounding the fantasy. Glide stays 0/s.
	// glide 0/s, hover -3/s, dive 0/s (a zero rate is free; glide keeps its
	// field so the rate stays data, dive passes 0 inline and has none).
	// Hover is "flying but barely moving horizontally";
	// the pawn classifies by comparing horizontal speed to this threshold.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float GlideDrainPerSecond = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float HoverDrainPerSecond = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float HoverSpeedThreshold = 150.0f;

	// [2026-10-28 T-011] At 0 stamina a draining action burns HP instead
	// (GAMEPLAY.md section 2: -2 HP/s = 50 s of warning on a full pool).
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float ExhaustionHPDrainPerSecond = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float GroundRegenPerSecond = 15.0f;

	// [2026-10-28 T-011] -3 per wingbeat (was a 10 placeholder in T-003);
	// GAMEPLAY.md section 2 attaches the cost to the discrete flap input.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float FlapStaminaCost = 3.0f;

	// [2026-11-04 T-124] One Ascent/Surge impulse costs one full reserve
	// (GAMEPLAY.md section 2, [tuning: PT-STAM]): 20 -> 10, so a launch
	// sequence (Ascent + entry Surge) costs 20 instead of 40 and a full
	// pool holds ten impulses. New as data (was a 20.0f literal at every
	// call site) so the retune is one edit and automation can pin it.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float ImpulseReserveCost = 10.0f;

	UPROPERTY(VisibleAnywhere, Category = "Dragon|Stats")
	float Stamina;

	// [2026-10-28 T-011] HP pool (GAMEPLAY.md section 2: 100 so damage
	// values below read as percent of pool).
	// [2026-10-28 T-041] Same public-UPROPERTY contract as stamina
	// (file header): production writes go through this component's
	// HP methods; the editor/tests may set the fields directly.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MaxHP = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Dragon|Stats")
	float HP;

	// [2026-11-04 T-080] Magic pool (GAMEPLAY.md S2: 100, same
	// percent scale as HP/stamina). Same public-UPROPERTY contract
	// as the other pools: production writes go through the magic
	// methods below; the editor/tests may set the fields directly.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MaxMagic = 100.0f;

	UPROPERTY(VisibleAnywhere, Category = "Dragon|Stats")
	float Magic;

	// [2026-11-04 T-080] Base magic regen (GAMEPLAY.md S6 pendant
	// line pins the base at 0.5/s). Time-based only by decision (see
	// file header); UScentComponent pauses it while true-sight is
	// active so the 3/s drain is never silently offset.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MagicRegenPerSecond = 0.5f;

	// [2026-10-28 T-011] Hard-landing damage tuning (GAMEPLAY.md section 2,
	// HP -5 to -25 scaled by impact, [tuning: PT-STAM]). Impact speeds are
	// in cm/s (UE units): 800 is above a flap-assisted descent (flap sets
	// Z to 650) but below a dive pull-out; 2200 is past MaxFlySpeed 1800
	// so only a committed dive/fall reaches full damage. PT-STAM replaces
	// these numbers; GAMEPLAY.md wants them in DA_DragonStats eventually,
	// until then they live here as tunable component data, not constants
	// buried in pawn code.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float LandingSpeedThreshold = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float LandingSpeedMaxDamage = 2200.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MinLandingDamage = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MaxLandingDamage = 25.0f;

	// [2026-10-27 T-003] Pure stamina math, owner-free on purpose: automation
	// tests exercise these directly without spawning a world.
	// [2026-11-04 T-124] Purity exception: the four spend verbs below
	// (TryConsumeStamina, both DrainForFlight overloads, and the
	// exhaustion pair DrainHPForExhaustion/ShouldForceFallAtZeroStamina)
	// no-op while the utm.testing.infinitestamina cvar is on, so god
	// mode gates every stamina cost/drain in both flight paths (hybrid
	// + legacy) from this one seam. Tests set the cvar directly; HP
	// damage (ApplyHPDamage) is never gated.
	static bool IsInfiniteStaminaEnabled();
	bool TryConsumeStamina(float Cost);
	void ApplyStaminaDelta(float Delta);
	// [2026-11-04 FLIGHT-VIGOR] Signed speed-band flight stamina (owner
	// override of GAMEPLAY.md S2 / FLIGHT-SPIKE.md's no-regen rule):
	// hovering -3/s, slow non-hover flight (<1200) -1/s, cruise band
	// (1200..1800) 0/s, fast flight (>= VigorRechargeSpeed) +3/s. Pure
	// and static so every band edge is headless-testable; the pawn feeds
	// it true airspeed + the precision/near-zero-horizontal hover fact.
	static constexpr float SlowFlightBandMaxSpeed = 1200.0f;
	static constexpr float VigorRechargeSpeed = 1800.0f;
	static float FlightStaminaRatePerSecond(float Airspeed, bool bHovering);
	// [2026-11-04 FLIGHT-VIGOR] Applies a signed rate (negative drains,
	// positive refills) over time through ApplyStaminaDelta, gated by
	// the infinite-stamina cvar like the other stamina verbs. Negative
	// time applies nothing. DrainForFlight below stays the fire verb.
	void ApplyFlightStaminaRate(float DeltaSeconds, float SignedRatePerSecond);
	void DrainForFlight(float DeltaSeconds);
	// [2026-11-03 T-062] Drain at a caller-chosen rate (hover positive;
	// glide/dive pass 0 and are free), which also means no exhaustion burn.
	void DrainForFlight(float DeltaSeconds, float RatePerSecond);
	void RegenOnGround(float DeltaSeconds);
	bool CanFlap() const;

	// [2026-10-28 T-011] HP math, same owner-free contract as stamina:
	// negative damage is ignored (healing is a separate verb, RestoreHP
	// below) and the result is always clamped to [0, MaxHP].
	void ApplyHPDamage(float Damage);
	void DrainHPForExhaustion(float DeltaSeconds);

	// [2026-11-04 T-074] Fired on every effective HP drop (damage > 0
	// that actually lowers HP) and exactly once per drop to 0 HP. The
	// defeat latch re-arms when RestoreHP lifts HP above 0, so a healed
	// dragon can be defeated again. GameFlow hook: ADragonCharacter's
	// outcome audit already maps HP <= 0 to EGameFlowOutcome::Lost /
	// slink-away (T-016/T-072); OnDragonDefeated is the event-driven
	// seam for presentation (T-068) to bind without polling.
	FOnDragonHit OnDragonHit;
	FOnDragonDefeated OnDragonDefeated;
	bool IsDefeated() const
	{
		return HP <= 0.0f;
	}

	// [2026-10-28 T-015] The heal verb: adds Amount to HP, clamped to
	// [0, MaxHP]; a negative Amount is ignored so no caller can deal
	// damage through the heal path (ApplyHPDamage owns damage,
	// symmetrically). Food is the only heal in M1 (docs/GAMEPLAY.md
	// section 2: HP comes back by eating), so the eat channel in
	// UDragonCombatComponent is its only production caller.
	void RestoreHP(float Amount);

	// [2026-11-04 T-080] Magic math, same owner-free contract as
	// stamina/HP. TryConsumeMagic is the refusal verb: a negative
	// cost or a cost above the pool is refused (false, pool
	// untouched) so artifact effects can gate on it exactly like
	// TryConsumeStamina. DrainMagic is the per-second verb
	// true-sight uses: it clamps at 0 and returns whether any magic
	// remains, so the caller shuts the effect off the tick the pool
	// empties instead of running it for free. RegenMagic applies
	// MagicRegenPerSecond over time, clamped at MaxMagic; negative
	// time is free by construction, never a drain in disguise.
	bool TryConsumeMagic(float Cost);
	void ApplyMagicDelta(float Delta);
	bool DrainMagic(float DeltaSeconds, float RatePerSecond);
	void RegenMagic(float DeltaSeconds);
	bool HasMagic() const;

	// [2026-10-28 T-011] Pure: maps an impact speed (cm/s, unsigned — the
	// caller passes abs of vertical velocity) to landing HP damage. Zero
	// at or below LandingSpeedThreshold, MinLandingDamage just above it,
	// then linear up to MaxLandingDamage at LandingSpeedMaxDamage and
	// clamped beyond. Reads only this component's tuning fields.
	float LandingDamageForImpactSpeed(float Speed) const;

	// [2026-10-28 T-036] Exhaustion only ends flight for an action that
	// is actually spending stamina. The pawn classifies the current
	// action as a rate (dive/glide 0, hover positive), and this
	// component owns the zero-stamina consequence for that rate: a
	// positive rate burns HP and forces a fall, while a free glide or dive
	// does neither. Keeping the decision here makes the rule testable
	// without spawning the pawn or a world.
	bool ShouldForceFallAtZeroStamina(float RatePerSecond) const;

  private:
	// [2026-11-04 T-074] Defeat broadcast latch (see OnDragonDefeated).
	bool bDefeatBroadcast = false;
};
