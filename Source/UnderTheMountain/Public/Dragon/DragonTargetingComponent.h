// Copyright Under the Mountain contributors.
// Purpose: smart lock-on (T-069) — one "current target" the camera,
// HUD, pickup (T-070) and combat verbs can share, instead of Eat's
// private nearest-scan and fire's private cone being the only
// notions of a target in the game. Created 2026-11-04 (T-069).
// Invariants: the pure seam (SelectTargetIndex / ShouldBreakLock /
// CategoryPriority) never touches a world, an actor, or a clock —
// candidates arrive as plain data, exactly like the combat
// component's parallel arrays, so automation decides every rule
// headless. The component half gathers at TargetRefreshSeconds
// cadence from TickComponent (never a per-frame all-actor scan)
// and keeps the last candidate list for cycling.
// [2026-11-04 T-069] Delegated deep-dive decisions (owner delegated
// judgement on this task): SOFT lock with retention — a lock holds
// until it breaks (target gone, line-of-sight lost at a refresh, or
// beyond the break range) rather than re-picking the best target
// every refresh; the one interrupt is a carrying thief appearing,
// which steals the lock, because stopping a theft in progress is
// the game's core verb. Acquisition range 2500 cm, break range is
// 1.25x that (hysteresis so a target at the edge cannot flicker
// the lock). L3 / Tab cycles. HUD shape is NOT decided here: this
// component exposes the lock (CurrentTarget*/HasLock) for a HUD
// task to render, and deliberately draws nothing itself.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DragonTargetingComponent.generated.h"

class AActor;

// [2026-11-04 T-069] What a candidate is. Priority is the category
// order from the task: a thief already carrying loot outranks the
// loot itself, which outranks food (livestock), which outranks a
// bystander villager. Non-carrying thieves are not candidates at
// all — until they carry, combat's area verbs find them fine and a
// lock on one would mostly shout about a thief still sneaking in.
enum class EDragonTargetCategory : uint8
{
	None,
	CarryingThief,
	Treasure,
	Livestock,
	Villager
};

// [2026-11-04 T-069] One gatherable thing, as plain data. Location
// is a snapshot (the registry/actor refresh cadence owns freshness,
// mirroring FTreasureEntry); bHasLineOfSight is computed by the
// gatherer, never by the pure seam, so tests set the flag directly.
// Actor is the in-world half's back-pointer for pickup/combat; the
// pure seam ignores it and headless candidates leave it null.
struct UNDERTHEMOUNTAIN_API FDragonTargetCandidate
{
	FName Id = NAME_None;
	EDragonTargetCategory Category = EDragonTargetCategory::None;
	FVector Location = FVector::ZeroVector;
	bool bHasLineOfSight = true;
	bool bValid = true;
	int32 GoldValue = 0;
	TWeakObjectPtr<AActor> Actor;
};

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UDragonTargetingComponent();

	// [2026-11-04 T-069] Acquisition range (cm). 2500 sits outside
	// every verb range (fire 1400, claw 450) so a lock is acquired
	// on approach, not only once a verb could already hit.
	UPROPERTY(EditAnywhere, Category = "Dragon|Targeting")
	float TargetRange = 2500.0f;

	// [2026-11-04 T-069] Gather cadence (seconds). 0.2 s = 5 Hz:
	// second-scale gameplay (a thief crossing 2500 cm at flee speed
	// takes ~5 s) loses nothing, and the cost is three bounded actor
	// scans five times a second instead of sixty.
	UPROPERTY(EditAnywhere, Category = "Dragon|Targeting")
	float TargetRefreshSeconds = 0.2f;

	// [2026-11-04 T-069] Category rank (lower wins). None ranks last
	// so a category-less candidate can never beat a real one.
	static int32 CategoryPriority(EDragonTargetCategory Category);

	// [2026-11-04 T-069] The pure pick: index of the best candidate
	// for a dragon at Origin, or INDEX_NONE. A candidate is eligible
	// only while valid, in line-of-sight, and within MaxRange of
	// Origin (range/LOS are inputs, per the task). Ordering:
	// category priority, then nearest, then (exact distance ties)
	// highest GoldValue, then Id — total and deterministic, so two
	// refreshes over the same world agree.
	static int32 SelectTargetIndex(const TArray<FDragonTargetCandidate>& Candidates,
								   const FVector& Origin, float MaxRange);

	// [2026-11-04 T-069] Lock-break rule, pure: the lock dies when
	// the locked candidate stops being gathered (dead, eaten,
	// picked up, unregistered), when a refresh finds no line of
	// sight to it, or when it leaves BreakRange. LOS breaking at a
	// refresh (not per frame) is the grace period — a thief passing
	// behind one pillar for a frame keeps the lock.
	static bool ShouldBreakLock(bool bCandidateStillPresent, bool bHasLineOfSight, float Distance,
								float BreakRange);

	// [2026-11-04 T-069] One selection refresh over an injected
	// candidate list: keeps a live lock (see header decision),
	// lets a carrying thief steal it, otherwise re-picks via
	// SelectTargetIndex. Returns true while anything is locked.
	// This is the seam the timer gatherer calls and tests drive.
	bool UpdateSelection(const TArray<FDragonTargetCandidate>& Candidates, const FVector& Origin);

	// [2026-11-04 T-069] Cycle to the next eligible candidate (the
	// L3/Tab verb): candidates are ordered exactly as
	// SelectTargetIndex ranks them and the pick wraps. With no
	// lock it acquires the best; with no eligible candidate it
	// clears. Returns true while anything is locked afterwards.
	bool CycleTarget(const TArray<FDragonTargetCandidate>& Candidates, const FVector& Origin);

	void ClearLock();

	bool HasLock() const
	{
		return CurrentCategory != EDragonTargetCategory::None && CurrentId != NAME_None;
	}
	FName CurrentTargetId() const
	{
		return CurrentId;
	}
	EDragonTargetCategory CurrentTargetCategory() const
	{
		return CurrentCategory;
	}
	FVector CurrentTargetLocation() const
	{
		return CurrentLocation;
	}
	AActor* CurrentTargetActor() const
	{
		return CurrentActor.Get();
	}

	// [2026-11-04 T-069] The last gathered list (refreshed at
	// TargetRefreshSeconds cadence), for cycle input and readers
	// that need the field, not just the winner.
	const TArray<FDragonTargetCandidate>& LastCandidates() const
	{
		return CachedCandidates;
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

  private:
	// [2026-11-04 T-069] The in-world gather: thieves (carrying
	// only), treasure actors (resting only), goats (alive), and
	// villagers, each with a visibility trace for the LOS flag.
	// Called only from TickComponent at the refresh cadence.
	void GatherCandidates(TArray<FDragonTargetCandidate>& OutCandidates) const;

	FName CurrentId = NAME_None;
	EDragonTargetCategory CurrentCategory = EDragonTargetCategory::None;
	FVector CurrentLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> CurrentActor;

	TArray<FDragonTargetCandidate> CachedCandidates;
	float RefreshTimer = 0.0f;
};
