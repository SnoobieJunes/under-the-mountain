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
#include "Hoard/HoardSubsystem.h"
#include "UI/GameFlowWidget.h"
#include "UI/SettingsWidget.h"

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

// [2026-11-04 T-138] Sandbox contract: the three pure rules flip
// together, the flag survives StartPlay (set at title, run starts
// after), RequestEnd is refused while armed, and a normal machine
// is untouched — the flag defaults off and quit-to-title disarms.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameFlowSandboxTest, "UnderTheMountain.GameFlow.Sandbox",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameFlowSandboxTest::RunTest(const FString& Parameters)
{
	// --- Pure rules: sandbox suppresses, normal run allows ---
	TestFalse(TEXT("sandbox suppresses spawns"),
			  UGameFlowMachine::ShouldSpawnWavesOrIncursions(true));
	TestTrue(TEXT("normal run allows spawns"),
			 UGameFlowMachine::ShouldSpawnWavesOrIncursions(false));
	TestFalse(TEXT("sandbox suppresses notoriety"),
			  UGameFlowMachine::ShouldApplyNotorietyChanges(true));
	TestTrue(TEXT("normal run allows notoriety"),
			 UGameFlowMachine::ShouldApplyNotorietyChanges(false));
	TestFalse(TEXT("sandbox suppresses outcome"),
			  UGameFlowMachine::ShouldResolveSessionOutcome(true));
	TestTrue(TEXT("normal run resolves outcome"),
			 UGameFlowMachine::ShouldResolveSessionOutcome(false));

	// --- Flag lifecycle on the machine ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		TestFalse(TEXT("sandbox defaults off"), Flow->IsSandboxEnabled());
		Flow->SetSandboxEnabled(true);
		TestTrue(TEXT("sandbox arms"), Flow->IsSandboxEnabled());
		TestTrue(TEXT("sandbox run still starts playing"), Flow->StartPlay());
		TestTrue(TEXT("flag survives start play"), Flow->IsSandboxEnabled());
		TestFalse(TEXT("sandbox refuses session end"), Flow->RequestEnd(EndedResult()));
		TestTrue(TEXT("sandbox run is still playing after refused end"), Flow->IsPlaying());
		TestFalse(TEXT("sandbox run has not ended"), Flow->IsEnded());
		Flow->QuitToTitle();
		TestFalse(TEXT("quit to title disarms sandbox"), Flow->IsSandboxEnabled());
	}

	// --- Normal run unchanged once disarmed ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		Flow->SetSandboxEnabled(true);
		Flow->SetSandboxEnabled(false);
		Flow->StartPlay();
		TestTrue(TEXT("disarmed run ends normally"), Flow->RequestEnd(EndedResult()));
		TestTrue(TEXT("disarmed run has ended"), Flow->IsEnded());
	}
	return true;
}

