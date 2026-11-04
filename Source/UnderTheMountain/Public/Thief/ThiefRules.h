// Copyright Under the Mountain contributors.
// Purpose: pure thief rules that used to live as scattered literals or
// not at all — enemy attacks on the dragon (T-074), waypoint steering
// (T-089), the star gear/loot table (T-132) and dwarf-party roles +
// entry routing (T-133) — as world-free statics so automation tests
// drive them without a level. Created 2026-11-04 (T-074/T-089/T-132/T-133).
// Invariants: no map coordinates exist outside EntryPoints() (the one
// authored entry table, T-133); every combat number here is a starting
// value tagged [tuning: PT-THIEF] / [tuning: PT-WAVE] for playtest, not
// a balance claim; randomness is injected as Roll01, never rolled here.

#pragma once

#include "CoreMinimal.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"

// [2026-11-04 T-133] How a thief gets into the mountain. Burglars use
// dragon-proof cracks (GAMEPLAY.md S5.1); everyone else uses the grand
// portal / peak entrance (S5.2/S5.3). Plain enum: pure logic, no reflection.
enum class EThiefEntryKind : uint8
{
	Crack,
	GrandPortal,
};

// [2026-11-04 T-133] One authored entry point. EntryPoints() below is
// the ONLY table of these in code (T-133 accept: "zero entry constants
// beyond one authored table"); spawners ask EntryForRole, never literals.
struct UNDERTHEMOUNTAIN_API FThiefEntryPoint
{
	FName Id = NAME_None;
	EThiefEntryKind Kind = EThiefEntryKind::GrandPortal;
	FVector Location = FVector::ZeroVector;
};

// [2026-11-04 T-074] Adventurer/guard attack numbers, GAMEPLAY.md S5.3/S5.4.
// MeleeDamage is S5.3's sword HP -6; CrossbowDamage is S5.3's HP -4.
// Range/tell/cadence have no spec number, so they are delegated starting
// values [tuning: PT-THIEF]: melee range is inside claw range (450) so a
// readable tell can be answered, the tell is long enough to dodge/flank,
// and cadence keeps one adventurer at ~5 HP/s worst case, never a melt.
struct UNDERTHEMOUNTAIN_API FThiefAttackProfile
{
	float MeleeDamage = 6.0f;			 // [tuning: PT-THIEF] S5.3 sword
	float GuardMeleeDamage = 3.0f;		 // [tuning: PT-THIEF] guard is a blocker, not a duelist
	float MeleeRange = 220.0f;			 // [tuning: PT-THIEF] cm, must close past claw reach's inner half
	float CrossbowDamage = 4.0f;		 // [tuning: PT-THIEF] S5.3 projectile
	float CrossbowRange = 1800.0f;		 // [tuning: PT-THIEF] cm, dodgeable in flight per S5.3
	float TellSeconds = 0.45f;			 // [tuning: PT-THIEF] wind-up shown before a melee strike lands
	float CrossbowTellSeconds = 0.6f;	 // [tuning: PT-THIEF] raise/aim before a bolt
	float MeleeCadenceSeconds = 1.2f;	 // [tuning: PT-THIEF] min seconds between melee strikes
	float CrossbowCadenceSeconds = 2.5f; // [tuning: PT-THIEF] min seconds between bolts
	int32 CrossbowMinStars = 3;			 // [tuning: PT-WAVE] S4 gear table: crossbows unlock at 3 stars
};

// [2026-11-04 T-132] One row of GAMEPLAY.md S4 DT_NotorietyGear. Gear is
// a spawn-time snapshot: a mid-wave star change never re-gears a live
// thief (S4 derived rule), so callers read this once at spawn.
struct UNDERTHEMOUNTAIN_API FNotorietyGearTier
{
	float HPMultiplier = 1.0f;
	float SpeedMultiplier = 1.0f;
	bool bSmokePuff = false;
	bool bTrapsLive = false;
	bool bShield = false;
	bool bFireCloak = false;
	bool bCrossbow = false;
	bool bPitAndNet = false;
	bool bHealerSupport = false;
	int32 LootMinGold = 0;
	int32 LootMaxGold = 0;
	float PrizeChance = 0.0f;			   // [tuning: PT-WAVE] 3-star "chance of Prize+ item"
	bool bGuaranteedPrizePerParty = false; // [tuning: PT-WAVE] 5-star row
};

