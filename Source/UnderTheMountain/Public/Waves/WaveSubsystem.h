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
// [2026-11-04 T-131] Between waves the FIncursionScheduler owns
// the rhythm (90-150 s gaps, 1-2 single-thief incursions, 20 s
// warning, 4-minute wave cap); this subsystem only acts on its
// verdicts. [2026-11-04 T-138] Sandbox freezes the whole machine.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Thief/ThiefDataAsset.h"
#include "Waves/IncursionScheduler.h"
#include "WaveSubsystem.generated.h"

class AThiefCharacter;
class UThiefDataAsset;

enum class EWaveState : uint8
{
	Intermission,
	Spawning,
	Active,
	SessionOver,
	// [2026-11-04 T-131] Warning is not a stored state (the machine
	// stays in Intermission while the scheduler's warning window is
	// open — see IsWaveWarningActive); the enumerator exists only
	// as the HUD's word for that window, appended last so existing
	// state values keep their numbers.
	Warning
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

	// [2026-11-04 T-131] Gap length is no longer a fixed property:
	// FIncursionScheduler rolls 90-150 s per gap (the old fixed
	// 25 s IntermissionSeconds is gone — no caller read it but
	// TickWave, and a constant cannot express the spec range).
	UPROPERTY(EditAnywhere, Category = "Waves")
	float SpawnGroupIntervalSeconds = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Waves")
	int32 FinalWave = 6;

	// [2026-11-04 T-138] Sandbox gate: while enabled, TickWave is
	// a no-op (no gaps advance, no incursions, no waves, no cap),
	// mirroring the GameFlow rule at this subsystem's own seam so
	// a direct TickWave call cannot spawn into a sandbox world.
	void SetSandboxEnabled(bool bEnabled);
	// [2026-11-04 T-138] True while sandbox freezes the machine
	// (see SetSandboxEnabled).
	bool IsSandboxEnabled() const;

	// [2026-11-01 T-016] Advance the machine. The pawn calls this
	// every tick; all spawning/timing lives here.
	void TickWave(float DeltaSeconds);

	EWaveState GetWaveState() const;
	int32 GetCurrentWave() const;
	bool IsSessionOver() const;
	// [2026-11-04 T-131] True during the scheduler's 20 s warning
	// window before the next wave (state stays Intermission; this
	// is how the HUD picks its "Warning" word without a state the
	// transition table would have to carry).
	bool IsWaveWarningActive() const;
	// [2026-11-04 T-131] Live incursion thieves (not dead, not at
	// the exit) — the count the scheduler's concurrency cap reads,
	// exposed so tests and logs read the same number.
	int32 CountActiveIncursions() const;

  private:
	void BeginWave(int32 Wave);
	void SpawnNextGroup();
	// [2026-11-04 T-133] Role is per-spawn (dwarf party members differ).
	AThiefCharacter* SpawnThief(UThiefDataAsset* Data, const FVector& EntryPoint,
								EThiefRole Role = EThiefRole::None);
	// [2026-11-04 T-131] Loads the three thief DataAssets once;
	// extracted from SpawnNextGroup so incursion spawns share the
	// same one-time load instead of duplicating the asset paths.
	void EnsureThiefDataLoaded();
	// [2026-11-04 T-131] Spawns one burglar as a gap incursion and
	// tracks it apart from WaveThieves: incursion thieves must not
	// hold a wave open (they belong to the gap, not the wave).
	void SpawnIncursionThief();
	// [2026-11-04 T-131] Wave cap reached: every live wave thief
	// is told to flee (UThiefBrain::ForceFlee); the wave then
	// resolves as they reach the exit, like any other resolution.
	void ForceWaveSurvivorsToFlee();
	bool AreWaveThievesResolved() const;

	EWaveState State = EWaveState::Intermission;
	int32 CurrentWave = 0;
	float StateTimer = 0.0f;
	int32 PendingBurglars = 0;
	int32 PendingDwarfParties = 0;
	int32 PendingAdventurers = 0;

	// [2026-11-04 T-131] The cadence clock for the current gap /
	// wave (see IncursionScheduler.h); bCadenceGapStarted marks a
	// gap whose rolls are drawn, reset when a wave begins so the
	// next intermission draws a fresh gap.
	FIncursionScheduler Cadence;
	bool bCadenceGapStarted = false;
	// [2026-11-04 T-138] See SetSandboxEnabled.
	bool bSandboxEnabled = false;

	UPROPERTY()
	TArray<TWeakObjectPtr<AThiefCharacter>> WaveThieves;

	// [2026-11-04 T-131] Incursion thieves, tracked apart from
	// WaveThieves (see SpawnIncursionThief).
	UPROPERTY()
	TArray<TWeakObjectPtr<AThiefCharacter>> IncursionThieves;

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
	// [2026-11-04 T-131] Records an incursion thief through the
	// same weak-pointer seam as TestRecordWaveThief, so the
	// concurrency count is testable without a spawned world.
	void TestRecordIncursionThief(TWeakObjectPtr<AThiefCharacter> Thief)
	{
		IncursionThieves.Add(Thief);
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
