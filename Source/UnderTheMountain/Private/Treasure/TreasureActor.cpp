// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] ATreasureActor implementation: the bridge between
// authored treasure data and the runtime registry. The actor owns no
// treasure state of its own beyond RegisteredId — value, carried state
// and location truth live in UTreasureRegistry (see its header).

#include "Treasure/TreasureActor.h"
#include "Treasure/TreasureDataAsset.h"
#include "Treasure/TreasureSubsystem.h"
#include "UnderTheMountain.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATreasureActor::ATreasureActor()
{
	// [2026-10-28 T-013] No tick: location changes are pushed to the
	// registry by whoever moves the treasure, not polled here.
	PrimaryActorTick.bCanEverTick = false;

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

	// [2026-10-28 T-013] Id left NAME_None so the registry generates a
	// unique one (two placements of the same asset stay distinct).
	FTreasureEntry Entry;
	Entry.GoldValue = TreasureData->GoldValue;
	Entry.Location = GetActorLocation();
	Entry.bIsSeeingStone = TreasureData->bIsSeeingStone;
	RegisteredId = Subsystem->Register(Entry);
	// [2026-10-28 T-013] One line per placement so a PIE log proves the
	// registry (and therefore scent) actually received this treasure.
	UE_LOG(LogUnderTheMountain, Log, TEXT("Treasure registered: %s value=%d seeingStone=%d as %s"),
		   *TreasureData->DisplayName, Entry.GoldValue, Entry.bIsSeeingStone ? 1 : 0,
		   *RegisteredId.ToString());
}

void ATreasureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RegisteredId != NAME_None)
	{
		// [2026-10-28 T-013] The world/subsystem may already be gone
		// during teardown; guard both hops and let the dying registry
		// take the entry with it in that case.
		if (UWorld* World = GetWorld())
		{
			if (UTreasureSubsystem* Subsystem = World->GetSubsystem<UTreasureSubsystem>())
			{
				Subsystem->Unregister(RegisteredId);
			}
		}
		RegisteredId = NAME_None;
	}

	Super::EndPlay(EndPlayReason);
}
