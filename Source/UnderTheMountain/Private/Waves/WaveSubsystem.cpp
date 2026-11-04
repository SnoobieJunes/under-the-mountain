// Copyright Under the Mountain contributors.
// [2026-11-01 T-016] UWaveSubsystem implementation: the section 4
// table, staggered spawns at the lair entry (west end of C1 -
// thieves walk the chamber chain to the hoard and flee back out
// the way they came), and wave-end resolution.

#include "Waves/WaveSubsystem.h"
#include "UnderTheMountain.h"

#include "Engine/World.h"
#include "Notoriety/NotorietySubsystem.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Thief/ThiefDataAsset.h"
#include "Thief/ThiefRules.h"

void UWaveSubsystem::CompositionForWave(int32 Wave, int32& OutBurglars, int32& OutDwarfParties,
										int32& OutAdventurers)
{
	// [2026-11-01 T-016] docs/GAMEPLAY.md section 4, verbatim.
	// Out-of-range waves clamp to the nearest row (wave 6 is the
	// final; nothing spawns past it).
	switch (FMath::Clamp(Wave, 1, 6))
	{
	case 1:
		OutBurglars = 2;
		OutDwarfParties = 0;
		OutAdventurers = 0;
		break;
	case 2:
		OutBurglars = 2;
		OutDwarfParties = 0;
		OutAdventurers = 1;
		break;
	case 3:
		OutBurglars = 3;
		OutDwarfParties = 1;
		OutAdventurers = 1;
		break;
	case 4:
		OutBurglars = 3;
		OutDwarfParties = 1;
		OutAdventurers = 2;
		break;
	case 5:
		OutBurglars = 4;
		OutDwarfParties = 2;
		OutAdventurers = 2;
		break;
	case 6:
	default:
		OutBurglars = 4;
		OutDwarfParties = 2;
		OutAdventurers = 3;
		break;
	}
}

int32 UWaveSubsystem::TotalActorsForWave(int32 Wave)
{
	int32 Burglars = 0, Parties = 0, Adventurers = 0;
	CompositionForWave(Wave, Burglars, Parties, Adventurers);
	// Dwarf parties are 3 members apiece (spec counts them so).
	return Burglars + Parties * 3 + Adventurers;
}

EWaveState UWaveSubsystem::GetWaveState() const
{
	return State;
}

int32 UWaveSubsystem::GetCurrentWave() const
{
	return CurrentWave;
}

bool UWaveSubsystem::IsSessionOver() const
{
	return State == EWaveState::SessionOver;
}

void UWaveSubsystem::TickWave(float DeltaSeconds)
{
	if (State == EWaveState::SessionOver)
	{
		return;
	}
	StateTimer += DeltaSeconds;
	switch (State)
	{
	case EWaveState::Intermission:
		if (StateTimer >= IntermissionSeconds)
		{
			BeginWave(CurrentWave + 1);
		}
		break;
	case EWaveState::Spawning:
		if (StateTimer >= SpawnGroupIntervalSeconds)
		{
			SpawnNextGroup();
		}
		break;
	case EWaveState::Active:
		if (AreWaveThievesResolved())
		{
			UE_LOG(LogUnderTheMountain, Log, TEXT("Wave %d cleared"), CurrentWave);
			if (CurrentWave >= FinalWave)
			{
				State = EWaveState::SessionOver;
				UE_LOG(LogUnderTheMountain, Log, TEXT("Final wave survived - session scoring due"));
			}
			else
			{
				State = EWaveState::Intermission;
				StateTimer = 0.0f;
			}
		}
		break;
	default:
		break;
	}
}

void UWaveSubsystem::BeginWave(int32 Wave)
{
	CurrentWave = Wave;
	CompositionForWave(Wave, PendingBurglars, PendingDwarfParties, PendingAdventurers);
	WaveThieves.Reset();
	// [2026-11-03 T-104] Fresh wave, fresh missing-thief ledger: a slot
	// that goes missing in this wave must report again even if the same
	// index went missing in an earlier wave.
	ReportedMissingSlots.Reset();
	State = EWaveState::Spawning;
	StateTimer = SpawnGroupIntervalSeconds; // first group immediately
	UE_LOG(LogUnderTheMountain, Log, TEXT("Wave %d incoming (%d actors)"), Wave,
		   TotalActorsForWave(Wave));
}

AThiefCharacter* UWaveSubsystem::SpawnThief(UThiefDataAsset* Data, const FVector& SpawnPoint, EThiefRole Role)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Data == nullptr)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AThiefCharacter* Thief = World->SpawnActorDeferred<AThiefCharacter>(
		AThiefCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, SpawnPoint), nullptr,
		nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Thief == nullptr)
	{
		return nullptr;
	}
	Thief->ThiefData = Data;
	// [2026-11-04 T-133] Exit/entry from the one authored table
	// (FThiefRules::EntryPoints): burglars flee to their crack, the
	// rest to the grand portal. No entry literals live here any more.
	const EThiefRole EffectiveRole = Role != EThiefRole::None ? Role : Data->Role;
	Thief->PartyRole = Role;
	Thief->ExitLocation = FThiefRules::EntryForRole(EffectiveRole, SpawnPoint).Location;
	Thief->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnPoint));
	// [2026-11-04 T-132] Star gear is a spawn snapshot (no mid-wave
	// re-gear, GAMEPLAY.md S4): HP/speed/loot scale from current stars.
	if (UThiefBrain* Brain = Thief->GetBrain())
	{
		int32 Stars = 0;
		if (UNotorietySubsystem* Notoriety = GetWorld()->GetSubsystem<UNotorietySubsystem>())
		{
			Stars = Notoriety->GetStars();
		}
		Brain->ApplyStarGear(Stars);
		Brain->SetRaiderLootRoll(FMath::FRand());
	}
	WaveThieves.Add(Thief);
	return Thief;
}

