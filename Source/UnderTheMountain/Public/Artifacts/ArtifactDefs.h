// Copyright Under the Mountain contributors.
// Purpose: [2026-11-04 T-081] code-side artifact catalogue + pure effect
// rules for docs/GAMEPLAY.md S6. The catalogue is the DataAsset-equivalent
// seam until DA_Artifact_* assets are authored in the editor: every S6
// index row is present exactly once, with slot, gold value, scope and
// whether its effect is implemented in code or data-only NAMED.
// Invariants: effect truth comes from UTreasureRegistry entries (a plain
// FTreasureEntry snapshot is enough — no world needed): an artifact whose
// entry is carried out is inactive from that same snapshot, regardless of
// worn state. Implemented effects are pure functions of (active, inputs).
#pragma once
#include "CoreMinimal.h"
#include "Treasure/TreasureRegistry.h"

// [2026-11-04 T-081] Implemented = a pure rule below changes a number.
// DataOnlyNamed = specced in S6, present as treasure/value + slot data,
// deliberately no effect code yet (listed in the catalogue comment).
enum class EArtifactEffectKind : uint8
{
	Implemented,
	DataOnlyNamed
};

struct UNDERTHEMOUNTAIN_API FArtifactDefinition
{
	FName Id = NAME_None;
	FString DisplayName;
	FName Slot = NAME_None; // Head/Neck/Body/Claws/Ring/StoneSocket
	int32 GoldValue = 0;
	FString Scope; // M1 / M4 etc, verbatim from S6
	EArtifactEffectKind Kind = EArtifactEffectKind::DataOnlyNamed;
	FString EffectSummary;
};

// [2026-11-04 T-081] Data-only NAMED items (no silent skips): MagicPendant
// (heal needs the T-080 magic pool), MindControlCrown (M4 domination;
// M1 value/cosmetic only), ElementalRings (M4 breath swap), TreasureSightHelm,
// StrengthArmor, ThiefGlow. They exist in All() with Kind=DataOnlyNamed.
struct UNDERTHEMOUNTAIN_API FArtifactCatalog
{
	static const TArray<FArtifactDefinition>& All();
	static const FArtifactDefinition* Find(FName Id);
	static int32 TotalStartingGold(); // S6 start set sum; S8 says 6700
};

struct UNDERTHEMOUNTAIN_API FArtifactRules
{
	// [2026-11-04 T-081] Registry truth is the switch: carried => off the
	// tick it is carried out; present and not carried => owned (in hoard
	// or worn) => active. Missing entry => inactive (fail closed).
	static bool IsActive(const FTreasureEntry* Entry)
	{
		return Entry != nullptr && !Entry->bCarried;
	}
	static bool IsActiveFor(const UTreasureRegistry* Registry, FName ArtifactId);

	// Seeing-Stone-style: mirrors UScentComponent's stolen-stone refusal.
	static bool CanUseTrueSight(const FTreasureEntry* SeeingStoneEntry)
	{
		return IsActive(SeeingStoneEntry);
	}

	// Hardened Scales: -30% all damage; pit/landing halved again, total
	// reduction capped at 50% (S6), so pit/landing floor is 0.5x.
	static float DamageAfterHardenedScales(float Damage, bool bActive, bool bPitOrLanding);
	// Flame Ring: burn 6 HP/s for 3 s after the cone leaves the target.
	static float FlameRingBurnDps(bool bActive)
	{
		return bActive ? 6.0f : 0.0f;
	}
	static float FlameRingBurnSeconds(bool bActive)
	{
		return bActive ? 3.0f : 0.0f;
	}
	// Ring of Power: +25 max magic, fire damage x1.25.
	static float MaxMagicWithRingOfPower(float Base, bool bActive)
	{
		return bActive ? Base + 25.0f : Base;
	}
	static float FireDamageWithRingOfPower(float Damage, bool bActive)
	{
		return bActive ? Damage * 1.25f : Damage;
	}
	// Enhanced Claws: claw 25 -> 40.
	static float ClawDamageWithEnhancedClaws(float Base, bool bActive)
	{
		return bActive ? 40.0f : Base;
	}

	// [2026-11-04 T-081] Slot exclusivity (S6 slot summary): Head one of
	// Crown/Helm, Claws one of Claws/ThiefGlow, Rings at most 2, others
	// unique by slot (Body allows Scales+Armor stacking per S6).
	static bool CanWearTogether(const TArray<FName>& WornIds, FName CandidateId);
};
