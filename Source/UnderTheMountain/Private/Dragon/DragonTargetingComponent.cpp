// Copyright Under the Mountain contributors.
// [2026-11-04 T-069] UDragonTargetingComponent implementation: the
// pure ranking/break rules plus the cadenced in-world gather. The
// pure half reads nothing but its arguments (SelectAnimState's
// precedent); the gather half is the only world-aware code here.

#include "Dragon/DragonTargetingComponent.h"
#include "UnderTheMountain.h"

#include "Dragon/DragonCharacter.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Livestock/GoatActor.h"
#include "NPC/VillagerActor.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Treasure/TreasureActor.h"
#include "Treasure/TreasureRegistry.h"
#include "Treasure/TreasureSubsystem.h"

UDragonTargetingComponent::UDragonTargetingComponent()
{
	// [2026-11-04 T-069] This component DOES tick — unlike combat/
	// stats, its whole job is a cadence, and TickComponent is the
	// cheapest honest clock for it. The tick body still only
	// gathers every TargetRefreshSeconds; per-frame it is one
	// float add and a comparison.
	PrimaryComponentTick.bCanEverTick = true;
}

int32 UDragonTargetingComponent::CategoryPriority(EDragonTargetCategory Category)
{
	switch (Category)
	{
	case EDragonTargetCategory::CarryingThief:
		return 0;
	case EDragonTargetCategory::Treasure:
		return 1;
	case EDragonTargetCategory::Livestock:
		return 2;
	case EDragonTargetCategory::Villager:
		return 3;
	default:
		return 4;
	}
}