// [2026-11-04 T-133] Snapshot of one dwarf-party member for the re-pick
// rule. The brain/character supplies liveness; this struct stays pure.
struct UNDERTHEMOUNTAIN_API FPartyMemberState
{
	EThiefRole Role = EThiefRole::None;
	bool bAlive = true;
};

struct UNDERTHEMOUNTAIN_API FThiefRules
{
	// --- T-074: enemies hurt the dragon ---
	static const FThiefAttackProfile& AttackProfile();
	static bool CanAttackDragon(EThiefRole Role);
	static float MeleeDamageForRole(EThiefRole Role);

	// [2026-11-04 T-133] Incoming dragon damage after frontal blocks
	// (GAMEPLAY.md S5.2/S5.4): guard halves frontal claw, adventurer
	// shield (2+ stars) negates frontal claw, tail always bypasses.
	static float ResolveIncomingDamage(float Damage, EThiefRole Role, bool bFromFront, bool bIsTailSwipe,
									   int32 Stars);

	// --- T-133: dwarf roles ---
	// Guard is 70 HP, carrier/trapper 45 (S5.2); other roles keep authored HP.
	static float HPForRole(EThiefRole Role, float AuthoredHP);
	// Carrier cap is 3 (S5.2); burglar 1, adventurer 2 (S5.1/S5.3).
	static int32 CarriedCapForRole(EThiefRole Role, int32 AuthoredCap);
	// [2026-11-04 T-133] After the carrier dies, another member picks up
	// the dropped loot after 3 s [tuning: PT-THIEF] (S5.2). Returns the
	// member index to promote (trapper first, guard last — he holds the
	// dragon), or -1 while the carrier lives / the delay has not elapsed.
	static int32 RepickCarrierIndex(const TArray<FPartyMemberState>& Members,
									float SecondsSinceCarrierDeath);
	static constexpr float CarrierRepickDelaySeconds = 3.0f; // [tuning: PT-THIEF] S5.2

	// --- T-089: waypoint-route steering ---
	// [2026-11-04 T-089] Next steering target along an injected waypoint
	// list: waypoints within ArriveRadius are consumed in order
	// (InOutIndex advances), then the final destination. An empty list
	// steers straight at the destination. Pure and actor-free, so
	// villagers (T-053) can reuse it; NavMesh volumes remain editor work.
	static FVector SteerTarget(const TArray<FVector>& Waypoints, const FVector& Current,
							   const FVector& FinalDestination, float ArriveRadius, int32& InOutIndex);
	static int32 NextWaypointIndex(const TArray<FVector>& Waypoints, const FVector& Current,
								   float ArriveRadius, int32 FromIndex);

	// --- T-133: entry routing (the one authored table) ---
	static const TArray<FThiefEntryPoint>& EntryPoints();
	// Burglars get the crack nearest HintLocation; all other roles get
	// the grand portal. Falls back to the portal if no crack exists.
	static FThiefEntryPoint EntryForRole(EThiefRole Role, const FVector& HintLocation);

	// --- T-132: star gear + raider loot ---
	static FNotorietyGearTier GearForStars(int32 Stars);
	static float ScaledHP(float BaseHP, int32 Stars);
	static float ScaledSpeed(float BaseSpeed, int32 Stars);
	// [2026-11-04 T-132] The loot a dead raider drops: gold interpolated
	// in the star tier's range by the injected Roll01 (deterministic in
	// tests, rolled by the caller in game). Registration is separate so
	// the drop can be logged/inspected before it becomes scent truth.
	// Note: hoard *scoring* of a drop is the wave-end rule in GAMEPLAY.md
	// S4; Register only makes the drop registered/scent-visible.
	static FTreasureEntry MakeRaiderLootDrop(FName ThiefId, int32 Stars, const FVector& Location,
											 float Roll01);
	static FName RegisterRaiderLootDrop(UTreasureRegistry* Registry, const FTreasureEntry& Drop);
};
