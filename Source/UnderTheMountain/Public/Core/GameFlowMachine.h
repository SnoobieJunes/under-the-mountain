// Copyright Under the Mountain contributors.
// Purpose: T-072 game-flow state machine — title → playing → paused →
// won/lost, with restart and quit-to-title from every screen so no
// state soft-locks. Pure logic (no world, no widgets): the game mode
// owns one and applies its verdicts (pause, screens, level reload).
// Headless-tested in Tests/GameFlowTest.cpp.
// Created 2026-11-03 (T-072).

#pragma once

#include "CoreMinimal.h"
#include "GameFlowMachine.generated.h"

// [2026-11-03 T-072] Run outcome shown on the end screen. Won = the
// hoard survived the session (T-016 ClassifyOutcome); Lost = it did
// not, or the dragon went down.
UENUM(BlueprintType)
enum class EGameFlowOutcome : uint8
{
	InProgress UMETA(DisplayName = "In progress"),
	Won UMETA(DisplayName = "Hoard defended"),
	Lost UMETA(DisplayName = "Hoard lost"),
};

USTRUCT(BlueprintType)
struct FGameFlowResult
{
	GENERATED_BODY()

	// [2026-11-03 T-072] Snapshot for the end screen: what the run
	// cost and where it ended. Filled by the game mode from the
	// subsystems at session end (T-016's AuditGameLoop already logs
	// exactly these numbers).
	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	EGameFlowOutcome Outcome = EGameFlowOutcome::InProgress;

	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	int64 StartingHoard = 0;

	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	int64 CurrentHoard = 0;

	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	int32 Stars = 0;

	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	int32 FinalWave = 0;
};

UCLASS()
class UNDERTHEMOUNTAIN_API UGameFlowMachine : public UObject
{
	GENERATED_BODY()

  public:
	// [2026-11-03 T-072] States. Title and the end screens are menu
	// states (world paused, UI input); Playing is the only unpaused
	// state. Starts at Title; autostart builds skip straight to
	// Playing (test/headless runs, not human play).
	enum class EState : uint8
	{
		Title,
		Playing,
		Paused,
		Won,
		Lost,
	};

	EState GetState() const
	{
		return State;
	}
	bool IsPlaying() const
	{
		return State == EState::Playing;
	}
	bool IsEnded() const
	{
		return State == EState::Won || State == EState::Lost;
	}
	const FGameFlowResult& GetResult() const
	{
		return Result;
	}

	// [2026-11-03 T-072] Transitions. Each returns whether it fired;
	// illegal transitions are no-ops (false), never soft-locks: every
	// state reaches Title (QuitToTitle) or a fresh run (RestartRun).
	// StartPlay: Title -> Playing (title Start button, autostart).
	bool StartPlay();
	// TogglePause: Playing <-> Paused (Esc / gamepad menu).
	bool TogglePause();
	// RequestEnd: Playing/Paused -> Won/Lost (session outcome).
	bool RequestEnd(const FGameFlowResult& InResult);
	// RestartRun: Won/Lost/Paused -> Playing (caller reloads the
	// level first; the machine only flips once the fresh world asks).
	bool RestartRun();
	// QuitToTitle: legal from every state, always succeeds.
	void QuitToTitle();

  private:
	EState State = EState::Title;
	FGameFlowResult Result;
};
