// Copyright Under the Mountain contributors.
// Purpose: pure world-life rules for wildlife food (T-090) and the
// dragon in water (T-091), GAMEPLAY.md S2's food ladder. No
// actors: the wildlife actor and water volumes do not exist yet
// (no deer/boar Meshy asset exists, and T-049 terrain carries no
// water bodies), so this seam is the tested contract those actors
// will call — exactly how AGoatActor's FoodStamina/FoodHP fields
// feed UDragonCombatComponent today. Created 2026-11-04
// (T-090/T-091).
// Delegated owner decisions (both tasks were Gated: owner):
// wildlife is the S2 row as written (+25 stamina / +5 HP / 0
// notoriety, unowned and unwitnessed — eating wildlife NEVER goes
// through UNotorietySubsystem, not even as a +0 call, so no
// witness rule can attach to it later by accident). In water the
// dragon WADES at any depth: fire breath is extinguished while
// grounded in water and vents as steam instead, movement slows
// with depth, and the dragon cannot drown and is never refused
// exit — wading out is always available, so water cannot
// soft-lock or trap the dragon (no swim state exists to fail).

#pragma once

#include "CoreMinimal.h"

// [2026-11-04 T-090] One row of the S2 food ladder. Notoriety is
// points as UNotorietySubsystem counts them; the livestock row's
// points are conditional (see FWorldLifeRules::FoodForKind).
struct UNDERTHEMOUNTAIN_API FFoodValues
{
	float Stamina = 0.0f;
	float HP = 0.0f;
	int32 NotorietyPoints = 0;
	// [2026-11-04 T-090] True only for livestock: S2 charges its
	// +3 solely when witnessed / inside a settlement radius, so a
	// flat points field would lie for the unwitnessed case. The
	// caller adds points through AddLivestockEatPoints only when
	// this is set AND its witness test passes.
	bool bNotorietyConditional = false;
};

// [2026-11-04 T-090] The ladder kinds, in S2's ascending order of
// both restore and notoriety cost: wildlife < livestock < people.
// Person splits on the lair boundary because S2/S4 price the
// same eat at 0 inside (invisible) and the largest gain outside.
enum class EFoodKind : uint8
{
	Wildlife,
	Livestock,
	PersonInsideLair,
	PersonOutsideLair
};

// [2026-11-04 T-091] What the dragon does in water: flies over it
// unaffected, wades shallow, wades deep (slower, still wading —
// the dragon is large and there is no swim verb), or is on dry
// ground entirely.
enum class EWaterBehaviour : uint8
{
	Dry,
	FlyOver,
	WadeShallow,
	WadeDeep
};

// [2026-11-04 T-091] The water decision for one tick of contact.
// bFireAllowed false + bSteam true is the S-decision "fire off,
// steam instead"; bCanDrown is always false by design (see file
// header) and is carried explicitly so no caller invents drowning.
struct UNDERTHEMOUNTAIN_API FWaterDecision
{
	EWaterBehaviour Behaviour = EWaterBehaviour::Dry;
	bool bFireAllowed = true;
	bool bSteam = false;
	bool bCanDrown = false;
	float MoveSpeedMultiplier = 1.0f;
};

// [2026-11-04 T-090/T-091] Namespace-as-struct of static pure
// functions, matching FScentMath/FWorldClock's contract.
struct UNDERTHEMOUNTAIN_API FWorldLifeRules
{
	// [2026-11-04 T-090] Depth (cm of water over the ground the
	// dragon stands on) where shallow wading becomes deep. 200 cm
	// is shin-to-belly on the dragon: past it the drag, not the
	// depth, is the cost. Tuning constant, one edit to retune.
	static constexpr float DeepWadeDepthCm = 200.0f;

	// [2026-11-04 T-090] The dragon distance (cm) at which
	// wildlife switches from graze to flee for the future actor:
	// 1500 cm is just outside fire range (1400, S5), so a careful
	// approach can get one breath off before the herd is already
	// gone — the hunt stays a hunt, not a walk-up.
	static constexpr float WildlifeFleeRadiusCm = 1500.0f;

	// [2026-11-04 T-090] The full S2 ladder, values verbatim from
	// the table: wildlife 25/5/0, livestock 50/10/+3-conditional,
	// person inside 60/15/0, person outside 75/20/+10.
	static FFoodValues FoodForKind(EFoodKind Kind);

	// [2026-11-04 T-090] The cheapest food's row, named because it
	// is the one the future wildlife actor stamps on itself (the
	// AGoatActor FoodStamina/FoodHP pattern).
	static FFoodValues WildlifeFood();

	// [2026-11-04 T-090] Flee or graze for the future wildlife
	// actor's tick: flee at or inside WildlifeFleeRadiusCm of the
	// dragon, graze beyond it. Negative distances are clamped to
	// 0 (on top of the dragon is definitely fleeing distance).
	static bool ShouldWildlifeFlee(float DistanceToDragonCm);

	// [2026-11-04 T-091] The water decision. Flying above water
	// (any depth) is FlyOver: fire allowed, full speed, no steam.
	// Grounded in water always wades: fire extinguished into
	// steam, speed x0.8 shallow / x0.5 deep, never drowning.
	// Depth at or below 0 grounded is Dry (no effect at all).
	static FWaterDecision DecideWaterBehaviour(float WaterDepthCm, bool bIsFlying);
};
