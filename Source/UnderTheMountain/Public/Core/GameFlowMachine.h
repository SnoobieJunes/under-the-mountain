// Copyright Under the Mountain contributors.
// Purpose: T-072 game-flow state machine — title → playing → paused →
// won/lost, with restart and quit-to-title from every screen so no
// state soft-locks. Pure logic (no world, no widgets): the game mode
// owns one and applies its verdicts (pause, screens, level reload).
// Headless-tested in Tests/GameFlowTest.cpp.
// Created 2026-11-03 (T-072).

#pragma once

#include "CoreMinimal.h"
#include "Hoard/HoardSubsystem.h"
#include "GameFlowMachine.generated.h"

// [2026-11-03 T-072] Run outcome shown on the end screen. Won = the
// hoard survived the session (T-016 ClassifyOutcome); Lost = it did
// not, or the dragon went down.
// [2026-11-04 T-135] PyrrhicWin is its own outcome, never collapsed
// into Won: GAMEPLAY.md section 8's gutted-hoard slink-away (final
// wave survived below the win fraction, at/above the lose fraction)
// reads differently on the end screen and must survive the snapshot.
UENUM(BlueprintType)
enum class EGameFlowOutcome : uint8
{
	InProgress UMETA(DisplayName = "In progress"),
	Won UMETA(DisplayName = "Hoard defended"),
	PyrrhicWin UMETA(DisplayName = "Pyrrhic win - hoard gutted"),
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

	// [2026-11-04 T-135] Resolution score inputs (GAMEPLAY.md
	// section 1: "kills inside/outside"): thief kills priced by
	// lair location, counted once per corpse by the pawn's audit.
	// The end screen presents both counts verbatim; no weighted
	// total is invented here because the spec scores the split.
	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	int32 InsideKills = 0;

	UPROPERTY(VisibleAnywhere, Category = "GameFlow")
	int32 OutsideKills = 0;
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

	// [2026-11-04 T-138] Sandbox flag, title-selectable on the flow
	// widget and settable from the utm.sandbox cvar by the game
	// mode. Deliberately separate from FGameFlowResult: sandbox is
	// a mode the run is launched in, not a result it produces, so
	// StartPlay's fresh-result reset must not clear it.
	void SetSandboxEnabled(bool bEnabled);
	// [2026-11-04 T-138] True while sandbox mode is armed (see
	// SetSandboxEnabled for why the flag lives apart from the result).
	bool IsSandboxEnabled() const;

	// [2026-11-04 T-138] Pure sandbox rule: wave and incursion
	// spawns only happen in a normal run. Split into three
	// predicates (spawns / notoriety / outcome) so each caller
	// gates exactly the verb it owns and a future mode can relax
	// one rule without forking the others.
	static bool ShouldSpawnWavesOrIncursions(bool bSandbox);
	// [2026-11-04 T-138] Pure sandbox rule: notoriety never moves
	// in sandbox, however public the dragon's acts are (the single
	// writer, UNotorietySubsystem, enforces the same rule at its
	// own seam so no caller can route around it).
	static bool ShouldApplyNotorietyChanges(bool bSandbox);
	// [2026-11-04 T-138] Pure sandbox rule: no win/lose audit and
	// no session outcome in sandbox — the run simply continues
	// until the player quits to title.
	static bool ShouldResolveSessionOutcome(bool bSandbox);

	// [2026-11-04 T-135] Maps the hoard classifier's outcome onto
	// the flow outcome one-to-one: PyrrhicWin stays PyrrhicWin
	// (never Won), both lose flavours collapse to Lost (the end
	// screen's hoard/HP numbers tell which), InProgress stays
	// InProgress. This is the single mapping site, so the pawn
	// cannot re-collapse pyrrhic at its call site again.
	static EGameFlowOutcome OutcomeForHoardOutcome(EHoardOutcome HoardOutcome);
	// [2026-11-04 T-135] Builds the end-screen snapshot from a
	// classified hoard outcome plus the resolution-score kill
	// split. Negative kill counts clamp to zero: a counter bug
	// must read as "none recorded", never as a negative score.
	static FGameFlowResult MakeResult(EHoardOutcome HoardOutcome, int64 CurrentHoard,
									  int64 StartingHoard, int32 Stars, int32 FinalWave,
									  int32 InsideKills, int32 OutsideKills);

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
	// [2026-11-04 T-138] See SetSandboxEnabled (kept across
	// StartPlay's result reset; cleared by QuitToTitle so arriving
	// at the title always re-asks which mode the next run is).
	bool bSandboxEnabled = false;
};
