// Copyright Under the Mountain contributors.
// [2026-11-01 T-016] UWaveSubsystem implementation: the section 4
// table, staggered spawns at the lair entry (west end of C1 -
// thieves walk the chamber chain to the hoard and flee back out
// the way they came), and wave-end resolution.

#include "Waves/WaveSubsystem.h"
#include "UnderTheMountain.h"

#include "Engine/World.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Thief/ThiefDataAsset.h"

namespace
{
// [2026-11-01 T-016] Lair entry: west edge of the entry chamber,
// at floor height. Exit is the same point - thieves leave the
// way they came in (the chamber chain doors are on the y=0 line).
const FVector EntryPoint(-1300.0f, 0.0f, 160.0f);
} // namespace

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
	State = EWaveState::Spawning;
	StateTimer = SpawnGroupIntervalSeconds; // first group immediately
	UE_LOG(LogUnderTheMountain, Log, TEXT("Wave %d incoming (%d actors)"), Wave,
		   TotalActorsForWave(Wave));
}

AThiefCharacter* UWaveSubsystem::SpawnThief(UThiefDataAsset* Data, const FVector& SpawnPoint)
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
	Thief->ExitLocation = EntryPoint;
	Thief->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnPoint));
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
			for (int32 Member = 0; Member < 3; ++Member)
			{
				SpawnThief(DwarfData, EntryPoint + FVector(0.0f, (Member - 1) * 120.0f, 0.0f));
			}
			Spawned += 3;
		}
		else if (PendingBurglars > 0)
		{
			--PendingBurglars;
			SpawnThief(BurglarData, EntryPoint + FVector(0.0f, Spawned * 120.0f, 0.0f));
			++Spawned;
		}
		else if (PendingAdventurers > 0)
		{
			--PendingAdventurers;
			SpawnThief(AdventurerData, EntryPoint + FVector(0.0f, Spawned * 120.0f, 0.0f));
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
	// [2026-11-01 T-016] Resolved = every spawned thief is dead,
	// escaped, or gone. An empty wave (data failed to load) counts
	// as resolved so the machine can never deadlock a session.
	for (const TWeakObjectPtr<AThiefCharacter>& Weak : WaveThieves)
	{
		AThiefCharacter* Thief = Weak.Get();
		if (Thief == nullptr)
		{
			continue;
		}
		UThiefBrain* Brain = Thief->GetBrain();
		if (Brain == nullptr)
		{
			continue;
		}
		if (Brain->State() != EThiefState::Dead && !Brain->ReachedExit())
		{
			return false;
		}
	}
	return true;
}
