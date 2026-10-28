// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] ATreasureActor implementation: the bridge between
// authored treasure data and the runtime registry. The actor owns no
// treasure state of its own beyond RegisteredId — value, carried state
// and location truth live in UTreasureRegistry (see its header).
// [2026-10-28 T-038] The one piece of actor-side bookkeeping added
// here is LastRegisteredLocation: the actor ticks and refreshes its
// own registry location when it moves, so location truth in the
// registry cannot go stale at the spawn point.

#include "Treasure/TreasureActor.h"
#include "Treasure/TreasureDataAsset.h"
#include "Treasure/TreasureRegistry.h"
#include "Treasure/TreasureSubsystem.h"
#include "UnderTheMountain.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATreasureActor::ATreasureActor()
{
	// [2026-10-28 T-038] Ticking is ON: the actor now keeps its
	// registry location fresh itself (Tick -> RefreshRegisteredLocation)
	// instead of relying on an external mover to push updates, which
	// left the scent endpoint stale whenever nobody remembered to.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// [2026-10-28 T-013] Engine sphere as the greybox body (same
	// object path the dragon placeholder uses). Guarded on
	// Succeeded() so a missing engine asset degrades to an invisible
	// but still-registered treasure instead of a construction crash.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
}

FName ATreasureActor::RegisterWithRegistry(UTreasureRegistry* Registry)
{
	// [2026-10-28 T-038] Fail explicitly, never silently: a null
	// registry or missing data asset means this treasure would emit
	// no scent and count toward no hoard, so say so and return
	// NAME_None. RegisteredId is left untouched on this path (a
	// fresh actor therefore stays NAME_None).
	if (Registry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("ATreasureActor %s cannot register: null UTreasureRegistry passed."),
			   *GetName());
		return NAME_None;
	}
	if (TreasureData == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("ATreasureActor %s has no TreasureData assigned; it will not register."),
			   *GetName());
		return NAME_None;
	}

	// [2026-10-28 T-013] Id left NAME_None so the registry generates a
	// unique one (two placements of the same asset stay distinct).
	// [2026-10-28 T-038] The location snapshot is the actor's CURRENT
	// location, and it is remembered as LastRegisteredLocation so
	// RefreshRegisteredLocation can tell "moved" from "resting".
	FTreasureEntry Entry;
	Entry.GoldValue = TreasureData->GoldValue;
	Entry.Location = GetActorLocation();
	Entry.bIsSeeingStone = TreasureData->bIsSeeingStone;
	const FName NewId = Registry->Register(Entry);
	if (NewId == NAME_None)
	{
		return NAME_None;
	}
	RegisteredId = NewId;
	LastRegisteredLocation = Entry.Location;
	bHasRegisteredLocation = true;
	return RegisteredId;
}

bool ATreasureActor::RefreshRegisteredLocation(UTreasureRegistry* Registry)
{
	// [2026-10-28 T-038] Nothing registered or nowhere to write: the
	// stored location cannot be called current, so report false.
	if (Registry == nullptr || RegisteredId == NAME_None)
	{
		return false;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (bHasRegisteredLocation && CurrentLocation == LastRegisteredLocation)
	{
		// [2026-10-28 T-038] Resting treasure: the stored location is
		// already current and no redundant registry write is made.
		return true;
	}
	if (!Registry->UpdateLocation(RegisteredId, CurrentLocation))
	{
		return false;
	}
	LastRegisteredLocation = CurrentLocation;
	bHasRegisteredLocation = true;
	return true;
}

void ATreasureActor::BeginPlay()
{
	Super::BeginPlay();

	if (TreasureData == nullptr)
	{
		// [2026-10-28 T-013] Errors are surfaced, never swallowed: a
		// treasure with no definition cannot emit scent or count
		// toward the hoard, and the designer must hear about it.
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("ATreasureActor %s has no TreasureData assigned; it will not register."),
			   *GetName());
		return;
	}

	UTreasureSubsystem* Subsystem =
		GetWorld() != nullptr ? GetWorld()->GetSubsystem<UTreasureSubsystem>() : nullptr;
	if (Subsystem == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("ATreasureActor %s found no UTreasureSubsystem in its world; it will not register."),
			   *GetName());
		return;
	}

	// [2026-10-28 T-038] Re-BeginPlay must not orphan the earlier
	// entry: unregister any previous id from the CURRENT world's
	// subsystem first (a stale id from a dead world simply was not
	// there to remove), then clear our bookkeeping.
	if (RegisteredId != NAME_None)
	{
		Subsystem->Unregister(RegisteredId);
		RegisteredId = NAME_None;
		bHasRegisteredLocation = false;
	}

	// [2026-10-28 T-038] Register via the subsystem's registry through
	// the shared seam, so entry construction and LastRegisteredLocation
	// bookkeeping exist in exactly one place.
	RegisteredId = RegisterWithRegistry(Subsystem->Registry());
	if (RegisteredId == NAME_None)
	{
		// [2026-10-28 T-038] Honest failure: never print the
		// "registered" success line for a treasure the registry
		// refused (e.g. an uninitialised subsystem registry).
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Treasure registration FAILED: %s value=%d seeingStone=%d (actor %s)"),
			   *TreasureData->DisplayName, TreasureData->GoldValue,
			   TreasureData->bIsSeeingStone ? 1 : 0, *GetName());
		RegisteredId = NAME_None;
		bHasRegisteredLocation = false;
		return;
	}
	// [2026-10-28 T-013] One line per placement so a PIE log proves the
	// registry (and therefore scent) actually received this treasure.
	UE_LOG(LogUnderTheMountain, Log, TEXT("Treasure registered: %s value=%d seeingStone=%d as %s"),
		   *TreasureData->DisplayName, TreasureData->GoldValue,
		   TreasureData->bIsSeeingStone ? 1 : 0, *RegisteredId.ToString());
}

void ATreasureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// [2026-10-28 T-038] Keep the registry's location snapshot fresh:
	// a moved or carried treasure actor's scent endpoint follows it
	// instead of staying at the spawn point. Every hop is guarded —
	// an unregistered actor, a missing world, or a torn-down
	// subsystem simply has nothing to refresh.
	if (RegisteredId == NAME_None)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	UTreasureSubsystem* Subsystem = World->GetSubsystem<UTreasureSubsystem>();
	if (Subsystem == nullptr)
	{
		return;
	}
	UTreasureRegistry* Registry = Subsystem->Registry();
	if (Registry == nullptr)
	{
		return;
	}
	RefreshRegisteredLocation(Registry);
}

void ATreasureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RegisteredId != NAME_None)
	{
		// [2026-10-28 T-013] The world/subsystem may already be gone
		// during teardown; guard both hops and let the dying registry
		// take the entry with it in that case.
		// [2026-10-28 T-038] But if a subsystem IS reachable and it
		// reports the entry already gone, that double-bookkeeping is
		// warned about instead of swallowed.
		if (UWorld* World = GetWorld())
		{
			if (UTreasureSubsystem* Subsystem = World->GetSubsystem<UTreasureSubsystem>())
			{
				if (!Subsystem->Unregister(RegisteredId))
				{
					UE_LOG(LogUnderTheMountain, Warning,
						   TEXT("ATreasureActor %s EndPlay: registry entry %s was already gone."),
						   *GetName(), *RegisteredId.ToString());
				}
			}
		}
		RegisteredId = NAME_None;
		bHasRegisteredLocation = false;
	}

	Super::EndPlay(EndPlayReason);
}
