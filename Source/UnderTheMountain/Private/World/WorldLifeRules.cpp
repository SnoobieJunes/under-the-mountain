// Copyright Under the Mountain contributors.
// [2026-11-04 T-090/T-091] Pure world-life rules; see
// WorldLifeRules.h for the delegated decisions.

#include "World/WorldLifeRules.h"

FFoodValues FWorldLifeRules::FoodForKind(EFoodKind Kind)
{
	// [2026-11-04 T-090] GAMEPLAY.md S2's DT_FoodValues rows,
	// verbatim. The ordering wildlife < livestock < people in both
	// restore and cost is the design's food pressure, so these
	// numbers must never be "balanced" per-row in isolation.
	FFoodValues Values;
	switch (Kind)
	{
	case EFoodKind::Wildlife:
		Values.Stamina = 25.0f;
		Values.HP = 5.0f;
		Values.NotorietyPoints = 0;
		break;
	case EFoodKind::Livestock:
		Values.Stamina = 50.0f;
		Values.HP = 10.0f;
		// [2026-11-04 T-090] +3 only if witnessed / in settlement
		// radius (S2 + S4); the flag carries the condition so an
		// unwitnessed farm eat is not charged by a flat table read.
		Values.NotorietyPoints = 3;
		Values.bNotorietyConditional = true;
		break;
	case EFoodKind::PersonInsideLair:
		Values.Stamina = 60.0f;
		Values.HP = 15.0f;
		Values.NotorietyPoints = 0;
		break;
	case EFoodKind::PersonOutsideLair:
		Values.Stamina = 75.0f;
		Values.HP = 20.0f;
		Values.NotorietyPoints = 10;
		break;
	}
	return Values;
}

FFoodValues FWorldLifeRules::WildlifeFood()
{
	return FoodForKind(EFoodKind::Wildlife);
}

bool FWorldLifeRules::ShouldWildlifeFlee(float DistanceToDragonCm)
{
	// [2026-11-04 T-090] Inclusive at the radius: at exactly fire
	// range + 100 the animal is already running, so closing the
	// last distance is against a moving target (see header).
	return FMath::Max(0.0f, DistanceToDragonCm) <= WildlifeFleeRadiusCm;
}

FWaterDecision FWorldLifeRules::DecideWaterBehaviour(float WaterDepthCm, bool bIsFlying)
{
	FWaterDecision Decision;
	// [2026-11-04 T-091] Airborne over water is just airborne: no
	// steam without contact, no drag, fire unaffected. This branch
	// is first so a deep lake can never ground-effect a flyover.
	if (bIsFlying)
	{
		Decision.Behaviour = EWaterBehaviour::FlyOver;
		return Decision;
	}
	// [2026-11-04 T-091] No water underfoot: dry rules, full stop.
	if (WaterDepthCm <= 0.0f)
	{
		return Decision;
	}
	// [2026-11-04 T-091] Grounded in water always wades: fire is
	// out and vents as steam for as long as the dragon stands in
	// it, depth only prices the drag. bCanDrown stays false at
	// every depth (header decision) — deep water is slow, never
	// lethal, and wading back out is always permitted.
	const bool bDeep = WaterDepthCm >= DeepWadeDepthCm;
	Decision.Behaviour = bDeep ? EWaterBehaviour::WadeDeep : EWaterBehaviour::WadeShallow;
	Decision.bFireAllowed = false;
	Decision.bSteam = true;
	Decision.MoveSpeedMultiplier = bDeep ? 0.5f : 0.8f;
	return Decision;
}