// [2026-11-04 T-135] Outcome-mapping contract: PyrrhicWin reaches
// the snapshot as itself at both spec boundary fractions, Won and
// Lost keep their meanings, and the resolution kill split rides
// along (clamped, never negative).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameFlowOutcomeTest, "UnderTheMountain.GameFlow.Outcome",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameFlowOutcomeTest::RunTest(const FString& Parameters)
{
	// --- The hoard-to-flow mapping, one-to-one where distinct ---
	TestTrue(TEXT("hoard Win maps to Won"),
			 UGameFlowMachine::OutcomeForHoardOutcome(EHoardOutcome::Win) == EGameFlowOutcome::Won);
	TestTrue(TEXT("hoard PyrrhicWin maps to PyrrhicWin, not Won"),
			 UGameFlowMachine::OutcomeForHoardOutcome(EHoardOutcome::PyrrhicWin) ==
				 EGameFlowOutcome::PyrrhicWin);
	TestTrue(TEXT("hoard LosePlundered maps to Lost"),
			 UGameFlowMachine::OutcomeForHoardOutcome(EHoardOutcome::LosePlundered) ==
				 EGameFlowOutcome::Lost);
	TestTrue(TEXT("hoard LoseDrivenOut maps to Lost"),
			 UGameFlowMachine::OutcomeForHoardOutcome(EHoardOutcome::LoseDrivenOut) ==
				 EGameFlowOutcome::Lost);
	TestTrue(TEXT("hoard InProgress maps to InProgress"),
			 UGameFlowMachine::OutcomeForHoardOutcome(EHoardOutcome::InProgress) ==
				 EGameFlowOutcome::InProgress);

	// --- Boundary hoard fractions through the real classifier
	// (8500 start, spec 0.5/0.2 fractions): the snapshot carries
	// pyrrhic at 4249 and at exactly 1700, never collapsed ---
	auto SnapshotAt = [](int64 Current, bool bFinal, bool bDown)
	{
		const EHoardOutcome HoardOutcome =
			UHoardSubsystem::ClassifyOutcome(Current, 8500, 0.5f, 0.2f, bFinal, bDown);
		return UGameFlowMachine::MakeResult(HoardOutcome, Current, 8500, 3, 6, 4, 1);
	};
	TestTrue(TEXT("4250 (win boundary) snapshots Won"),
			 SnapshotAt(4250, true, false).Outcome == EGameFlowOutcome::Won);
	TestTrue(TEXT("4249 (below win) snapshots PyrrhicWin"),
			 SnapshotAt(4249, true, false).Outcome == EGameFlowOutcome::PyrrhicWin);
	TestTrue(TEXT("1700 (lose boundary) snapshots PyrrhicWin"),
			 SnapshotAt(1700, true, false).Outcome == EGameFlowOutcome::PyrrhicWin);
	TestTrue(TEXT("1699 (below lose) snapshots Lost"),
			 SnapshotAt(1699, true, false).Outcome == EGameFlowOutcome::Lost);
	TestTrue(TEXT("dragon down snapshots Lost on a full hoard"),
			 SnapshotAt(8500, true, true).Outcome == EGameFlowOutcome::Lost);
	TestTrue(TEXT("thin hoard mid-session snapshots InProgress"),
			 SnapshotAt(100, false, false).Outcome == EGameFlowOutcome::InProgress);

	// --- Snapshot fields stick, kill counts clamp ---
	{
		const FGameFlowResult Result = SnapshotAt(3000, true, false);
		TestEqual(TEXT("snapshot hoard sticks"), Result.CurrentHoard, (int64)3000);
		TestEqual(TEXT("snapshot inside kills stick"), Result.InsideKills, 4);
		TestEqual(TEXT("snapshot outside kills stick"), Result.OutsideKills, 1);
		const FGameFlowResult Clamped =
			UGameFlowMachine::MakeResult(EHoardOutcome::Win, 8500, 8500, 0, 6, -2, -1);
		TestEqual(TEXT("negative inside kills clamp to zero"), Clamped.InsideKills, 0);
		TestEqual(TEXT("negative outside kills clamp to zero"), Clamped.OutsideKills, 0);
	}

	// --- The machine ends pyrrhic runs in the Won state while the
	// snapshot stays PyrrhicWin; InProgress never ends a run ---
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		Flow->StartPlay();
		TestTrue(TEXT("pyrrhic end resolves"), Flow->RequestEnd(SnapshotAt(3000, true, false)));
		TestTrue(TEXT("pyrrhic run has ended"), Flow->IsEnded());
		TestTrue(TEXT("ended state is the won state"),
				 Flow->GetState() == UGameFlowMachine::EState::Won);
		TestTrue(TEXT("snapshot outcome stays PyrrhicWin"),
				 Flow->GetResult().Outcome == EGameFlowOutcome::PyrrhicWin);
	}
	{
		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		Flow->StartPlay();
		TestFalse(TEXT("InProgress snapshot cannot end a run"),
				  Flow->RequestEnd(SnapshotAt(100, false, false)));
		TestTrue(TEXT("run still playing after InProgress refusal"), Flow->IsPlaying());
	}

	// --- End-screen text: all three outcomes, verbatim split ---
	TestEqual(TEXT("won verdict"), UGameFlowWidget::FormatOutcomeVerdict(EGameFlowOutcome::Won),
			  FString(TEXT("HOARD DEFENDED")));
	TestEqual(TEXT("pyrrhic verdict is distinct"),
			  UGameFlowWidget::FormatOutcomeVerdict(EGameFlowOutcome::PyrrhicWin),
			  FString(TEXT("PYRRHIC WIN - HOARD GUTTED")));
	TestEqual(TEXT("lost verdict"), UGameFlowWidget::FormatOutcomeVerdict(EGameFlowOutcome::Lost),
			  FString(TEXT("HOARD LOST")));
	TestEqual(TEXT("resolution line carries the split"),
			  UGameFlowWidget::FormatResolutionScore(4, 1),
			  FString(TEXT("Resolution: 4 inside / 1 outside kills")));
	{
		FGameFlowResult Result = SnapshotAt(3000, true, false);
		const FString Screen = UGameFlowWidget::FormatEndScreen(Result);
		TestTrue(TEXT("end screen shows the pyrrhic verdict"),
				 Screen.Contains(TEXT("PYRRHIC WIN - HOARD GUTTED")));
		TestTrue(TEXT("end screen shows the resolution split"),
				 Screen.Contains(TEXT("Resolution: 4 inside / 1 outside kills")));
		TestTrue(TEXT("end screen shows the hoard cost"), Screen.Contains(TEXT("Hoard 3000 of 8500")));
		Result.Outcome = EGameFlowOutcome::Won;
		TestTrue(TEXT("won end screen shows defended"),
				 UGameFlowWidget::FormatEndScreen(Result).Contains(TEXT("HOARD DEFENDED")));
		Result.Outcome = EGameFlowOutcome::Lost;
		TestTrue(TEXT("lost end screen shows lost"),
				 UGameFlowWidget::FormatEndScreen(Result).Contains(TEXT("HOARD LOST")));
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
	const TCHAR* Buttons[12] = {TEXT("FlowStart"), TEXT("FlowStartSandbox"), TEXT("FlowTitleQuit"),
								TEXT("FlowTitleSettings"), TEXT("FlowResume"), TEXT("FlowPauseRestart"),
								TEXT("FlowPauseTitle"), TEXT("FlowPauseQuit"), TEXT("FlowPauseSettings"),
								TEXT("FlowEndRestart"), TEXT("FlowEndTitle"), TEXT("FlowEndQuit")};
	for (int32 I = 0; I < 12; ++I)
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

	// [2026-11-04] Settings round-trip: SETTINGS from title opens the
	// settings panel and hides the flow panels; its CLOSE returns to
	// title. Same from pause returns to pause (origin is remembered).
	{
		USettingsWidget* Settings = NewObject<USettingsWidget>();
		TestNotNull(TEXT("settings widget constructs"), Settings);
		if (Settings != nullptr)
		{
			Settings->Initialize();
			Settings->NativeOnInitialized();
			Widget->BindSettingsWidget(Settings);
			UWidgetTree* SettingsTree = Settings->WidgetTree;
			UWidget* SettingsPanel =
				SettingsTree != nullptr ? SettingsTree->FindWidget(FName(TEXT("SettingsPanelBorder"))) : nullptr;
			TestNotNull(TEXT("settings panel exists"), SettingsPanel);
			UButton* TitleSettings = Tree->FindWidget<UButton>(FName(TEXT("FlowTitleSettings")));
			UButton* PauseSettings = Tree->FindWidget<UButton>(FName(TEXT("FlowPauseSettings")));
			UButton* Close = SettingsTree != nullptr
								 ? SettingsTree->FindWidget<UButton>(FName(TEXT("SettingsClose")))
								 : nullptr;
			TestNotNull(TEXT("settings close button exists"), Close);
			if (SettingsPanel != nullptr && TitleSettings != nullptr && Close != nullptr)
			{
				Widget->ShowTitle();
				TitleSettings->OnClicked.Broadcast();
				TestTrue(TEXT("title SETTINGS opens settings"),
						 SettingsPanel->GetVisibility() == ESlateVisibility::Visible);
				TestTrue(TEXT("opening settings hides the title panel"),
						 Panels[0]->GetVisibility() == ESlateVisibility::Hidden);
				Close->OnClicked.Broadcast();
				TestTrue(TEXT("settings close hides settings"),
						 SettingsPanel->GetVisibility() == ESlateVisibility::Hidden);
				TestTrue(TEXT("settings close returns to title"),
						 Panels[0]->GetVisibility() == ESlateVisibility::Visible);
			}
			if (SettingsPanel != nullptr && PauseSettings != nullptr && Close != nullptr)
			{
				Widget->ShowPause();
				PauseSettings->OnClicked.Broadcast();
				TestTrue(TEXT("pause SETTINGS opens settings"),
						 SettingsPanel->GetVisibility() == ESlateVisibility::Visible);
				Close->OnClicked.Broadcast();
				TestTrue(TEXT("settings close returns to pause (origin remembered)"),
						 Panels[1]->GetVisibility() == ESlateVisibility::Visible &&
							 Panels[0]->GetVisibility() == ESlateVisibility::Hidden);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
