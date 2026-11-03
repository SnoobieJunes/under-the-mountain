// Copyright Under the Mountain contributors.
// [2026-11-03 T-072] Pure transition rules for the game-flow shell.
// Every transition is guarded: illegal calls return false and change
// nothing, so no caller can soft-lock the run by driving the machine
// from the wrong screen.

#include "Core/GameFlowMachine.h"

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
	Result = InResult;
	State = (InResult.Outcome == EGameFlowOutcome::Won) ? EState::Won : EState::Lost;
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
}
