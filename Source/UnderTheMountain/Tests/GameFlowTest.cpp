// Copyright Under the Mountain contributors.
// Purpose: T-072 game-flow state machine contract — every transition,
// every illegal-transition guard, and the no-soft-lock rule (every
// state reaches title or a fresh run). Pure logic, headless: the
// machine owns no widgets, pause flags, or level loads, so NewObject
// plus the transition calls prove the whole graph.
// Created 2026-11-03 (T-072).

#include "Misc/AutomationTest.h"
#include "Core/GameFlowMachine.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-03 T-072] The shell's contract: title → playing → paused →
// won/lost, restart from any end/paused state, quit-to-title from
// everywhere, and no transition that can strand the run.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameFlowTest, "UnderTheMountain.GameFlow.Transitions",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FGameFlowResult EndedResult()
{
	FGameFlowResult Result;
	Result.Outcome = EGameFlowOutcome::Won;
	Result.StartingHoard = 3000;
	Result.CurrentHoard = 2400;
	Result.Stars = 3;
	Result.FinalWave = 6;
	return Result;
}

bool FGameFlowTest::RunTest(const FString& Parameters)
{
	// --- Start: title is the only entry, played once ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		TestTrue(TEXT("starts at title"),
				 Flow->GetState() == UGameFlowMachine::EState::Title);
		TestFalse(TEXT("title counts as not playing"), Flow->IsPlaying());
		TestTrue(TEXT("start play leaves title"), Flow->StartPlay());
		TestTrue(TEXT("playing reads back"), Flow->IsPlaying());
		TestFalse(TEXT("second start is refused"), Flow->StartPlay());
	}

	// --- Pause toggle: playing <-> paused, nowhere else ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		TestFalse(TEXT("pause from title is refused"), Flow->TogglePause());
		Flow->StartPlay();
		TestTrue(TEXT("pause engages from playing"), Flow->TogglePause());
		TestFalse(TEXT("paused is not playing"), Flow->IsPlaying());
		TestTrue(TEXT("resume returns to playing"), Flow->TogglePause());
		TestTrue(TEXT("back to playing"), Flow->IsPlaying());
	}

	// --- End: playing/paused resolve, the result sticks ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		Flow->StartPlay();
		TestTrue(TEXT("end resolves from playing"), Flow->RequestEnd(EndedResult()));
		TestTrue(TEXT("ended reads back"), Flow->IsEnded());
		TestFalse(TEXT("playing after end is not playing"), Flow->IsPlaying());
		TestEqual(TEXT("hoard snapshot sticks"), Flow->GetResult().CurrentHoard, (int64)2400);
		TestEqual(TEXT("stars snapshot sticks"), Flow->GetResult().Stars, 3);
		TestFalse(TEXT("pause from won is refused"), Flow->TogglePause());
		TestFalse(TEXT("start from won is refused"), Flow->StartPlay());
	}

	// --- End from pause, and lost resolves too ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		Flow->StartPlay();
		Flow->TogglePause();
		FGameFlowResult Lost;
		Lost.Outcome = EGameFlowOutcome::Lost;
		TestTrue(TEXT("end resolves from pause"), Flow->RequestEnd(Lost));
		TestTrue(TEXT("lost ends the run"), Flow->IsEnded());
	}

	// --- Restart: end/paused states return to a clean playing run ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		TestFalse(TEXT("restart from title is refused"), Flow->RestartRun());
		Flow->StartPlay();
		TestFalse(TEXT("restart from playing is refused"), Flow->RestartRun());
		Flow->TogglePause();
		TestTrue(TEXT("restart from pause works"), Flow->RestartRun());
		TestTrue(TEXT("restarted run is playing"), Flow->IsPlaying());
		TestTrue(TEXT("restart clears the result"),
				 Flow->GetResult().Outcome == EGameFlowOutcome::InProgress);
		Flow->RequestEnd(EndedResult());
		TestTrue(TEXT("restart from won works"), Flow->RestartRun());
		TestTrue(TEXT("restarted run is playing again"), Flow->IsPlaying());
	}

	// --- No soft-lock: every state reaches title in one call ---
	{
		for (int32 Step = 0; Step < 4; ++Step)
		{
			UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
			// Drive to each state, then quit to title.
			if (Step >= 1)
			{
				Flow->StartPlay();
			}
			if (Step >= 2)
			{
				Flow->TogglePause();
			}
			if (Step == 3)
			{
				Flow->RequestEnd(EndedResult());
			}
			Flow->QuitToTitle();
			TestTrue(TEXT("every state reaches title"),
					 Flow->GetState() == UGameFlowMachine::EState::Title);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
