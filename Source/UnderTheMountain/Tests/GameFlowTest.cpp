// Copyright Under the Mountain contributors.
// Purpose: T-072 game-flow state machine contract — every transition,
// every illegal-transition guard, and the no-soft-lock rule (every
// state reaches title or a fresh run). Pure logic, headless: the
// machine owns no widgets, pause flags, or level loads, so NewObject
// plus the transition calls prove the whole graph.
// Created 2026-11-03 (T-072).

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Core/GameFlowMachine.h"
#include "UI/GameFlowWidget.h"

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

// [2026-11-03 T-112] Flow-widget render contract: T-072's screens shipped
// with their panels never attached to the canvas (empty root, zero pixels
// in every -game capture) and the widget non-focusable (the UIOnly focus
// Error). Pins attach + visibility switching + button bindings + focus.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowWidgetRenderTest, "UnderTheMountain.UI.FlowWidget",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowWidgetRenderTest::RunTest(const FString& Parameters)
{
	UGameFlowWidget* Widget = NewObject<UGameFlowWidget>();
	TestNotNull(TEXT("flow widget constructs"), Widget);
	if (Widget == nullptr)
	{
		return false;
	}
	// [2026-11-03 T-112] Headless has no player context, so Initialize
	// builds the WidgetTree but skips NativeOnInitialized — drive it
	// directly (public on this widget exactly for this test).
	Widget->Initialize();
	UWidgetTree* Tree = Widget->WidgetTree;
	TestNotNull(TEXT("widget tree built"), Tree);
	if (Tree == nullptr)
	{
		return false;
	}
	Widget->NativeOnInitialized();

	UWidget* Root = Tree->RootWidget;
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(Root);
	TestNotNull(TEXT("root is a canvas"), Canvas);
	// [2026-11-03 T-112] Focus needs no tree: pin it before the
	// attach asserts so it goes red on its own pre-fix merit.
	TestTrue(TEXT("flow widget is focusable (the UIOnly focus target)"),
			 Widget->IsFocusable());
	if (Canvas == nullptr)
	{
		return false;
	}
	TestEqual(TEXT("all three panels attached to the canvas"), Canvas->GetChildrenCount(), 3);

	// Every panel border hangs under the canvas; every box under its border.
	const TCHAR* Borders[3] = {TEXT("FlowTitleBorder"), TEXT("FlowPauseBorder"),
							   TEXT("FlowEndBorder")};
	const TCHAR* Boxes[3] = {TEXT("FlowTitle"), TEXT("FlowPause"), TEXT("FlowEnd")};
	UWidget* Panels[3] = {nullptr, nullptr, nullptr};
	for (int32 I = 0; I < 3; ++I)
	{
		Panels[I] = Tree->FindWidget(FName(Borders[I]));
		TestNotNull(Borders[I], Panels[I]);
		if (Panels[I] != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("%s hangs under the canvas"), Borders[I]),
					 Panels[I]->GetParent() == Canvas);
		}
		UWidget* Box = Tree->FindWidget(FName(Boxes[I]));
		TestNotNull(Boxes[I], Box);
		if (Box != nullptr && Panels[I] != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("%s hangs under its border"), Boxes[I]),
					 Box->GetParent() == Panels[I]);
		}
	}

	// Every screen button exists with its click bound (the binding is what
	// carries START RUN -> StartRun; T-030 owns the live click feel-check).
	const TCHAR* Buttons[9] = {TEXT("FlowStart"), TEXT("FlowTitleQuit"), TEXT("FlowResume"),
							   TEXT("FlowPauseRestart"), TEXT("FlowPauseTitle"), TEXT("FlowPauseQuit"),
							   TEXT("FlowEndRestart"), TEXT("FlowEndTitle"), TEXT("FlowEndQuit")};
	for (int32 I = 0; I < 9; ++I)
	{
		UButton* Button = Tree->FindWidget<UButton>(FName(Buttons[I]));
		TestNotNull(Buttons[I], Button);
		if (Button != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("%s click is bound"), Buttons[I]),
					 Button->OnClicked.IsBound());
		}
	}

	if (Panels[0] == nullptr || Panels[1] == nullptr || Panels[2] == nullptr)
	{
		return false;
	}
	Widget->ShowTitle();
	TestTrue(TEXT("title shows its panel"),
			 Panels[0]->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("title hides pause + end"),
			 Panels[1]->GetVisibility() == ESlateVisibility::Hidden &&
				 Panels[2]->GetVisibility() == ESlateVisibility::Hidden);
	Widget->ShowPause();
	TestTrue(TEXT("pause shows its panel"),
			 Panels[1]->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("pause hides title + end"),
			 Panels[0]->GetVisibility() == ESlateVisibility::Hidden &&
				 Panels[2]->GetVisibility() == ESlateVisibility::Hidden);
	Widget->ShowEnd(EndedResult());
	TestTrue(TEXT("end shows its panel"),
			 Panels[2]->GetVisibility() == ESlateVisibility::Visible);
	UTextBlock* Outcome = Tree->FindWidget<UTextBlock>(FName(TEXT("FlowEndOutcome")));
	TestNotNull(TEXT("end outcome line exists"), Outcome);
	if (Outcome != nullptr)
	{
		TestFalse(TEXT("end outcome line renders text"), Outcome->GetText().IsEmpty());
	}
	Widget->HideAll();
	TestTrue(TEXT("hide-all hides every panel"),
			 Panels[0]->GetVisibility() == ESlateVisibility::Hidden &&
				 Panels[1]->GetVisibility() == ESlateVisibility::Hidden &&
				 Panels[2]->GetVisibility() == ESlateVisibility::Hidden);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
