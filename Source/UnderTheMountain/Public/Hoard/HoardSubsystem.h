// Copyright Under the Mountain contributors.
// Purpose: the hoard ledger and win/lose classification (T-016,
// docs/GAMEPLAY.md section 8). Starting hoard is computed from
// the treasure registry (never hand-entered, per spec); thefts
// recorded at escape reduce it; classification is a pure function
// of (current hoard, starting hoard, final-wave-survived,
// dragon-down) so AT-HoardThresholds-style boundary tests read
// fractions, not literals.
// Created 2026-11-01 (T-016).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/WorldSubsystem.h"
#include "HoardSubsystem.generated.h"

// [2026-11-01 T-016] Session outcome. InProgress until a wave-end
// scoring or the dragon falls.
enum class EHoardOutcome : uint8
{
	InProgress,
	Win,
	PyrrhicWin,
	LosePlundered,
	LoseDrivenOut
};

// [2026-11-01 T-016] DA_HoardThresholds (section 8 names this
// asset): the tuning surface for win/lose. Authored at
// /Game/Gameplay/Waves/DA_HoardThresholds by
// Scripts/create_waves.py; the subsystem falls back to these
// defaults when the asset is absent (tests, headless runs).
UCLASS(BlueprintType)
class UNDERTHEMOUNTAIN_API UHoardThresholdsAsset : public UDataAsset
{
	GENERATED_BODY()

  public:
	UPROPERTY(EditAnywhere, Category = "Hoard")
	float WinFraction = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Hoard")
	float LoseFraction = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Hoard")
	int32 FinalWave = 6;
};

UCLASS()
class UNDERTHEMOUNTAIN_API UHoardSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-016] The classification, pure: dragon down
	// loses by being driven out regardless of gold; otherwise a
	// final-wave survivor at/above WinFraction wins, at/above
	// LoseFraction is the pyrrhic slink-away, below it plundered.
	// Mid-session (no final scoring yet) only the dragon-down
	// clause fires - a thin hoard mid-run is a warning, not a loss.
	static EHoardOutcome ClassifyOutcome(int64 CurrentHoard, int64 StartingHoard, float WinFraction,
										 float LoseFraction, bool bFinalWaveSurvived,
										 bool bDragonDown);

	// [2026-11-01 T-016] Latch the session start from the live
	// registry total. First call wins; later calls are no-ops so
	// a mid-session registry change cannot move the goalposts.
	void RecordSessionStart(int64 RegistryTotalGold);

	// [2026-11-01 T-016] A thief got out with loot: reduce the
	// hoard by what it carried (registry values, read by the
	// caller at the escape moment).
	void RecordTheft(int64 GoldValue, const TCHAR* ThiefName);

	int64 GetStartingHoard() const;
	int64 GetStolenGold() const;
	int64 GetCurrentHoard(int64 RegistryTotalGold) const;

	const UHoardThresholdsAsset* Thresholds() const;

  private:
	int64 StartingHoard = 0;
	int64 StolenGold = 0;
	bool bSessionStarted = false;

	UPROPERTY()
	TObjectPtr<UHoardThresholdsAsset> ThresholdsAsset;
};
