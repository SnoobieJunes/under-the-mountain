// Copyright Under the Mountain contributors.
// Purpose: the dragon's combat verbs — fire breath, roar, and the eat
// channel (docs/GAMEPLAY.md sections 2 and 5.4) — behind one component
// the pawn drives, with every stamina/HP change routed through
// UDragonStatsComponent so its pools keep exactly one writer.
// Created 2026-10-28 (T-015).
// Invariants: this component never ticks itself
// (PrimaryComponentTick.bCanEverTick = false, mirroring
// UDragonStatsComponent): the pawn calls TickFire/TickEat each frame
// and hands in the parallel brains/locations arrays it gathered
// in-world — that injected seam is what lets automation tests drive
// combat with no world spawned. Fire is fully on (bFiring, set only
// by a successful TryStartFire) or fully off, never partial; at most
// one meal is pending at a time (PendingThief and PendingGoat are
// never both set — starting a meal replaces any previous one).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Thief/ThiefDataAsset.h" // [2026-11-04 T-076] EThiefKind for the claw rules
#include "DragonCombatComponent.generated.h"

class AGoatActor;
class ABurnablePropActor;
class UDragonStatsComponent;
class UThiefBrain;

// [2026-10-28 T-015] Outcome of one TickEat call. AteThief doubles as
// the corpse-consumed signal: UThiefBrain deliberately carries no
// "consumed" flag, so the pawn watches for AteThief and hides or
// removes the corpse actor itself. A plain enum class (not UENUM),
// mirroring EThiefState's no-reflection choice for hermetic logic.
enum class EEatResult : uint8
{
	None,
	InProgress,
	AteThief,
	AteGoat
};

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonCombatComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UDragonCombatComponent();

	// [2026-10-28 T-015] Fire breath tuning (docs/GAMEPLAY.md
	// sections 2 and 5.4). 18 DPS kills a 20 HP burglar in ~1.2 s of
	// held breath; 1400 cm range and a 30 degree half-angle make the
	// cone a committed close-range attack, not a screen-clear. All
	// combat numbers live as fields here so feel work tweaks data,
	// never pawn code.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Fire")
	float FireDps = 18.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Fire")
	float FireRange = 1400.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Fire")
	float FireConeHalfAngleDeg = 30.0f;

	// [2026-10-28 T-015] Sustained breath costs 5 stamina/s — a full
	// pool buys 20 s of fire, so breath discipline, not the pool
	// size, is the limit in a single fight.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Fire")
	float FireCostPerSecond = 5.0f;

	// [2026-10-28 T-015] Minimum stamina to START breathing (10 =
	// two seconds of burn guaranteed). The hysteresis is deliberate:
	// a started breath burns all the way to 0, but a nearly-empty
	// pool cannot sputter out one-frame puffs.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Fire")
	float FireMinStamina = 10.0f;

	// [2026-10-28 T-015] Roar tuning (docs/GAMEPLAY.md section 5.4):
	// 10 stamina, 6 s cooldown, 1200 cm radius. The cooldown — not
	// the cost — is what stops roar from being the answer to every
	// thief, so both gates are enforced in TryRoar, cooldown first
	// (a blocked roar never spends stamina).
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Roar")
	float RoarCost = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Roar")
	float RoarCooldownSeconds = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Roar")
	float RoarRadius = 1200.0f;

	// [2026-11-01 T-068] Wing buffet `[tuning: PT-THIEF]`, priced from
	// [2026-11-01 T-068] precedents: cost between claw (-4) and roar (-10)
	// [2026-11-01 T-068] because it dazes a frontal group without damage;
	// [2026-11-01 T-068] 700 cm / 140 deg arc = wingspan reach, inside roar.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Buffet")
	float WingBuffetCost = 8.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Buffet")
	float WingBuffetRadius = 700.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Buffet")
	float WingBuffetHalfAngleDeg = 70.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Buffet")
	float WingBuffetCooldownSeconds = 3.0f;

	// [2026-11-01 T-068] Tail slam: damage 20 + cost -4 are GAMEPLAY.md
	// [2026-11-01 T-068] section 5.4/section 2 verbatim (tail bypasses the
	// [2026-11-01 T-068] frontal shield block because it arcs behind);
	// [2026-11-01 T-068] 500 cm / 110 deg rear arc `[tuning: PT-THIEF]`.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Tail")
	float TailSlamCost = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Tail")
	float TailSlamDamage = 20.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Tail")
	float TailSlamRadius = 500.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Tail")
	float TailSlamHalfAngleDeg = 55.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Tail")
	float TailSlamCooldownSeconds = 2.0f;

	// [2026-11-04 T-076] Claw attack (GAMEPLAY.md §5.4 + §2 verb
	// table, `[tuning: PT-THIEF]` per T-076's 2026-11-01 decision):
	// 25 damage one-shots a 20 HP burglar, -4 stamina per swing,
	// 450 cm foreleg reach (beyond EatRange 300, far inside fire's
	// 1400), 110° frontal arc (55° half-angle, wider than fire's
	// cone because a swipe sweeps). Cooldown/delay are verb-timing
	// values, see TryClaw.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Claw")
	float ClawDamage = 25.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Claw")
	float ClawCost = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Claw")
	float ClawRange = 450.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Claw")
	float ClawHalfAngleDeg = 55.0f;
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Claw")
	float ClawCooldownSeconds = 0.8f;
	// [2026-11-04 T-076] Damage lands this long after the swing
	// starts. T-068's claw montage + notify do not exist (no claw
	// clip is in the pawn's clip set), so the verb's own timing is
	// the damage window — the pawn ticks TickClaw and the hit is
	// judged against where targets stand when the window opens.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Claw")
	float ClawDamageDelaySeconds = 0.25f;

	// [2026-11-04 T-076] §5.4 target priority for one claw swing:
	// a dwarf carrying loot first (stopping the carrier is the
	// efficient kill), then any carrier, then a dwarf, then
	// anyone. Higher wins; distance breaks ties in TickClaw.
	static int32 ClawTargetPriority(EThiefKind Kind, bool bCarryingLoot);

	// [2026-11-04 T-076] §5.4 frontal block, pure: only an
	// adventurer at gear tier >= 2 blocks, and only when the
	// dragon is in the thief's frontal 120° (60° half-angle —
	// a shield covers the front, not the flank). Tail/flank
	// attacks simply never call this with a frontal geometry.
	static bool IsFrontalClawBlocked(EThiefKind Kind, int32 GearTier, const FVector& ThiefForward,
									 const FVector& ThiefLocation, const FVector& DragonLocation);

	// [2026-11-04 T-076] Starts a claw swing: TryRoar's gating
	// contract (null stats, cooldown, then stamina; blocked =
	// free + Warning). Success pays ClawCost, starts the cooldown,
	// and arms the damage window (ClawDamageDelaySeconds); the
	// hit itself lands in TickClaw. One swing at a time — a press
	// during a pending window is refused by the cooldown.
	bool TryClaw(float CurrentTimeSeconds, UDragonStatsComponent* Stats);

	// [2026-11-04 T-076] True while a swing's damage window has
	// not opened yet (the pawn gathers claw targets only then).
	bool IsClawPending() const
	{
		return ClawTimeRemaining > 0.0f;
	}

	// [2026-11-04 T-076] Advances the swing by Dt; returns 0 until
	// the window opens, then judges the arc ONCE against the
	// targets as they now stand (parallel brains/locations/
	// forwards — the pawn gathers them), hits at most one (the
	// ClawTargetPriority winner, nearest on ties) for ClawDamage,
	// and returns 1. A blocked frontal (IsFrontalClawBlocked on
	// the winner) deals nothing, sets bOutBlocked, and still
	// returns 0 hits — the shield eats the swing, nobody behind
	// it is hit instead. A whiff (nobody in arc) returns 0 with
	// bOutBlocked false.
	int32 TickClaw(float Dt, const FVector& Origin, const FVector& Forward,
				   const TArray<UThiefBrain*>& Brains, const TArray<FVector>& BrainLocations,
				   const TArray<FVector>& BrainForwards, bool& bOutBlocked);

	// [2026-10-28 T-015] Eat tuning (docs/GAMEPLAY.md section 2):
	// a 2 s channel makes eating a decision taken under pressure,
	// not an instant heal. EatRange is enforced by the pawn (it
	// owns world positions and picks the meal); this component is
	// location-free on purpose, so StartEat* trusts that the pawn
	// already checked the dragon is within 300 cm.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Eat")
	float EatChannelSeconds = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Eat")
	float EatRange = 300.0f;

	// [2026-10-28 T-015] Thief-corpse food values (+60 stamina/+15
	// HP, docs/GAMEPLAY.md section 2). They live here because a
	// corpse has no authored food fields of its own; goat values
	// instead live on AGoatActor per placement and are read from
	// the goat at channel completion.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Eat")
	float ThiefFoodStamina = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Eat")
	float ThiefFoodHP = 15.0f;

	// [2026-11-01 T-077] Crush tuning (docs/GAMEPLAY.md section 5.4
	// sets crush at 40). CrushMinImpactSpeed is the anti-farming
	// threshold invented by T-077 and recorded in GAMEPLAY.md: 300
	// cm/s is half the dragon's MaxWalkSpeed (600), so a deliberate
	// charge or a committed descent crushes, while a gentle touchdown
	// (a controlled landing settles far below 300 vertical) or idle
	// body contact never does. CrushRadius is the landing footprint:
	// what is under the dragon when it comes down, in cm.
	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Crush")
	float CrushDamage = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Crush")
	float CrushMinImpactSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Combat|Crush")
	float CrushRadius = 300.0f;

	// [2026-11-01 T-077] Pure crush rule: the damage one impact
	// deals. At or above MinImpactSpeed the full Damage lands; below
	// it, zero — a gentle touchdown must not farm kills, so there is
	// no partial credit. Speed is signed-agnostic (a fall and a
	// charge are both speeds), negative tuning clamps to zero damage
	// rather than healing the victim. Static so tests exercise the
	// shipped rule verbatim.
	static float CrushDamageForImpact(float ImpactSpeed, float Damage, float MinImpactSpeed);

	// [2026-10-28 T-015] Pure cone test: true when Target is within
	// Range of Origin AND the angle between (Target - Origin) and
	// Forward is no more than HalfAngleDeg (the exact edge counts as
	// inside). A zero-length target offset counts as inside — a
	// thief standing in the dragon's mouth is definitely in the
	// breath, whatever the facing. A near-zero Forward aims
	// nowhere, so only the zero-offset case can pass with one.
	// Static and state-free so tests (and the pawn's own target
	// picks) can use it without a component instance.
	static bool IsInFireCone(const FVector& Origin, const FVector& Forward, const FVector& Target,
							 float Range, float HalfAngleDeg);

	// [2026-10-28 T-015] Starts the breath if Stats holds at least
	// FireMinStamina. Returns false — and logs a Warning, never
	// silently — when Stats is null or stamina is short; on success
	// bFiring is set and TickFire does the per-frame work.
	bool TryStartFire(UDragonStatsComponent* Stats);

	// [2026-10-28 T-015] Stops the breath unconditionally (input
	// release, forced landing, pawn teardown all funnel here).
	void StopFire();

	// [2026-10-28 T-015] True between a successful TryStartFire and
	// the stop (explicit, or stamina reaching 0 inside TickFire).
	bool IsFiring() const;

	// [2026-10-28 T-015] One frame of held breath: drains
	// FireCostPerSecond stamina for Dt, then applies FireDps * Dt
	// damage to every live brain inside the fire cone from Origin
	// along Forward, and returns how many brains were damaged this
	// tick. Brains[i] stands at BrainLocations[i] (parallel arrays
	// are the injected seam — the pawn gathers them in-world);
	// null and Dead brains are skipped. Returns 0 when not firing.
	// The tick that empties the stamina pool still deals its damage
	// (that breath was already paid for); firing stops from the
	// next tick. A null Stats logs a Warning and stops the breath:
	// free fire is never the fallback.
	// [2026-11-01 T-078] Goats (and any future damageable with an
	// ApplyGoatDamage-shaped seam) burn through the same cone: the
	// breath does not check what a target is before setting it on
	// fire, only where it stands. Goats ride as a second pair of
	// parallel arrays so the thief path is untouched.
	// [2026-11-01 T-055] Burnable props ride as a third pair: in
	// the cone they accumulate exposure seconds (their own state
	// machine decides ignition), never HP damage.
	int32 TickFire(float Dt, UDragonStatsComponent* Stats, const FVector& Origin,
				   const FVector& Forward, const TArray<UThiefBrain*>& Brains,
				   const TArray<FVector>& BrainLocations, const TArray<AGoatActor*>& Goats,
				   const TArray<FVector>& GoatLocations,
				   const TArray<ABurnablePropActor*>& Burnables,
				   const TArray<FVector>& BurnableLocations);

	// [2026-10-28 T-015] Attempts a roar at game-time
	// CurrentTimeSeconds (injected so tests own the clock — this
	// component reads no world time). Blocked attempts (cooldown
	// running, insufficient stamina, null Stats) return false and
	// log a Warning naming the blocker; a blocked roar spends no
	// stamina and does not move the cooldown. On success the cost
	// is paid, the cooldown starts, and every live brain within
	// RoarRadius of Origin gets OnRoar at its own location (loot
	// drops where that thief stands). Distance is 3D, not a ground
	// projection: a roar from cruising altitude reaches less far
	// along the ground than one at a thief's ear, which is the
	// honest blast geometry.
	bool TryRoar(float CurrentTimeSeconds, UDragonStatsComponent* Stats, const FVector& Origin,
				 const TArray<UThiefBrain*>& Brains, const TArray<FVector>& BrainLocations);

	// [2026-11-01 T-068] Wing buffet: same gating contract as TryRoar
	// [2026-11-01 T-068] (cooldown first, blocked = free + Warning). Every
	// [2026-11-01 T-068] live brain in the frontal arc gets OnWingBuffet
	// [2026-11-01 T-068] (fumble one item, Dazed). Returns hit count via
	// [2026-11-01 T-068] OutNumDazed so the pawn can log/knock back actors.
	bool TryWingBuffet(float CurrentTimeSeconds, UDragonStatsComponent* Stats, const FVector& Origin,
					   const FVector& Forward, const TArray<UThiefBrain*>& Brains,
					   const TArray<FVector>& BrainLocations, int32& OutNumDazed);

	// [2026-11-01 T-068] Tail slam: rear-arc damage (Forward negated).
	// [2026-11-01 T-068] ApplyDamage owns death/loot; this owns gating+arc.
	bool TryTailSlam(float CurrentTimeSeconds, UDragonStatsComponent* Stats, const FVector& Origin,
					 const FVector& Forward, const TArray<UThiefBrain*>& Brains,
					 const TArray<FVector>& BrainLocations, int32& OutNumHit);

	// [2026-11-01 T-068] Pure arc test shared by both verbs (range + half
	// [2026-11-01 T-068] angle in the ground plane; zero offset = inside).
	static bool IsInArc2D(const FVector& Origin, const FVector& Forward, const FVector& Target,
						  float Range, float HalfAngleDeg);

	// [2026-10-28 T-015] Begins the eat channel on a dead thief.
	// Only a non-null brain in EThiefState::Dead qualifies — the
	// dragon eats corpses, it does not swallow thieves mid-flee.
	// Any meal already pending is replaced (one-meal invariant).
	// Returns false (Warning logged) for a null or living brain.
	bool StartEatThief(UThiefBrain* Brain);

	// [2026-10-28 T-015] Begins the eat channel on a goat that has
	// not already been eaten. The goat need not be dead first — a
	// dragon that catches a live goat and holds the channel eats
	// it (AGoatActor::Consume marks it dead as part of eating).
	// Any meal already pending is replaced. Returns false (Warning
	// logged) for a null or already-eaten goat.
	bool StartEatGoat(AGoatActor* Goat);

	// [2026-10-28 T-015] Advances the eat channel by Dt. None when
	// no meal is pending; InProgress while the channel runs (also
	// InProgress, without counting down, when Stats is null — the
	// meal is never consumed into a void). On completion the food
	// is applied exactly once — thief: ThiefFoodStamina/
	// ThiefFoodHP through Stats' verbs, pending cleared (the pawn
	// hides the corpse on seeing AteThief); goat: the goat's own
	// FoodStamina/FoodHP, then AGoatActor::Consume() — the pending
	// pointer is cleared, and the meal kind is returned.
	EEatResult TickEat(float Dt, UDragonStatsComponent* Stats);

	// [2026-10-28 T-015] Abandons any pending meal without applying
	// food: the pawn calls this when damage interrupts the channel
	// (and may call it any time — cancelling nothing is a no-op).
	void CancelEat();

  private:
	// [2026-10-28 T-015] Breath state and the roar clock. LastRoarTime
	// starts at -1000 s so the first roar of a session is never on
	// cooldown at any plausible CurrentTimeSeconds (including 0).
	bool bFiring = false;
	float LastRoarTime = -1000.0f;
	// [2026-11-04 T-076] Claw clock + pending window (see TryClaw).
	float LastClawTime = -1000.0f;
	float ClawTimeRemaining = 0.0f;
	float LastBuffetTime = -1000.0f;   // [2026-11-01 T-068]
	float LastTailSlamTime = -1000.0f; // [2026-11-01 T-068]

	// [2026-10-28 T-015] The pending meal: at most one of these is
	// non-null (StartEat* clears the other). UPROPERTY TObjectPtrs
	// keep the meal reachable for GC across the channel even if the
	// owning pawn drops its own reference mid-bite.
	UPROPERTY()
	TObjectPtr<UThiefBrain> PendingThief;

	UPROPERTY()
	TObjectPtr<AGoatActor> PendingGoat;

	// [2026-10-28 T-015] Seconds left on the channel; meaningful
	// only while a meal is pending, and reset to EatChannelSeconds
	// by every successful StartEat*.
	float EatTimeRemaining = 0.0f;
};