void UWaveSubsystem::SpawnNextGroup()
{
	StateTimer = 0.0f;
	if (!bDataLoaded)
	{
		bDataLoaded = true;
		BurglarData = LoadObject<UThiefDataAsset>(
			nullptr, TEXT("/Game/Gameplay/Thieves/DA_Thief_Burglar.DA_Thief_Burglar"));
		DwarfData = LoadObject<UThiefDataAsset>(
			nullptr, TEXT("/Game/Gameplay/Thieves/DA_Thief_DwarfParty.DA_Thief_DwarfParty"));
		AdventurerData = LoadObject<UThiefDataAsset>(
			nullptr, TEXT("/Game/Gameplay/Thieves/DA_Thief_Adventurer.DA_Thief_Adventurer"));
	}
	// [2026-11-01 T-016] Groups of at most 3 actors per stagger
	// tick (a dwarf party is one group of 3 by itself).
	int32 Spawned = 0;
	while (Spawned < 3)
	{
		if (PendingDwarfParties > 0)
		{
			--PendingDwarfParties;
			// [2026-11-04 T-133] One party, three roles: carrier,
			// trapper, guard (GAMEPLAY.md S5.2), spawned at the portal.
			static const EThiefRole DwarfRoles[3] = {EThiefRole::Carrier, EThiefRole::Trapper,
													 EThiefRole::Guard};
			const FVector PartyEntry =
				FThiefRules::EntryForRole(EThiefRole::Carrier, FVector::ZeroVector).Location;
			for (int32 Member = 0; Member < 3; ++Member)
			{
				SpawnThief(DwarfData, PartyEntry + FVector(0.0f, (Member - 1) * 120.0f, 0.0f),
						   DwarfRoles[Member]);
			}
			Spawned += 3;
		}
		else if (PendingBurglars > 0)
		{
			--PendingBurglars;
			// [2026-11-04 T-133] Burglars enter by a crack, not the portal.
			const FVector CrackEntry =
				FThiefRules::EntryForRole(EThiefRole::Burglar, FVector::ZeroVector).Location;
			SpawnThief(BurglarData, CrackEntry + FVector(0.0f, Spawned * 120.0f, 0.0f),
					   EThiefRole::Burglar);
			++Spawned;
		}
		else if (PendingAdventurers > 0)
		{
			--PendingAdventurers;
			const FVector PortalEntry =
				FThiefRules::EntryForRole(EThiefRole::Adventurer, FVector::ZeroVector).Location;
			SpawnThief(AdventurerData, PortalEntry + FVector(0.0f, Spawned * 120.0f, 0.0f),
					   EThiefRole::Adventurer);
			++Spawned;
		}
		else
		{
			break;
		}
	}
	if (PendingBurglars <= 0 && PendingDwarfParties <= 0 && PendingAdventurers <= 0)
	{
		State = EWaveState::Active;
		UE_LOG(LogUnderTheMountain, Log, TEXT("Wave %d fully entered (%d thieves)"), CurrentWave,
			   WaveThieves.Num());
	}
}

bool UWaveSubsystem::AreWaveThievesResolved() const
{
	// [2026-11-03 T-104] Resolved = every spawned thief is dead,
	// escaped, or gone. An empty wave (data failed to load) counts
	// as resolved so the machine can never deadlock a session — and a
	// missing individual (null slot where a spawn was recorded, or a
	// live thief with no brain) also counts as resolved for the same
	// reason, but LOUDLY: an Error naming the wave and the slot, once
	// per slot per wave, so a failed spawn or unaccounted despawn can
	// never end a wave silently again.
	int32 Slot = 0;
	for (const TWeakObjectPtr<AThiefCharacter>& Weak : WaveThieves)
	{
		AThiefCharacter* Thief = Weak.Get();
		if (Thief == nullptr)
		{
			if (!ReportedMissingSlots.Contains(Slot))
			{
				UE_LOG(LogUnderTheMountain, Error,
					   TEXT("Wave %d: thief slot %d missing (spawn recorded, actor gone) - counting resolved"),
					   CurrentWave, Slot);
				ReportedMissingSlots.Add(Slot);
			}
			++Slot;
			continue;
		}
		UThiefBrain* Brain = Thief->GetBrain();
		if (Brain == nullptr)
		{
			if (!ReportedMissingSlots.Contains(Slot))
			{
				UE_LOG(LogUnderTheMountain, Error,
					   TEXT("Wave %d: thief %s has no brain - counting resolved"), CurrentWave,
					   *Thief->GetName());
				ReportedMissingSlots.Add(Slot);
			}
			++Slot;
			continue;
		}
		if (Brain->State() != EThiefState::Dead && !Brain->ReachedExit())
		{
			return false;
		}
		++Slot;
	}
	return true;
}