int32 UDragonTargetingComponent::SelectTargetIndex(const TArray<FDragonTargetCandidate>& Candidates,
												   const FVector& Origin, float MaxRange)
{
	int32 Best = INDEX_NONE;
	int32 BestPriority = MAX_int32;
	float BestDistance = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FDragonTargetCandidate& Candidate = Candidates[Index];
		if (!Candidate.bValid || Candidate.Category == EDragonTargetCategory::None ||
			!Candidate.bHasLineOfSight)
		{
			continue;
		}
		const float Distance = FVector::Dist(Origin, Candidate.Location);
		if (Distance > MaxRange)
		{
			continue;
		}
		const int32 Priority = CategoryPriority(Candidate.Category);
		bool bBetter = Priority < BestPriority;
		if (!bBetter && Priority == BestPriority)
		{
			// [2026-11-04 T-069] Same category: nearest wins; an
			// exact distance tie (two treasures on one spot) falls
			// to gold, then Id, so the pick never depends on the
			// gather's iteration order.
			bBetter = Distance < BestDistance - KINDA_SMALL_NUMBER ||
					  (FMath::IsNearlyEqual(Distance, BestDistance) &&
					   (Candidate.GoldValue > Candidates[Best].GoldValue ||
						(Candidate.GoldValue == Candidates[Best].GoldValue &&
						 Candidate.Id.Compare(Candidates[Best].Id) < 0)));
		}
		if (bBetter)
		{
			Best = Index;
			BestPriority = Priority;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool UDragonTargetingComponent::ShouldBreakLock(bool bCandidateStillPresent, bool bHasLineOfSight,
												float Distance, float BreakRange)
{
	return !bCandidateStillPresent || !bHasLineOfSight || Distance > BreakRange;
}

bool UDragonTargetingComponent::UpdateSelection(const TArray<FDragonTargetCandidate>& Candidates,
												const FVector& Origin)
{
	CachedCandidates = Candidates;
	if (HasLock())
	{
		const float BreakRange = TargetRange * 1.25f;
		const FDragonTargetCandidate* Locked = nullptr;
		bool bCarryingThiefPresent = false;
		for (const FDragonTargetCandidate& Candidate : Candidates)
		{
			if (Candidate.Category == EDragonTargetCategory::CarryingThief && Candidate.bValid &&
				Candidate.bHasLineOfSight &&
				FVector::Dist(Origin, Candidate.Location) <= TargetRange)
			{
				bCarryingThiefPresent = true;
			}
			if (Candidate.Id == CurrentId && Candidate.Category == CurrentCategory)
			{
				Locked = &Candidate;
			}
		}
		const bool bBroken = Locked == nullptr || !Locked->bValid ||
							 ShouldBreakLock(true, Locked->bHasLineOfSight,
											 FVector::Dist(Origin, Locked->Location), BreakRange);
		// [2026-11-04 T-069] Retention with one interrupt (header):
		// a live lock survives better treasure appearing, but a
		// carrying thief steals it — that is the theft the player
		// is here to stop. (If the lock IS the carrying thief,
		// bBroken above is the only exit.)
		const bool bStolen =
			bCarryingThiefPresent && CurrentCategory != EDragonTargetCategory::CarryingThief;
		if (!bBroken && !bStolen)
		{
			CurrentLocation = Locked->Location;
			CurrentActor = Locked->Actor;
			return true;
		}
		ClearLock();
	}
	const int32 Best = SelectTargetIndex(Candidates, Origin, TargetRange);
	if (Best == INDEX_NONE)
	{
		return false;
	}
	CurrentId = Candidates[Best].Id;
	CurrentCategory = Candidates[Best].Category;
	CurrentLocation = Candidates[Best].Location;
	CurrentActor = Candidates[Best].Actor;
	return true;
}

bool UDragonTargetingComponent::CycleTarget(const TArray<FDragonTargetCandidate>& Candidates,
											const FVector& Origin, int32 Direction)
{
	CachedCandidates = Candidates;
	// [2026-11-04 T-069] Rank the eligible field once, in the same
	// order SelectTargetIndex would pick them, then step.
	TArray<int32> Ranked;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FDragonTargetCandidate& Candidate = Candidates[Index];
		if (Candidate.bValid && Candidate.Category != EDragonTargetCategory::None &&
			Candidate.bHasLineOfSight && FVector::Dist(Origin, Candidate.Location) <= TargetRange)
		{
			Ranked.Add(Index);
		}
	}
	Ranked.Sort([&Candidates, &Origin](int32 A, int32 B)
				{
					const int32 PriorityA = CategoryPriority(Candidates[A].Category);
					const int32 PriorityB = CategoryPriority(Candidates[B].Category);
					if (PriorityA != PriorityB)
					{
						return PriorityA < PriorityB;
					}
					return FVector::Dist(Origin, Candidates[A].Location) <
						   FVector::Dist(Origin, Candidates[B].Location); });
	if (Ranked.Num() == 0)
	{
		ClearLock();
		return false;
	}
	// [2026-11-04] Prev (Direction < 0, D-pad Left) with no lock starts
	// from the far end of the ranked field, mirroring next-from-none.
	const int32 Step = Direction < 0 ? -1 : 1;
	int32 Next = Step > 0 ? Ranked[0] : Ranked[Ranked.Num() - 1];
	for (int32 Rank = 0; Rank < Ranked.Num(); ++Rank)
	{
		if (Candidates[Ranked[Rank]].Id == CurrentId &&
			Candidates[Ranked[Rank]].Category == CurrentCategory)
		{
			Next = Ranked[(Rank + Step + Ranked.Num()) % Ranked.Num()];
			break;
		}
	}
	CurrentId = Candidates[Next].Id;
	CurrentCategory = Candidates[Next].Category;
	CurrentLocation = Candidates[Next].Location;
	CurrentActor = Candidates[Next].Actor;
	return true;
}

void UDragonTargetingComponent::ClearLock()
{
	CurrentId = NAME_None;
	CurrentCategory = EDragonTargetCategory::None;
	CurrentLocation = FVector::ZeroVector;
	CurrentActor.Reset();
}

void UDragonTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
											  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshTimer -= DeltaTime;
	if (RefreshTimer > 0.0f)
	{
		return;
	}
	RefreshTimer = TargetRefreshSeconds;
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->GetWorld() == nullptr)
	{
		return;
	}
	TArray<FDragonTargetCandidate> Candidates;
	GatherCandidates(Candidates);
	UpdateSelection(Candidates, Owner->GetActorLocation());
}

