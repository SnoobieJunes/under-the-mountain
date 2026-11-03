// Copyright Under the Mountain contributors.
// Purpose: dragon resources (stamina first) behind one component so flight,
// combat and UI never reach into each other. Created 2026-10-27 (T-003).
// [2026-10-28 T-011] Extended with the HP pool, flight-mode drain rates
// (glide/hover/dives are classified by the pawn), exhaustion HP drain and
// the hard-landing damage curve from docs/GAMEPLAY.md section 2.
// Invariants: stamina is clamped to [0, MaxStamina] and HP to
// [0, MaxHP] when written through this component's methods.
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
	// glide 0/s, hover -8/s, dive 0/s (a zero rate is free; glide keeps its
	// field so the rate stays data, dive passes 0 inline and has none).
	// Hover is "flying but barely moving horizontally";
	// the pawn classifies by comparing horizontal speed to this threshold.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float GlideDrainPerSecond = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float HoverDrainPerSecond = 8.0f;

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
	bool TryConsumeStamina(float Cost);
	void ApplyStaminaDelta(float Delta);
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

	// [2026-10-28 T-015] The heal verb: adds Amount to HP, clamped to
	// [0, MaxHP]; a negative Amount is ignored so no caller can deal
	// damage through the heal path (ApplyHPDamage owns damage,
	// symmetrically). Food is the only heal in M1 (docs/GAMEPLAY.md
	// section 2: HP comes back by eating), so the eat channel in
	// UDragonCombatComponent is its only production caller.
	void RestoreHP(float Amount);

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
};
