// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] UScentComponent implementation: the only place
// scent visuals are spawned. Appearance math is FScentMath's, treasure
// truth is UTreasureRegistry's; this component just keeps one ribbon
// and one marker per live registry entry pointed at the right place.
// [2026-10-28 T-038] Lifetime rule: visuals belong to exactly one
// registry/world, so the cached subsystem is revalidated every call
// and any world/registry loss or change destroys the old visuals.

#include "Scent/ScentComponent.h"
#include "Scent/ScentMath.h"
#include "Scent/ScentRibbonActor.h"
#include "Scent/TrueSightMarkerActor.h"
#include "Treasure/TreasureRegistry.h"
#include "Treasure/TreasureSubsystem.h"
#include "UnderTheMountain.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UScentComponent::UScentComponent()
{
	// [2026-10-28 T-013] Ticks because ribbons track moving treasure
	// (carried pieces) and the carried pulse is time-based.
	PrimaryComponentTick.bCanEverTick = true;
}

bool UScentComponent::IsSeeingStoneCarried(const UTreasureRegistry* Registry)
{
	if (Registry == nullptr)
	{
		return false;
	}
	for (const TPair<FName, FTreasureEntry>& Pair : Registry->Entries())
	{
		if (Pair.Value.bIsSeeingStone && Pair.Value.bCarried)
		{
			return true;
		}
	}
	return false;
}

UTreasureRegistry* UScentComponent::ResolveRegistry()
{
	// [2026-10-28 T-038] Validate the cache on EVERY call: world
	// subsystems are per-world, so a cached pointer from a previous
	// PIE/world must never be reused in the current one. With no
	// world there is nothing to resolve from and the stale cache is
	// dropped instead of lingering.
	UWorld* CurrentWorld = GetWorld();
	if (CurrentWorld == nullptr)
	{
		CachedSubsystem = nullptr;
		return nullptr;
	}
	if (CachedSubsystem != nullptr && CachedSubsystem->GetWorld() != CurrentWorld)
	{
		CachedSubsystem = nullptr;
	}
	if (CachedSubsystem == nullptr)
	{
		CachedSubsystem = CurrentWorld->GetSubsystem<UTreasureSubsystem>();
	}
	return CachedSubsystem != nullptr ? CachedSubsystem->Registry() : nullptr;
}

bool UScentComponent::SetTrueSightActive(bool bActive)
{
	if (!bActive)
	{
		bTrueSightActive = false;
		return false;
	}

	// [2026-10-28 T-013] Fail closed: activation needs a registry
	// proving no Seeing Stone entry is currently carried. No registry
	// means no proof, so true-sight stays off and says why.
	UTreasureRegistry* Registry = ResolveRegistry();
	if (Registry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("True-sight refused: no treasure registry reachable to check the Seeing Stone."));
		bTrueSightActive = false;
		return false;
	}
	if (IsSeeingStoneCarried(Registry))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("True-sight refused: the Seeing Stone is carried (stolen)."));
		bTrueSightActive = false;
		return false;
	}

	bTrueSightActive = true;
	return true;
}

bool UScentComponent::IsTrueSightActive() const
{
	return bTrueSightActive;
}

bool UScentComponent::ToggleTrueSight()
{
	return SetTrueSightActive(!bTrueSightActive);
}

bool UScentComponent::SetScentVisible(bool bVisible)
{
	bScentVisible = bVisible;
	ApplyScentVisibility();
	return bScentVisible;
}

bool UScentComponent::IsScentVisible() const
{
	return bScentVisible;
}

bool UScentComponent::ToggleScentVisible()
{
	return SetScentVisible(!bScentVisible);
}

void UScentComponent::ApplyScentVisibility()
{
	// [2026-11-03 T-061] Hide/show only — the ribbon map keeps every
	// entry, so toggling never respawns and the registry/scent math
	// path is untouched. Headless-safe: an empty map is a no-op.
	for (TPair<FName, TObjectPtr<AScentRibbonActor>>& Pair : Ribbons)
	{
		if (IsValid(Pair.Value.Get()))
		{
			Pair.Value->SetActorHiddenInGame(!bScentVisible);
		}
	}
}

void UScentComponent::BeginPlay()
{
	Super::BeginPlay();
	// [2026-10-28 T-013] Prime the cache; ResolveRegistry also
	// re-resolves lazily, so a miss here is not fatal.
	ResolveRegistry();
}

void UScentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAllVisuals();
	// [2026-10-28 T-038] Drop the (per-world) subsystem cache and
	// reset the transition-log state, so a later BeginPlay in a new
	// world cannot resolve through the dead world's subsystem and
	// logs its first ribbon count again.
	CachedSubsystem = nullptr;
	LastLoggedRibbonCount = -1;
	Super::EndPlay(EndPlayReason);
}

void UScentComponent::DestroyAllVisuals()
{
	// [2026-10-28 T-038] IsValid (not just != nullptr) before Destroy:
	// an actor already pending-kill from world teardown must not be
	// destroyed twice just because our map still points at it.
	for (TPair<FName, TObjectPtr<AScentRibbonActor>>& Pair : Ribbons)
	{
		if (IsValid(Pair.Value.Get()))
		{
			Pair.Value->Destroy();
		}
	}
	Ribbons.Empty();

	for (TPair<FName, TObjectPtr<ATrueSightMarkerActor>>& Pair : Markers)
	{
		if (IsValid(Pair.Value.Get()))
		{
			Pair.Value->Destroy();
		}
	}
	Markers.Empty();
}

void UScentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
									FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	UTreasureRegistry* Registry = ResolveRegistry();
	if (Owner == nullptr || World == nullptr || Registry == nullptr)
	{
		// [2026-10-28 T-013] Nothing to smell with or from; quietly
		// inert (this is the headless/no-world path, not an error).
		// [2026-10-28 T-038] Inert must not mean orphaned: visuals
		// belong to exactly one registry/world, so losing any hop
		// destroys them and resets the transition-log/cap latches,
		// letting a later healthy tick log and warn afresh.
		if (!Ribbons.IsEmpty() || !Markers.IsEmpty())
		{
			DestroyAllVisuals();
		}
		LastSeenRegistry = Registry;
		LastLoggedRibbonCount = -1;
		bWarnedRibbonCap = false;
		return;
	}

	// [2026-10-28 T-038] Registry identity change (new world/PIE
	// session): visuals belong to exactly one registry/world, so
	// destroy everything built from the previous registry before
	// spawning from this one.
	if (Registry != LastSeenRegistry.Get())
	{
		DestroyAllVisuals();
		LastSeenRegistry = Registry;
	}

	// [2026-10-28 T-013] The stone can be stolen while true-sight is
	// already on; enforce the same gate every tick so markers never
	// outlive the right to see them.
	if (bTrueSightActive && IsSeeingStoneCarried(Registry))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("True-sight ended: the Seeing Stone is now carried (stolen)."));
		bTrueSightActive = false;
	}

	// [2026-10-28 T-013] Highest value first so the cap keeps the
	// most valuable scents. Pointers into the map are used only
	// within this tick, before any registry mutation.
	TArray<const FTreasureEntry*> Sorted;
	Sorted.Reserve(Registry->Count());
	for (const TPair<FName, FTreasureEntry>& Pair : Registry->Entries())
	{
		Sorted.Add(&Pair.Value);
	}
	Sorted.Sort([](const FTreasureEntry& A, const FTreasureEntry& B)
				{ return A.GoldValue > B.GoldValue; });

	const int32 Cap = FMath::Max(0, MaxActiveRibbons);
	if (Sorted.Num() > Cap)
	{
		if (!bWarnedRibbonCap)
		{
			// [2026-10-28 T-013] Caps say so in their output: skipped
			// ribbons are a visible budget decision, logged once per
			// overflow episode rather than every tick.
			UE_LOG(LogUnderTheMountain, Warning,
				   TEXT("Scent ribbon cap reached: %d treasures registered, showing top %d by gold value."),
				   Sorted.Num(), Cap);
			bWarnedRibbonCap = true;
		}
		Sorted.SetNum(Cap);
	}
	else
	{
		bWarnedRibbonCap = false;
	}

	// [2026-10-28 T-013] Nose approximation: owner location raised
	// 50 cm. The dragon's real head socket replaces this when the
	// skeletal mesh lands; ribbon math is unaffected either way.
	const FVector NoseLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const float TimeSeconds = World->GetTimeSeconds();

	TSet<FName> DesiredIds;
	DesiredIds.Reserve(Sorted.Num());

	// [2026-10-28 T-013] Log ribbon-count transitions only, so a PIE
	// log proves ribbons exist without spamming every tick.
	if (Sorted.Num() != LastLoggedRibbonCount)
	{
		UE_LOG(LogUnderTheMountain, Log, TEXT("Scent ribbons active: %d (true-sight %s)"), Sorted.Num(),
			   bTrueSightActive ? TEXT("on") : TEXT("off"));
		LastLoggedRibbonCount = Sorted.Num();
	}

	for (const FTreasureEntry* Entry : Sorted)
	{
		DesiredIds.Add(Entry->Id);

		// [2026-10-28 T-013] Appearance comes from value/carried/
		// true-sight only (FScentMath); Entry->Location is used here
		// solely as the ribbon's far endpoint — geometry, not a cue.
		const FScentRibbonParams Params =
			FScentMath::ParamsForEntry(*Entry, TimeSeconds, bTrueSightActive);

		TObjectPtr<AScentRibbonActor>* RibbonSlot = Ribbons.Find(Entry->Id);
		AScentRibbonActor* Ribbon = RibbonSlot != nullptr ? RibbonSlot->Get() : nullptr;
		if (Ribbon == nullptr)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Owner;
			Ribbon = World->SpawnActor<AScentRibbonActor>(AScentRibbonActor::StaticClass(),
														  NoseLocation, FRotator::ZeroRotator,
														  SpawnParams);
			if (Ribbon == nullptr)
			{
				// [2026-10-28 T-038] A failed spawn must be surfaced
				// once (naming the entry) instead of silently retried
				// every tick — and it must not 'continue' past the
				// marker handling below: a missing ribbon does not
				// take this entry's true-sight marker down with it.
				if (!bWarnedRibbonSpawnFailure)
				{
					UE_LOG(LogUnderTheMountain, Warning,
						   TEXT("Scent ribbon spawn FAILED for treasure entry %s; retrying on later ticks."),
						   *Entry->Id.ToString());
					bWarnedRibbonSpawnFailure = true;
				}
			}
			else
			{
				bWarnedRibbonSpawnFailure = false;
				Ribbons.Add(Entry->Id, Ribbon);
				// [2026-11-03 T-061] A ribbon born while the scent layer
				// is hidden starts hidden — otherwise every new treasure
				// would punch a visible hole through the hidden layer.
				Ribbon->SetActorHiddenInGame(!bScentVisible);
			}
		}
		if (Ribbon != nullptr)
		{
			Ribbon->SetEndpoints(NoseLocation, Entry->Location, Params.Width);
			Ribbon->SetOpacityFactor(Params.Opacity);
		}
		// [2026-10-28 T-041] FScentRibbonParams carries no colour: it
		// is baked into NS_ScentRibbon's particle colour at authoring
		// time (tarnished gold, T-031). Per-entry colour variation is
		// not a scent cue, so only width and opacity are pushed above.

		TObjectPtr<ATrueSightMarkerActor>* MarkerSlot = Markers.Find(Entry->Id);
		ATrueSightMarkerActor* Marker = MarkerSlot != nullptr ? MarkerSlot->Get() : nullptr;
		if (Marker == nullptr)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Owner;
			Marker = World->SpawnActor<ATrueSightMarkerActor>(
				ATrueSightMarkerActor::StaticClass(), Entry->Location, FRotator::ZeroRotator,
				SpawnParams);
			if (Marker == nullptr)
			{
				// [2026-10-28 T-038] Same one-shot surfacing as the
				// ribbon path above: log once naming the entry, latch,
				// and let the stale-visual sweep below still run for
				// every other entry (no 'continue' skipping the tick).
				if (!bWarnedMarkerSpawnFailure)
				{
					UE_LOG(LogUnderTheMountain, Warning,
						   TEXT("True-sight marker spawn FAILED for treasure entry %s; retrying on later ticks."),
						   *Entry->Id.ToString());
					bWarnedMarkerSpawnFailure = true;
				}
			}
			else
			{
				bWarnedMarkerSpawnFailure = false;
				Markers.Add(Entry->Id, Marker);
			}
		}
		if (Marker != nullptr)
		{
			Marker->SetActorLocation(Entry->Location);
			Marker->SetMarkerVisible(bTrueSightActive);
		}
	}

	// [2026-10-28 T-013] Entries that vanished (unregistered, or
	// pushed past the cap) lose their visuals this tick.
	for (auto It = Ribbons.CreateIterator(); It; ++It)
	{
		if (!DesiredIds.Contains(It->Key))
		{
			if (It->Value != nullptr)
			{
				It->Value->Destroy();
			}
			It.RemoveCurrent();
		}
	}
	for (auto It = Markers.CreateIterator(); It; ++It)
	{
		if (!DesiredIds.Contains(It->Key))
		{
			if (It->Value != nullptr)
			{
				It->Value->Destroy();
			}
			It.RemoveCurrent();
		}
	}
}
