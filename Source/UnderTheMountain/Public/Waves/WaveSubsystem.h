// Copyright Under the Mountain contributors.
// Purpose: the wave state machine (T-016, docs/GAMEPLAY.md
// sections 1 and 4). Waves are world pressure, never rails: the
// machine spawns thief parties at the lair entry on a stagger,
// watches them resolve (dead or escaped), and scores the session
// at the final wave - the dragon may ignore all of it and keep
// roaming; thieves keep looting an undefended hoard meanwhile.
// Created 2026-11-01 (T-016).
// Invariants: composition comes only from CompositionForWave
// (the section 4 table); spawning is staggered in small groups,
// never a full wave at once; a wave is cleared exactly when every
// spawned thief is dead or has reached the exit.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Thief/ThiefDataAsset.h"
#include "WaveSubsystem.generated.h"

class AThiefCharacter;
class UThiefDataAsset;

enum class EWaveState : uint8
{
	Intermission,
	Spawning,
	Active,
	SessionOver
};

UCLASS()
class UNDERTHEMOUNTAIN_API UWaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-016] The section 4 composition table (0-1 star
	// baseline; stars gear thieves, they do not change counts).
	// Dwarf parties count as 3 members each in the total.
	static void CompositionForWave(int32 Wave, int32& OutBurglars, int32& OutDwarfParties,
								   int32& OutAdventurers);
	static int32 TotalActorsForWave(int32 Wave);

	// [2026-11-01 T-016] Seconds between waves and between spawn
	// groups (spec: 15-30 s stagger; intermission is the breather
	// the wave table implies between incursions).
	UPROPERTY(EditAnywhere, Category = "Waves")
	float IntermissionSeconds = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Waves")
	float SpawnGroupIntervalSeconds = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Waves")
	int32 FinalWave = 6;

	// [2026-11-01 T-016] Advance the machine. The pawn calls this
	// every tick; all spawning/timing lives here.
	void TickWave(float DeltaSeconds);

	EWaveState GetWaveState() const;
	int32 GetCurrentWave() const;
	bool IsSessionOver() const;

  private:
	void BeginWave(int32 Wave);
	void SpawnNextGroup();
	// [2026-11-04 T-133] Role is per-spawn (dwarf party members differ).
	AThiefCharacter* SpawnThief(UThiefDataAsset* Data, const FVector& EntryPoint,
								EThiefRole Role = EThiefRole::None);
	bool AreWaveThievesResolved() const;

	EWaveState State = EWaveState::Intermission;
	int32 CurrentWave = 0;
	float StateTimer = 0.0f;
	int32 PendingBurglars = 0;
	int32 PendingDwarfParties = 0;
	int32 PendingAdventurers = 0;

	UPROPERTY()
	TArray<TWeakObjectPtr<AThiefCharacter>> WaveThieves;

	// [2026-11-03 T-104] Once-per-slot ledger for the loud missing-thief
	// report in AreWaveThievesResolved. Mutable because resolution is a
	// const query and the ledger is only a reporting cache — it never
	// changes the verdict, and BeginWave clears it for the next wave.
	mutable TSet<int32> ReportedMissingSlots;

#if WITH_DEV_AUTOMATION_TESTS
  public:
	// [2026-11-03 T-104] Test seams (dev builds only; production spawns
	// through SpawnThief, which is why a null slot means a spawn whose
	// actor is gone). A null entry models exactly that: the spawn was
	// recorded, the actor is not there anymore. The parameter is a weak
	// pointer (not a raw pointer) deliberately: converting a raw pointer
	// needs the complete type, and this header only forward-declares the
	// character — callers (which include the full header) convert.
	void TestRecordWaveThief(TWeakObjectPtr<AThiefCharacter> Thief)
	{
		WaveThieves.Add(Thief);
	}
	bool TestAreWaveThievesResolved() const
	{
		return AreWaveThievesResolved();
	}
#endif

	UPROPERTY()
	TObjectPtr<UThiefDataAsset> BurglarData;
	UPROPERTY()
	TObjectPtr<UThiefDataAsset> DwarfData;
	UPROPERTY()
	TObjectPtr<UThiefDataAsset> AdventurerData;
	bool bDataLoaded = false;
};