void UDragonTargetingComponent::GatherCandidates(TArray<FDragonTargetCandidate>& OutCandidates) const
{
	OutCandidates.Reset();
	AActor* Owner = GetOwner();
	UWorld* World = Owner != nullptr ? Owner->GetWorld() : nullptr;
	if (Owner == nullptr || World == nullptr)
	{
		return;
	}
	const FVector Origin = Owner->GetActorLocation();
	auto HasLOS = [World, Owner, Origin](AActor* CandidateActor, const FVector& Location)
	{
		if (CandidateActor == nullptr)
		{
			return true;
		}
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Owner);
		Params.AddIgnoredActor(CandidateActor);
		// [2026-11-04 T-069] Eye height at both ends: a ground-level
		// trace clips every hummock between dragon and target.
		return !World->LineTraceSingleByChannel(
			Hit, Origin + FVector(0.0f, 0.0f, 100.0f), Location + FVector(0.0f, 0.0f, 100.0f),
			ECC_Visibility, Params);
	};

	UTreasureRegistry* Registry = nullptr;
	if (UTreasureSubsystem* Subsystem = World->GetSubsystem<UTreasureSubsystem>())
	{
		Registry = Subsystem->Registry();
	}

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, AThiefCharacter::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		AThiefCharacter* Thief = Cast<AThiefCharacter>(Actor);
		UThiefBrain* Brain = Thief != nullptr ? Thief->GetBrain() : nullptr;
		if (Brain == nullptr || Brain->State() == EThiefState::Dead ||
			Brain->CarriedItems().Num() == 0)
		{
			continue;
		}
		FDragonTargetCandidate Candidate;
		Candidate.Id = Thief->GetFName();
		Candidate.Category = EDragonTargetCategory::CarryingThief;
		Candidate.Location = Thief->GetActorLocation();
		Candidate.bHasLineOfSight = HasLOS(Thief, Candidate.Location);
		Candidate.Actor = Thief;
		OutCandidates.Add(Candidate);
	}

	Actors.Reset();
	UGameplayStatics::GetAllActorsOfClass(World, ATreasureActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		ATreasureActor* Treasure = Cast<ATreasureActor>(Actor);
		if (Treasure == nullptr || Treasure->RegisteredId == NAME_None || Registry == nullptr)
		{
			continue;
		}
		const FTreasureEntry* Entry = Registry->Find(Treasure->RegisteredId);
		// [2026-11-04 T-069] Carried treasure is not a treasure
		// candidate: carried-by-thief rides the thief candidate,
		// carried-by-dragon is already in the dragon's claws.
		if (Entry == nullptr || Entry->bCarried)
		{
			continue;
		}
		FDragonTargetCandidate Candidate;
		Candidate.Id = Treasure->RegisteredId;
		Candidate.Category = EDragonTargetCategory::Treasure;
		Candidate.Location = Treasure->GetActorLocation();
		Candidate.GoldValue = Entry->GoldValue;
		Candidate.bHasLineOfSight = HasLOS(Treasure, Candidate.Location);
		Candidate.Actor = Treasure;
		OutCandidates.Add(Candidate);
	}

	Actors.Reset();
	UGameplayStatics::GetAllActorsOfClass(World, AGoatActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		AGoatActor* Goat = Cast<AGoatActor>(Actor);
		if (Goat == nullptr || Goat->IsDead() || Goat->IsEaten())
		{
			continue;
		}
		FDragonTargetCandidate Candidate;
		Candidate.Id = Goat->GetFName();
		Candidate.Category = EDragonTargetCategory::Livestock;
		Candidate.Location = Goat->GetActorLocation();
		Candidate.bHasLineOfSight = HasLOS(Goat, Candidate.Location);
		Candidate.Actor = Goat;
		OutCandidates.Add(Candidate);
	}

	Actors.Reset();
	UGameplayStatics::GetAllActorsOfClass(World, AVillagerActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		AVillagerActor* Villager = Cast<AVillagerActor>(Actor);
		if (Villager == nullptr)
		{
			continue;
		}
		FDragonTargetCandidate Candidate;
		Candidate.Id = Villager->GetFName();
		Candidate.Category = EDragonTargetCategory::Villager;
		Candidate.Location = Villager->GetActorLocation();
		Candidate.bHasLineOfSight = HasLOS(Villager, Candidate.Location);
		Candidate.Actor = Villager;
		OutCandidates.Add(Candidate);
	}
}
