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
	void TogglePauseRun();
	void RestartRun();
	void QuitToTitle();
	void QuitGame();
	// Session end from the pawn's audit (T-016 outcome + numbers).
	void RequestSessionEnd(EGameFlowOutcome Outcome, int64 CurrentHoard, int64 StartingHoard,
						   int32 Stars, int32 Wave);
	bool HasRunStarted() const;
	// [2026-11-03 T-072] Pawn pause routing reads the state; all
	// mutation goes through the verbs above (the machine stays the
	// only writer).
	UGameFlowMachine::EState GetFlowState() const;

  private:
	// [2026-11-03 T-072] Applies a machine state to the world: pause
	// flag, input mode + cursor, visible screen.
	void ApplyFlowState();
	APlayerController* GetFlowController() const;

	// [2026-11-03 T-072] Held by UPROPERTY so the GC sees them (the
	// T-108 contract: every UObject the game owns must reflect).
	UPROPERTY()
	TObjectPtr<UGameFlowMachine> FlowMachine;
	UPROPERTY()
	TObjectPtr<UGameFlowWidget> FlowWidget;
};
