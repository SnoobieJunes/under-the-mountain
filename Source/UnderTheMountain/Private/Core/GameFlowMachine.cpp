// Copyright Under the Mountain contributors.
// [2026-11-03 T-072] Pure transition rules for the game-flow shell.
// Every transition is guarded: illegal calls return false and change
// nothing, so no caller can soft-lock the run by driving the machine
// from the wrong screen.

#include "Core/GameFlowMachine.h"

void UGameFlowMachine::SetSandboxEnabled(bool bEnabled)
{
	bSandboxEnabled = bEnabled;
}

bool UGameFlowMachine::IsSandboxEnabled() const
{
	return bSandboxEnabled;
}

bool UGameFlowMachine::ShouldSpawnWavesOrIncursions(bool bSandbox)
{
	// [2026-11-04 T-138] Sandbox is free flight only: world life
	// (villagers, goats, scent, treasure) stays, game pressure goes.
	return !bSandbox;
}

bool UGameFlowMachine::ShouldApplyNotorietyChanges(bool bSandbox)
{
	// [2026-11-04 T-138] A sandbox dragon is unwitnessed by rule,
	// not by location: even a village-square kill moves nothing.
	return !bSandbox;
}

bool UGameFlowMachine::ShouldResolveSessionOutcome(bool bSandbox)
{
	// [2026-11-04 T-138] No audit, no end screen, no outcome in
	// sandbox; quitting to title is the only way out.
	return !bSandbox;
}

EGameFlowOutcome UGameFlowMachine::OutcomeForHoardOutcome(EHoardOutcome HoardOutcome)
{
	// [2026-11-04 T-135] One-to-one where the spec distinguishes
	// (Win / PyrrhicWin / InProgress), collapsed only where the
	// end screen does not (the two lose flavours).
	switch (HoardOutcome)
	{
	case EHoardOutcome::Win:
		return EGameFlowOutcome::Won;
	case EHoardOutcome::PyrrhicWin:
		return EGameFlowOutcome::PyrrhicWin;
	case EHoardOutcome::LosePlundered:
	case EHoardOutcome::LoseDrivenOut:
		return EGameFlowOutcome::Lost;
	case EHoardOutcome::InProgress:
	default:
		return EGameFlowOutcome::InProgress;
	}
}

FGameFlowResult UGameFlowMachine::MakeResult(EHoardOutcome HoardOutcome, int64 CurrentHoard,
											 int64 StartingHoard, int32 Stars, int32 FinalWave,
											 int32 InsideKills, int32 OutsideKills)
{
	// [2026-11-04 T-135] The snapshot the end screen renders: the
	// outcome via OutcomeForHoardOutcome (pyrrhic never collapsed)
	// and the inside/outside kill split clamped at zero.
	FGameFlowResult Snapshot;
	Snapshot.Outcome = OutcomeForHoardOutcome(HoardOutcome);
	Snapshot.CurrentHoard = CurrentHoard;
	Snapshot.StartingHoard = StartingHoard;
	Snapshot.Stars = Stars;
	Snapshot.FinalWave = FinalWave;
	Snapshot.InsideKills = FMath::Max(0, InsideKills);
	Snapshot.OutsideKills = FMath::Max(0, OutsideKills);
	return Snapshot;
}

bool UGameFlowMachine::StartPlay()
{
	if (State != EState::Title)
	{
		return false;
	}
	State = EState::Playing;
	Result = FGameFlowResult();
	return true;
}

bool UGameFlowMachine::TogglePause()
{
	if (State == EState::Playing)
	{
		State = EState::Paused;
		return true;
	}
	if (State == EState::Paused)
	{
		State = EState::Playing;
		return true;
	}
	return false;
}

bool UGameFlowMachine::RequestEnd(const FGameFlowResult& InResult)
{
	if (State != EState::Playing && State != EState::Paused)
	{
		return false;
	}
	// [2026-11-04 T-138] Sandbox runs never resolve: refuse the
	// end even if a caller drives the audit anyway, so the refusal
	// is proven at this seam and not just by caller discipline.
	if (!ShouldResolveSessionOutcome(bSandboxEnabled))
	{
		return false;
	}
	// [2026-11-04 T-135] InProgress is not an ending: previously
	// any non-Won outcome fell through to Lost, which would have
	// ended a live run on a misclassified snapshot.
	if (InResult.Outcome == EGameFlowOutcome::InProgress)
	{
		return false;
	}
	Result = InResult;
	// [2026-11-04 T-135] Pyrrhic shares the Won state (it is a win
	// the player walks away from) while the snapshot above keeps
	// the distinct outcome for the end screen.
	State = (InResult.Outcome == EGameFlowOutcome::Lost) ? EState::Lost : EState::Won;
	return true;
}

bool UGameFlowMachine::RestartRun()
{
	if (State != EState::Won && State != EState::Lost && State != EState::Paused)
	{
		return false;
	}
	State = EState::Playing;
	Result = FGameFlowResult();
	return true;
}

void UGameFlowMachine::QuitToTitle()
{
	State = EState::Title;
	Result = FGameFlowResult();
	// [2026-11-04 T-138] The title is where the mode is chosen, so
	// arriving there disarms sandbox; the next run re-selects it
	// (title button, or the cvar re-applied by the game mode).
	bSandboxEnabled = false;
}
