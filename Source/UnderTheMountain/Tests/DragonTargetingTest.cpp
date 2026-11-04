// Copyright Under the Mountain contributors.
// Purpose: T-069 smart-targeting contract tests — the pure
// category-priority/range/LOS selection, lock retention with the
// carrying-thief interrupt, lock-break rules, and cycling — all
// over injected FDragonTargetCandidate data with no world, the
// SelectAnimState precedent. Created 2026-11-04 (T-069).

#include "Misc/AutomationTest.h"
#include "Dragon/DragonTargetingComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
// [2026-11-04 T-069] Candidate factory: valid + in-sight by
// default, tests flip the one flag under examination.
FDragonTargetCandidate MakeCandidate(const TCHAR* Id, EDragonTargetCategory Category,
									 const FVector& Location, int32 GoldValue = 0)
{
	FDragonTargetCandidate Candidate;
	Candidate.Id = FName(Id);
	Candidate.Category = Category;
	Candidate.Location = Location;
	Candidate.GoldValue = GoldValue;
	return Candidate;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonTargetSelectionTest,
								 "UnderTheMountain.Dragon.TargetSelection",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonTargetSelectionTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const float Range = 2500.0f;

	TestTrue(TEXT("carrying thief outranks treasure outranks livestock outranks villager"),
			 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory::CarryingThief) <
					 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory::Treasure) &&
				 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory::Treasure) <
					 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory::Livestock) &&
				 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory::Livestock) <
					 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory::Villager));

	// Priority beats proximity: a far carrying thief wins over a
	// treasure at the dragon's feet; without the thief the nearest
	// of the next category wins.
	{
		const TArray<FDragonTargetCandidate> Candidates = {
			MakeCandidate(TEXT("NearTreasure"), EDragonTargetCategory::Treasure,
						  FVector(100.0f, 0.0f, 0.0f), 500),
			MakeCandidate(TEXT("Goat"), EDragonTargetCategory::Livestock, FVector(50.0f, 0.0f, 0.0f)),
			MakeCandidate(TEXT("Carrier"), EDragonTargetCategory::CarryingThief,
						  FVector(2000.0f, 0.0f, 0.0f)),
		};
		TestEqual(TEXT("carrying thief wins over nearer treasure"),
				  UDragonTargetingComponent::SelectTargetIndex(Candidates, Origin, Range), 2);
		TestEqual(TEXT("without the thief, treasure beats the nearer goat"),
				  UDragonTargetingComponent::SelectTargetIndex(
					  TArray<FDragonTargetCandidate>{Candidates[0], Candidates[1]}, Origin, Range),
				  0);
	}

	// Range and LOS are hard inputs, never advisory.
	{
		FDragonTargetCandidate OutOfRange =
			MakeCandidate(TEXT("Far"), EDragonTargetCategory::Treasure, FVector(2600.0f, 0.0f, 0.0f));
		FDragonTargetCandidate NoSight =
			MakeCandidate(TEXT("Hidden"), EDragonTargetCategory::Treasure, FVector(100.0f, 0.0f, 0.0f));
		NoSight.bHasLineOfSight = false;
		FDragonTargetCandidate Invalid =
			MakeCandidate(TEXT("Dead"), EDragonTargetCategory::Livestock, FVector(100.0f, 0.0f, 0.0f));
		Invalid.bValid = false;
		const TArray<FDragonTargetCandidate> Candidates = {OutOfRange, NoSight, Invalid};
		TestEqual(TEXT("out-of-range, no-LOS, and invalid candidates are all ineligible"),
				  UDragonTargetingComponent::SelectTargetIndex(Candidates, Origin, Range),
				  INDEX_NONE);
	}

	// Lock retention, interrupt, break, and cycle through the
	// component seam (UpdateSelection stores the candidate list).
	{
		UDragonTargetingComponent* Targeting = NewObject<UDragonTargetingComponent>();
		const TArray<FDragonTargetCandidate> Field = {
			MakeCandidate(TEXT("Goblet"), EDragonTargetCategory::Treasure,
						  FVector(300.0f, 0.0f, 0.0f), 100),
			MakeCandidate(TEXT("Goat"), EDragonTargetCategory::Livestock, FVector(100.0f, 0.0f, 0.0f)),
		};
		TestTrue(TEXT("first refresh locks the treasure (priority over nearer goat)"),
				 Targeting->UpdateSelection(Field, Origin));
		TestEqual(TEXT("locked id is the treasure"), Targeting->CurrentTargetId(),
				  FName(TEXT("Goblet")));

		const TArray<FDragonTargetCandidate> Richer = {
			MakeCandidate(TEXT("Goblet"), EDragonTargetCategory::Treasure,
						  FVector(300.0f, 0.0f, 0.0f), 100),
			MakeCandidate(TEXT("Crown"), EDragonTargetCategory::Treasure, FVector(200.0f, 0.0f, 0.0f),
						  1200),
		};
		TestTrue(TEXT("lock survives a better treasure appearing"),
				 Targeting->UpdateSelection(Richer, Origin));
		TestEqual(TEXT("lock still on the first treasure"), Targeting->CurrentTargetId(),
				  FName(TEXT("Goblet")));

		TArray<FDragonTargetCandidate> Theft = Richer;
		Theft.Add(MakeCandidate(TEXT("Carrier"), EDragonTargetCategory::CarryingThief,
								FVector(900.0f, 0.0f, 0.0f)));
		TestTrue(TEXT("a carrying thief steals the lock"),
				 Targeting->UpdateSelection(Theft, Origin));
		TestEqual(TEXT("lock moved to the carrier"), Targeting->CurrentTargetId(),
				  FName(TEXT("Carrier")));

		TestTrue(TEXT("cycle steps off the carrier to the next ranked candidate"),
				 Targeting->CycleTarget(Theft, Origin));
		TestEqual(TEXT("cycle landed on the crown (nearest treasure)"),
				  Targeting->CurrentTargetId(), FName(TEXT("Crown")));

		TestFalse(TEXT("empty field breaks and clears the lock"),
				  Targeting->UpdateSelection(TArray<FDragonTargetCandidate>{}, Origin));
		TestFalse(TEXT("no lock reads as no lock"), Targeting->HasLock());
	}

	// Break rules, pinned directly.
	TestTrue(TEXT("gone target breaks the lock"),
			 UDragonTargetingComponent::ShouldBreakLock(false, true, 100.0f, 3125.0f));
	TestTrue(TEXT("lost LOS at a refresh breaks the lock"),
			 UDragonTargetingComponent::ShouldBreakLock(true, false, 100.0f, 3125.0f));
	TestTrue(TEXT("beyond break range breaks the lock"),
			 UDragonTargetingComponent::ShouldBreakLock(true, true, 3126.0f, 3125.0f));
	TestFalse(TEXT("present, seen, and inside break range holds"),
			  UDragonTargetingComponent::ShouldBreakLock(true, true, 3125.0f, 3125.0f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
