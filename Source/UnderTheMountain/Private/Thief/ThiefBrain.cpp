// Copyright Under the Mountain contributors.
// [2026-10-28 T-014] UThiefBrain implementation: the whole thief state
// machine as deterministic functions of (config, registry, dt,
// location) — no world, no actors, no clock — so
// Tests/ThiefBrainTest.cpp exercises every transition headless.

#include "Thief/ThiefBrain.h"

#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"
#include "UnderTheMountain.h"

namespace
{
// [2026-10-28 T-014] Log spelling of each state, kept next to the one
// transition funnel that uses it.
const TCHAR* ThiefStateName(EThiefState State)
{
	switch (State)
	{
	case EThiefState::Sneak:
		return TEXT("Sneak");
	case EThiefState::Grab:
		return TEXT("Grab");
	case EThiefState::Flee:
		return TEXT("Flee");
	case EThiefState::Scared:
		return TEXT("Scared");
	case EThiefState::Trapped:
		return TEXT("Trapped");
	case EThiefState::Dead:
		return TEXT("Dead");
	default:
		return TEXT("Unknown");
	}
}
} // namespace

void UThiefBrain::Init(const UThiefDataAsset* Config, UTreasureRegistry* Registry, FName InThiefId)
{
	ConfigAsset = Config;
	RegistryObject = Registry;
	ThiefId = InThiefId;

	CurrentState = EThiefState::Sneak;
	CurrentHP = Config != nullptr ? Config->MaxHP : 0.0f;
	Carried.Reset();
	TargetId = NAME_None;
	GrabTimer = 0.0f;
	ScareTimer = 0.0f;
	NumTrapsPlaced = 0;
	bReachedExit = false;
	Destination = ExitLocation;

	if (Config == nullptr || Registry == nullptr)
	{
		// [2026-10-28 T-014] Surfaced, not swallowed: a brain without
		// config/registry will no-op every Tick, and this warning is
		// the evidence of why the thief stands still.
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Thief %s: Init with %s — brain will no-op"), *ThiefId.ToString(),
			   Config == nullptr ? TEXT("null config") : TEXT("null registry"));
	}
}

EThiefState UThiefBrain::State() const
{
	return CurrentState;
}

float UThiefBrain::HP() const
{
	return CurrentHP;
}

const TArray<FName>& UThiefBrain::CarriedItems() const
{
	return Carried;
}

FVector UThiefBrain::CurrentDestination() const
{
	return Destination;
}

void UThiefBrain::SetExitLocation(FVector InExitLocation)
{
	ExitLocation = InExitLocation;
}

FVector UThiefBrain::GetExitLocation() const
{
	return ExitLocation;
}

bool UThiefBrain::ReachedExit() const
{
	return bReachedExit;
}

void UThiefBrain::TransitionTo(EThiefState NewState, const TCHAR* Reason)
{
	if (NewState == CurrentState)
	{
		return;
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s: %s->%s %s"), *ThiefId.ToString(),
		   ThiefStateName(CurrentState), ThiefStateName(NewState),
		   Reason != nullptr ? Reason : TEXT(""));
	CurrentState = NewState;
}

bool UThiefBrain::PickBestTarget(FName& OutId, FVector& OutLocation) const
{
	OutId = NAME_None;
	OutLocation = FVector::ZeroVector;
	if (RegistryObject == nullptr)
	{
		return false;
	}

	// [2026-10-28 T-014] Greed rule: highest GoldValue among uncarried
	// entries wins; ties keep the first found so equal-value picks are
	// at least stable within one registry layout.
	int32 BestValue = -1;
	for (const TPair<FName, FTreasureEntry>& Pair : RegistryObject->Entries())
	{
		const FTreasureEntry& Entry = Pair.Value;
		if (!Entry.bCarried && Entry.GoldValue > BestValue)
		{
			BestValue = Entry.GoldValue;
			OutId = Entry.Id;
			OutLocation = Entry.Location;
		}
	}
	return OutId != NAME_None;
}

void UThiefBrain::DropItem(FName ItemId, const FVector& Location)
{
	if (!Carried.RemoveSingle(ItemId))
	{
		return;
	}
	if (RegistryObject != nullptr)
	{
		RegistryObject->SetCarried(ItemId, false, NAME_None);
		RegistryObject->UpdateLocation(ItemId, Location);
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s: dropped %s at %s"), *ThiefId.ToString(),
		   *ItemId.ToString(), *Location.ToString());
}

void UThiefBrain::DropAllItems(const FVector& Location)
{
	// [2026-10-28 T-014] Copy first: DropItem mutates Carried.
	const TArray<FName> ToDrop = Carried;
	for (const FName& ItemId : ToDrop)
	{
		DropItem(ItemId, Location);
	}
}

