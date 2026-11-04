// Copyright Under the Mountain contributors.
// Purpose: the thief state machine as pure logic on a plain UObject —
// sneak to the richest treasure, grab it, flee to the exit, react to
// roars/damage, and (for dwarves) place dragon-slowing traps — with no
// world, actor, or clock dependency so automation tests drive it
// directly. Created 2026-10-28 (T-014).
// Invariants: an item is in CarriedItems exactly while the registry
// marks it carried by this thief; HP is clamped to [0, Config.MaxHP]
// once initialised; CurrentDestination is where the owning character
// should walk this tick and changes only in Tick/roar/death handling.

#pragma once

#include "CoreMinimal.h"
#include "Thief/ThiefDataAsset.h" // [2026-11-04 T-076] EThiefKind for the claw accessors below
#include "UObject/Object.h"
#include "Thief/ThiefDataAsset.h"
#include "ThiefBrain.generated.h"

class UTreasureRegistry;
class UDragonStatsComponent;

// [2026-11-04 T-074] Dragon-attack phase for armed thieves. Tell is the
// readable wind-up (counter window); Cooldown is cadence between strikes.
enum class EThiefAttackPhase : uint8
{
	Idle,
	Tell,
	Cooldown,
};

// [2026-10-28 T-014] Thief behaviour states. Trapped is reserved for a
// future immobilising effect: Tick already treats it as "no movement"
// alongside Dead, but nothing transitions into it yet. A plain enum
// class (not UENUM) on purpose — the brain is hermetic C++ exercised by
// tests with no reflection involved, mirroring FTreasureEntry.
enum class EThiefState : uint8
{
	Sneak,
	Grab,
	Flee,
	Scared,
	Trapped,
	Dead,
	// [2026-11-01 T-068] Wing-buffet daze: stands stunned (destination =
	// [2026-11-01 T-068] self) until DazeTimer expires, then Flees.
	Dazed
};

