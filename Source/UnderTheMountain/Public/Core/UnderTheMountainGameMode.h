// Copyright Under the Mountain contributors.
// Purpose: T-072 game-flow shell — owns the UGameFlowMachine and its
// screens (title / pause / won-lost), applies the machine's verdicts:
// world pause, end screens, level reload for clean restarts, quit.
// Created 2026-10-27 (T-003); flow shell added 2026-11-03 (T-072).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/GameFlowMachine.h"
#include "UnderTheMountainGameMode.generated.h"

class UGameFlowWidget;
class USettingsWidget;

UCLASS()
class UNDERTHEMOUNTAIN_API AUnderTheMountainGameMode : public AGameModeBase
{
	GENERATED_BODY()

  public:
	AUnderTheMountainGameMode();

  protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

  public:
	// [2026-11-03 T-072] Flow verbs, wired to the widget buttons and
	// the pawn's pause input. Every verb goes through the machine, so
	// illegal calls (pause from title, restart from title) are no-ops.
	void StartRun();
	// [2026-11-04 T-138] Title sandbox entry: arms the GameFlow
	// sandbox flag, propagates it to the wave/notoriety subsystem
	// gates, then starts play exactly like StartRun — one shared
	// start path so sandbox cannot drift from the normal launch.
	void StartSandboxRun();
	// [2026-11-04 T-138] True while the current run was launched
	// in sandbox (title button or utm.sandbox cvar); the pawn and
	// HUD read this, the machine remains the only writer.
	bool IsSandboxRun() const;
	void TogglePauseRun();
	void RestartRun();
	void QuitToTitle();
	void QuitGame();
	// Session end from the pawn's audit (T-016 outcome + numbers).
	// [2026-11-04 T-135] Inside/OutsideKills carry the resolution
	// score split into the snapshot (default 0 for callers that
	// have no kill accounting yet).
	void RequestSessionEnd(EGameFlowOutcome Outcome, int64 CurrentHoard, int64 StartingHoard,
						   int32 Stars, int32 Wave, int32 InsideKills = 0,
						   int32 OutsideKills = 0);
	bool HasRunStarted() const;
	// [2026-11-03 T-072] Pawn pause routing reads the state; all
	// mutation goes through the verbs above (the machine stays the
	// only writer).
	UGameFlowMachine::EState GetFlowState() const;

  private:
	// [2026-11-03 T-072] Applies a machine state to the world: pause
	// flag, input mode + cursor, visible screen.
	void ApplyFlowState();
	// [2026-11-04 T-138] Pushes the machine's sandbox flag into
	// the wave and notoriety subsystem gates (their own seams, so
	// the rules hold even for callers that bypass this mode).
	// Called at BeginPlay, both start verbs, and quit-to-title.
	void ApplySandboxToSubsystems();
	APlayerController* GetFlowController() const;

	// [2026-11-03 T-072] Held by UPROPERTY so the GC sees them (the
	// T-108 contract: every UObject the game owns must reflect).
	UPROPERTY()
	TObjectPtr<UGameFlowMachine> FlowMachine;
	UPROPERTY()
	TObjectPtr<UGameFlowWidget> FlowWidget;
	// [2026-11-04] Settings screen, created beside the flow widget and
	// added one viewport layer above it (see BeginPlay).
	UPROPERTY()
	TObjectPtr<USettingsWidget> SettingsWidget;

	// [2026-11-03 T-110] Proof capture (automation screenshots): when
	// utm.proof.shotdelay is positive, the game takes one HighResShot
	// that many wall-clock seconds after the first tick and, when
	// utm.proof.quit is positive, quits that many seconds after the
	// shot. Both default off — players never see it. T-110's -game
	// captures ride on this because PIE-in-viewport is uninspectable
	// (game viewport absent from the SlateInspector tree, full-window
	// MCP captures exceed the stream) and OS window capture needed a
	// Screen Recording grant mid-task (used as cross-check only).
	// Wall clock, not tick delta: this mode ticks even while the title
	// world, and paused delta is not to be trusted for countdowns.
	void TickProofCapture();
	double ProofFirstTickTime = -1.0;
	double ProofShotTime = -1.0;
	bool bProofShotTaken = false;
	// [2026-11-04 T-020] Proof fps accumulators: average + worst frame over
	// [2026-11-04 T-020] the run (minus a 10 s first-launch warmup), printed
	// [2026-11-04 T-020] with the shot. stat fps never logs, so the packaged
	// [2026-11-04 T-020] perf gate needs its own numbers in stdout.
	double ProofLastTickTime = -1.0;
	double ProofFrameTotal = 0.0;
	double ProofFrameWorst = 0.0;
	int64 ProofFrames = 0;
	int64 ProofFrames60 = 0;
	int64 ProofFrames30 = 0;
	int64 ProofFramesSlow = 0;
};