void UThiefBrain::Tick(float Dt, const FVector& ThiefLocation)
{
	LastKnownLocation = ThiefLocation;

	if (ConfigAsset == nullptr || RegistryObject == nullptr)
	{
		// [2026-10-28 T-014] Null config/registry: no state change, no
		// crash. Init already logged the reason.
		return;
	}

	// [2026-10-28 T-014] Carried loot rides at the thief's location so
	// scent ribbons follow the steal in progress.
	for (const FName& ItemId : Carried)
	{
		RegistryObject->UpdateLocation(ItemId, ThiefLocation);
	}

	switch (CurrentState)
	{
	case EThiefState::Sneak:
	{
		FName BestId;
		FVector BestLocation;
		if (!PickBestTarget(BestId, BestLocation))
		{
			TargetId = NAME_None;
			Destination = ExitLocation;
			TransitionTo(EThiefState::Flee, TEXT("no uncarried treasure left"));
			break;
		}
		TargetId = BestId;
		Destination = BestLocation;
		if (FVector::Dist2D(ThiefLocation, BestLocation) <= ArriveRadius) // [2026-10-28 T-014] 2D: treasure height (on a pile) must not block a grab
		{
			GrabTimer = 0.0f;
			const FString Reason = FString::Printf(TEXT("target=%s"), *BestId.ToString());
			TransitionTo(EThiefState::Grab, *Reason);
		}
		break;
	}

	case EThiefState::Grab:
	{
		const FTreasureEntry* Target = RegistryObject->Find(TargetId);
		if (Target == nullptr)
		{
			// [2026-10-28 T-014] The treasure vanished mid-grab
			// (unregistered under us): abandon the grab and re-pick on
			// the next Sneak tick instead of grabbing air.
			TargetId = NAME_None;
			TransitionTo(EThiefState::Sneak, TEXT("grab target gone"));
			break;
		}
		Destination = Target->Location;
		GrabTimer += Dt;
		if (GrabTimer < ConfigAsset->GrabTimeSeconds)
		{
			break;
		}

		RegistryObject->SetCarried(TargetId, true, ThiefId);
		RegistryObject->UpdateLocation(TargetId, ThiefLocation);
		if (!Carried.Contains(TargetId))
		{
			Carried.Add(TargetId);
		}
		UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s: grabbed %s (carrying %d)"),
			   *ThiefId.ToString(), *TargetId.ToString(), Carried.Num());

		FName NextId;
		FVector NextLocation;
		const bool bMoreTreasure = PickBestTarget(NextId, NextLocation);
		TargetId = NAME_None;
		if (Carried.Num() >= ConfigAsset->CarriedCap || !bMoreTreasure)
		{
			Destination = ExitLocation;
			TransitionTo(EThiefState::Flee,
						 Carried.Num() >= ConfigAsset->CarriedCap ? TEXT("carried cap reached")
																  : TEXT("hoard empty"));
		}
		else
		{
			TransitionTo(EThiefState::Sneak, TEXT("going back for more"));
		}
		break;
	}

	case EThiefState::Flee:
		Destination = ExitLocation;
		if (FVector::Dist2D(ThiefLocation, ExitLocation) <= ArriveRadius)
		{
			bReachedExit = true;
		}
		break;

	case EThiefState::Scared:
		Destination = ExitLocation;
		ScareTimer -= Dt;
		if (ScareTimer <= 0.0f)
		{
			TransitionTo(EThiefState::Flee, TEXT("roar panic faded"));
		}
		break;

	case EThiefState::Trapped:
	case EThiefState::Dead:
		// [2026-10-28 T-014] No movement: destination is the thief
		// itself, so the character's "walk to destination" stands down.
		Destination = ThiefLocation;
		break;
	}
}

void UThiefBrain::OnRoar(const FVector& ThiefLocation)
{
	LastKnownLocation = ThiefLocation;
	if (CurrentState == EThiefState::Dead || ConfigAsset == nullptr)
	{
		return;
	}

	if (ConfigAsset->bDropsAllLootOnRoar)
	{
		DropAllItems(ThiefLocation);
	}
	else if (Carried.Num() > 0)
	{
		// [2026-10-28 T-014] Hold-fast thieves (dwarves) fumble exactly
		// one item — the first grabbed — and keep the rest.
		DropItem(Carried[0], ThiefLocation);
	}

	// [2026-10-28 T-014] An interrupted grab is abandoned: roaring at a
	// thief mid-Grab leaves the treasure where it lay.
	TargetId = NAME_None;
	GrabTimer = 0.0f;
	ScareTimer = ScaredSeconds;
	Destination = ExitLocation;
	TransitionTo(EThiefState::Scared, TEXT("dragon roared"));
}

void UThiefBrain::OnRoar()
{
	OnRoar(LastKnownLocation);
}

bool UThiefBrain::ApplyDamage(float Damage, const FVector& ThiefLocation)
{
	LastKnownLocation = ThiefLocation;
	if (CurrentState == EThiefState::Dead)
	{
		return false;
	}

	CurrentHP = FMath::Max(0.0f, CurrentHP - FMath::Max(0.0f, Damage));
	if (CurrentHP > 0.0f)
	{
		return false;
	}

	DropAllItems(ThiefLocation);
	TargetId = NAME_None;
	Destination = ThiefLocation;
	TransitionTo(EThiefState::Dead, TEXT("hp exhausted"));
	return true;
}

bool UThiefBrain::ApplyDamage(float Damage)
{
	return ApplyDamage(Damage, LastKnownLocation);
}

int32 UThiefBrain::TrapsPlaced() const
{
	return NumTrapsPlaced;
}

bool UThiefBrain::TryPlaceTrap()
{
	if (ConfigAsset == nullptr || !ConfigAsset->bCanPlaceTraps)
	{
		return false;
	}
	if (CurrentState != EThiefState::Sneak)
	{
		return false;
	}
	if (NumTrapsPlaced >= ConfigAsset->MaxTraps)
	{
		return false;
	}
	++NumTrapsPlaced;
	UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s: placed trap %d/%d"), *ThiefId.ToString(),
		   NumTrapsPlaced, ConfigAsset->MaxTraps);
	return true;
}

float UThiefBrain::TrapSlowFactor() const
{
	// [2026-10-28 T-014] Fallbacks mirror UThiefDataAsset's defaults so
	// a config-less brain still reports a sane trap, never a 0x freeze.
	return ConfigAsset != nullptr ? ConfigAsset->TrapSlowFactor : 0.5f;
}

float UThiefBrain::TrapSlowSeconds() const
{
	return ConfigAsset != nullptr ? ConfigAsset->TrapSlowSeconds : 4.0f;
}
