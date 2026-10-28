// Copyright Under the Mountain contributors.
// Purpose: the dragon's nose. Each tick it reads UTreasureRegistry
// (via UTreasureSubsystem) and maintains one AScentRibbonActor per
// treasure plus the true-sight markers, applying FScentMath so ribbon
// appearance encodes value and carried state only. Created
// 2026-10-28 (T-013).
// Invariants: ribbons and markers are created only from registry
// entries (true-sight shows treasure, never thieves); at most
// MaxActiveRibbons ribbons exist, keeping the highest gold values;
// true-sight cannot be active while a Seeing Stone entry is carried.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScentComponent.generated.h"

class AScentRibbonActor;
class ATrueSightMarkerActor;
class UTreasureRegistry;
class UTreasureSubsystem;

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UScentComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UScentComponent();

	// [2026-10-28 T-013] Ribbon budget: past this many treasures the
	// lowest-value ribbons are skipped (and the skip is logged once)
	// so a huge hoard cannot spawn unbounded actors. Sorting is by
	// gold value descending, so what is dropped is always the least
	// valuable scent.
	UPROPERTY(EditAnywhere, Category = "Dragon|Scent", meta = (ClampMin = "0"))
	int32 MaxActiveRibbons = 64;

	// [2026-10-28 T-013] docs/GAMEPLAY.md section 3 charges magic for
	// true-sight. Stored as tuning now; the drain itself lands with
	// the magic pool, so nothing reads this yet beyond data.
	UPROPERTY(EditAnywhere, Category = "Dragon|Scent")
	float TrueSightMagicDrainPerSecond = 3.0f;

	// [2026-10-28 T-013] Requests a true-sight state. Activating is
	// refused — state stays off, a warning is logged, false returned —
	// while any registered Seeing Stone entry is carried (stolen), or
	// when no registry is reachable to prove otherwise (fail closed:
	// true-sight without its stone must never half-work).
	// Deactivating always succeeds. Returns whether true-sight is
	// active after the call.
	bool SetTrueSightActive(bool bActive);

	bool IsTrueSightActive() const;

	// [2026-10-28 T-013] Flips true-sight through SetTrueSightActive
	// (so the Seeing Stone refusal applies here too) and returns the
	// resulting state.
	bool ToggleTrueSight();

	// [2026-10-28 T-013] Caches the world's UTreasureSubsystem.
	virtual void BeginPlay() override;

	// [2026-10-28 T-013] Destroys every ribbon and marker this
	// component spawned, so removing the nose removes its visuals.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// [2026-10-28 T-013] Rebuilds/refreshes ribbons and markers from
	// the registry: spawns or reuses one ribbon per (capped) entry
	// from the owner's nose approximation to the entry location,
	// destroys visuals whose entries vanished, and shows markers
	// only while true-sight is active. Fully null-guarded: with no
	// owner, world or subsystem it does nothing, so headless tests of
	// the registry/math never need this component.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

  private:
	// [2026-10-28 T-013] Resolves (and caches) the registry through
	// the world's UTreasureSubsystem; nullptr when any hop is missing.
	UTreasureRegistry* ResolveRegistry();

	// [2026-10-28 T-013] True while any entry is both the Seeing Stone
	// and carried — the stolen-stone state that blocks true-sight.
	static bool IsSeeingStoneCarried(const UTreasureRegistry* Registry);

	void DestroyAllVisuals();

	UPROPERTY()
	TMap<FName, TObjectPtr<AScentRibbonActor>> Ribbons;

	UPROPERTY()
	TMap<FName, TObjectPtr<ATrueSightMarkerActor>> Markers;

	// [2026-10-28 T-013] Cached subsystem handle; re-resolved whenever
	// null so a component that began play before its world had the
	// subsystem still picks it up on a later tick.
	UPROPERTY()
	TObjectPtr<UTreasureSubsystem> CachedSubsystem;

	UPROPERTY(VisibleAnywhere, Category = "Dragon|Scent")
	bool bTrueSightActive = false;

	// [2026-10-28 T-013] One-shot latch so the ribbon-cap warning
	// logs once per overflow episode instead of every tick.
	bool bWarnedRibbonCap = false;

	// [2026-10-28 T-013] Last ribbon count written to the log; -1 so
	// the first tick with any treasure always logs once.
	int32 LastLoggedRibbonCount = -1;
};