// [2026-10-28 T-014] The brain itself. AThiefCharacter owns one per
// thief, feeds it Tick(dt, location) and walks toward
// CurrentDestination(); everything else — target choice, grab timing,
// roar drops, death — is decided here against UTreasureRegistry.
UCLASS()
class UNDERTHEMOUNTAIN_API UThiefBrain : public UObject
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-014] Distance (cm) at which the thief counts as
	// having arrived — at a treasure in Sneak (starts the grab) and at
	// the exit in Flee (sets ReachedExit).
	UPROPERTY(EditAnywhere, Category = "Thief")
	float ArriveRadius = 150.0f;

	// [2026-10-28 T-014] Seconds a roar keeps the thief Scared (running
	// for the exit, loot already dropped) before plain Flee resumes.
	UPROPERTY(EditAnywhere, Category = "Thief")
	float ScaredSeconds = 6.0f;
	// [2026-11-01 T-068] Buffet daze length `[tuning: PT-THIEF]`.
	float DazedSeconds = 2.5f;

	// [2026-10-28 T-014] Binds config + registry and resets all runtime
	// state (HP from Config.MaxHP, empty hands, Sneak). Either pointer
	// may be null: Tick then no-ops rather than crashing, so a thief
	// spawned before the treasure subsystem exists degrades visibly in
	// the log instead of taking the game down. InId is recorded as the
	// carrier id on every treasure this thief grabs.
	void Init(const UThiefDataAsset* Config, UTreasureRegistry* Registry, FName InThiefId);

	EThiefState State() const;
	float HP() const;
	const TArray<FName>& CarriedItems() const;

	// [2026-11-04 T-076] §5.4 claw inputs, read from the config so
	// combat never parses DisplayName: archetype (burglar default
	// with no config), authored gear tier, and whether the hands
	// are full — the three facts UDragonCombatComponent's claw
	// priority/block rules are written against.
	EThiefKind Kind() const;
	int32 GearTier() const;
	bool IsCarryingLoot() const;

	// [2026-10-28 T-014] Where the owning character should walk now:
	// the target treasure in Sneak/Grab, the exit in Flee/Scared, and
	// the thief's own last location in Trapped/Dead (stand still).
	FVector CurrentDestination() const;

	// [2026-10-28 T-014] The world location Flee/Scared run to.
	void SetExitLocation(FVector InExitLocation);
	FVector GetExitLocation() const;

	// [2026-10-28 T-014] True once a Fleeing thief has come within
	// ArriveRadius of the exit; the character logs the escape and
	// stops ticking. Stays true until the next Init.
	bool ReachedExit() const;

	// [2026-10-28 T-014] Advances the state machine by Dt seconds given
	// the thief's current world location. Sneak re-picks the
	// highest-GoldValue uncarried treasure every tick (so a richer
	// piece registering mid-sneak redirects the thief); with no
	// uncarried treasure left it Flees empty-handed. While carrying,
	// every carried entry's registry location is refreshed to the
	// thief's location so scent follows the loot.
	void Tick(float Dt, const FVector& ThiefLocation);

	// [2026-10-28 T-014] The dragon roared at ThiefLocation: drop loot
	// where the thief stands — everything when
	// Config.bDropsAllLootOnRoar, otherwise exactly one item (the
	// first carried) — then become Scared for ScaredSeconds. A dead
	// thief ignores roars.
	void OnRoar(const FVector& ThiefLocation);
	// [2026-11-04 T-131] Wave cap reached: run for the exit now,
	// keeping whatever is carried (fleeing survivors are the wave
	// resolving, not a scare — no loot drop, unlike OnRoar). Dead
	// thieves and thieves already at the exit ignore it; the brain
	// otherwise drops any grab/scare/daze and heads out, so the
	// owning character walks it home on the next Tick.
	void ForceFlee();

	// [2026-10-28 T-014] Convenience overload for callers without a
	// location handy; drops at the last location Tick reported.
	void OnRoar();

	// [2026-11-01 T-068] Wing buffet landed: fumble one carried item and
	// [2026-11-01 T-068] stand dazed for DazedSeconds, then Flee. Dead
	// [2026-11-01 T-068] thieves and a null config ignore it, like OnRoar.
	void OnWingBuffet(const FVector& ThiefLocation);
	void OnWingBuffet();
	bool IsDazed() const
	{
		return CurrentState == EThiefState::Dazed;
	}

	// [2026-10-28 T-014] Subtracts Damage (negative damage is ignored;
	// healing is a separate, later verb). On reaching 0 HP the thief
	// drops ALL carried loot at ThiefLocation, dies, and returns true;
	// otherwise returns false.
	bool ApplyDamage(float Damage, const FVector& ThiefLocation);
	// [2026-10-28 T-014] Convenience overload dropping loot at the
	// last location Tick reported.
	bool ApplyDamage(float Damage);

	// [2026-10-28 T-014] Trap placement: only a trap-capable config in
	// the Sneak state under its MaxTraps budget may place one. The
	// brain owns the count/budget rule; spawning the ATrapActor is the
	// character's job (it reads the slow tuning via the getters).
	int32 TrapsPlaced() const;
	bool TryPlaceTrap();
	float TrapSlowFactor() const;
	float TrapSlowSeconds() const;

	// [2026-11-04 T-133] Runtime role: starts as Config->Role, party
	// spawns override per member (one shared dwarf asset, three roles).
	// Setting Guard/Carrier/Trapper re-derives HP cap via FThiefRules
	// only before any damage is taken (Init-time use; mid-run calls
	// change the role label/cap but never heal or wound).
	void SetRole(EThiefRole InRole);
	EThiefRole Role() const;
	int32 EffectiveCarriedCap() const;

	// [2026-11-04 T-132] Spawn-time star gear snapshot (GAMEPLAY.md S4:
	// no mid-wave re-gear). Rescales HP from the Init HP by the tier
	// multiplier, records the speed multiplier for the character, and
	// enables the crossbow/raider-loot rules at their tiers.
	void ApplyStarGear(int32 Stars);
	int32 GearStars() const;
	float GearSpeedMultiplier() const;
	// [2026-11-04 T-132] Injected raider-loot roll (caller rolls in
	// game; tests pin it). On death the brain registers the drop.
	void SetRaiderLootRoll(float Roll01);

	// [2026-11-04 T-074] Advances the attack rule by Dt against an
	// injected dragon Stats seam. Armed roles only (FThiefRules::
	// CanAttackDragon), dragon in the territory it is fighting over
	// (Sneak/Grab/Flee states attack; Scared/Dazed/Dead never do —
	// a scared adventurer runs, S5.3). In range: Tell (wind-up) ->
	// strike through Stats->ApplyHPDamage (which fires OnDragonHit /
	// OnDragonDefeated) -> Cooldown. Leaving range cancels the tell.
	// Returns the phase after this tick; a null Stats still runs the
	// timers but lands nothing (surfaced by the phase, never a crash).
	EThiefAttackPhase TickDragonAttack(float Dt, const FVector& ThiefLocation,
									   const FVector& DragonLocation, UDragonStatsComponent* DragonStats);
	EThiefAttackPhase AttackPhase() const;

  private:
	// [2026-10-28 T-014] UPROPERTY TObjectPtrs keep config/registry
	// reachable for GC for exactly the brain's lifetime (raw pointers
	// would let GC collect them under a live thief).
	UPROPERTY()
	TObjectPtr<const UThiefDataAsset> ConfigAsset;

	UPROPERTY()
	TObjectPtr<UTreasureRegistry> RegistryObject;

	FName ThiefId = NAME_None;
	EThiefState CurrentState = EThiefState::Sneak;
	float CurrentHP = 0.0f;
	TArray<FName> Carried;
	FVector Destination = FVector::ZeroVector;
	FVector ExitLocation = FVector::ZeroVector;

	// [2026-10-28 T-014] Last location Tick (or a roar/damage call)
	// reported; the no-location overloads and the Trapped/Dead
	// destination fall back to it.
	FVector LastKnownLocation = FVector::ZeroVector;

	// [2026-10-28 T-014] The treasure being snuck to / grabbed, and the
	// two state timers (grab progress counts up to
	// Config.GrabTimeSeconds; the scare timer counts down to 0).
	FName TargetId = NAME_None;
	float GrabTimer = 0.0f;
	float ScareTimer = 0.0f;
	float DazeTimer = 0.0f; // [2026-11-01 T-068] counts down in Dazed

	int32 NumTrapsPlaced = 0;
	bool bReachedExit = false;

	// [2026-11-04 T-133/T-132/T-074] Role/gear/attack runtime state.
	// BaseHP is the role-derived HP at Init, before the gear multiplier,
	// so ApplyStarGear can rescale without compounding on re-application.
	EThiefRole CurrentRole = EThiefRole::None;
	float BaseHP = 0.0f;
	int32 CurrentGearStars = 0;
	float CurrentSpeedMultiplier = 1.0f;
	float RaiderLootRoll = 0.5f;
	bool bRaiderLootEnabled = false;
	EThiefAttackPhase CurrentAttackPhase = EThiefAttackPhase::Idle;
	float AttackTellTimer = 0.0f;
	float AttackCooldownTimer = 0.0f;
	bool bPendingRangedStrike = false;

	// [2026-10-28 T-014] Single funnel for state changes so every
	// transition is logged at Log verbosity in one format
	// ("Thief <id>: Sneak->Grab target=<id>") and no call site can
	// change state silently.
	void TransitionTo(EThiefState NewState, const TCHAR* Reason);

	// [2026-10-28 T-014] Finds the highest-GoldValue uncarried entry.
	// Returns false when the registry is missing or everything is
	// already carried/gone.
	bool PickBestTarget(FName& OutId, FVector& OutLocation) const;

	// [2026-10-28 T-014] Drops one carried item at Location: registry
	// marked uncarried + relocated, removed from Carried. No-op for an
	// item this thief is not carrying.
	void DropItem(FName ItemId, const FVector& Location);

	// [2026-10-28 T-014] Drops every carried item at Location (death,
	// and roars for drop-all configs).
	void DropAllItems(const FVector& Location);
};
