// Copyright Under the Mountain contributors.
// [2026-11-03 T-072] Code-built flow screens: one full-screen canvas
// with three switchable panels. Buttons are flat-colour styled in
// code (the T-019 precedent) so they read as designed UI.

#include "UI/GameFlowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/UnderTheMountainGameMode.h"
#include "UI/MenuStyle.h"
#include "UI/SettingsWidget.h"
#include "Input/Reply.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace
{
// [2026-11-04] Menu buttons/text come from the shared menu style
// (UtmMenuStyle) so flow and settings cannot drift apart.
UButton* MakeFlowButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
						FLinearColor Normal, int32 FontSize)
{
	return UtmMenuStyle::MakeButton(Tree, Name, Label, Normal, FontSize);
}

UTextBlock* MakeFlowText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size,
						 FLinearColor Color, bool bBold = false)
{
	return UtmMenuStyle::MakeText(Tree, Name, Text, Size, Color, bBold);
}

// [2026-11-03 T-112] One screen panel: a dim Border holding the content
// Box, attached to the canvas centred (the T-019 intro-choice pattern —
// T-072 built these but never attached them, so the root canvas rendered
// empty). Returns the Border (the widget ShowOnly toggles); the Box comes
// back via OutBox for filling.
UBorder* MakeFlowPanel(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name,
					   UVerticalBox*& OutBox, FVector2D Size = FVector2D(600.0f, 540.0f))
{
	OutBox = nullptr;
	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),
													*(FString(Name) + TEXT("Border")));
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
	if (Panel == nullptr || Box == nullptr || Canvas == nullptr)
	{
		return nullptr;
	}
	Panel->SetContent(Box);
	// [2026-11-04] Hidden panels are Collapsed, never Hidden or
	// HitTestInvisible: Collapsed removes layout AND hit-testing,
	// so a non-current panel can never intercept a mouse click
	// meant for the visible one.
	Panel->SetVisibility(ESlateVisibility::Collapsed);
	Panel->SetBrushColor(UtmMenuStyle::PanelTint());
	Panel->SetPadding(FMargin(44.0f, 34.0f));
	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Panel))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D(0.0f, 0.0f));
		Slot->SetSize(Size);
	}
	OutBox = Box;
	return Panel;
}
} // namespace

UGameFlowWidget::UGameFlowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// [2026-11-03 T-112] Focusable at construction: the game mode points
	// UIOnly focus at this widget, and an unfocusable target logs a
	// LogPlayerController Error on every menu open (SObjectWidget reads
	// this flag when focus is attempted).
	SetIsFocusable(true);
}

void UGameFlowWidget::BindToGameMode(AUnderTheMountainGameMode* Mode)
{
	BoundMode = Mode;
}

void UGameFlowWidget::BindSettingsWidget(USettingsWidget* Settings)
{
	if (Settings == nullptr || BoundSettings == Settings)
	{
		return;
	}
	BoundSettings = Settings;
	Settings->OnClosed.AddUObject(this, &UGameFlowWidget::HandleSettingsClosed);
}

void UGameFlowWidget::OpenSettings(bool bFromTitle)
{
	bSettingsFromTitle = bFromTitle;
	if (BoundSettings != nullptr)
	{
		HideAll();
		BoundSettings->ShowSettings();
		// [2026-11-04] Move the UIOnly focus target to the widget
		// now on top, or clicks keep landing on this (hidden) one.
		if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
		{
			Mode->OnMenuSettingsOpened();
		}
	}
}

void UGameFlowWidget::HandleOpenSettingsFromTitle()
{
	OpenSettings(true);
}

void UGameFlowWidget::HandleOpenSettingsFromPause()
{
	OpenSettings(false);
}

void UGameFlowWidget::HandleSettingsClosed()
{
	// [2026-11-04] Return to the originating panel; Show* restores
	// pad focus to that panel's first button.
	if (bSettingsFromTitle)
	{
		ShowTitle();
	}
	else
	{
		ShowPause();
	}
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->OnMenuSettingsClosed();
	}
}

bool UGameFlowWidget::IsSettingsOpen() const
{
	return BoundSettings != nullptr && BoundSettings->IsOpen();
}

EMenuBackAction UGameFlowWidget::ResolveBackAction(bool bSettingsOpen, bool bSettingsFromTitle,
												   EGameFlowPanel VisiblePanel)
{
	// [2026-11-04] Pure chain (see header): settings first, then
	// pause-resume; title and end deliberately ignore Back.
	if (bSettingsOpen)
	{
		return bSettingsFromTitle ? EMenuBackAction::CloseSettingsToTitle
								  : EMenuBackAction::CloseSettingsToPause;
	}
	return VisiblePanel == EGameFlowPanel::Pause ? EMenuBackAction::ResumeRun
												 : EMenuBackAction::Ignored;
}

bool UGameFlowWidget::HandleBack()
{
	switch (ResolveBackAction(IsSettingsOpen(), bSettingsFromTitle, VisiblePanel))
	{
	case EMenuBackAction::CloseSettingsToTitle:
	case EMenuBackAction::CloseSettingsToPause:
		// Settings' HandleBack hides it and broadcasts OnClosed;
		// HandleSettingsClosed above re-shows the origin panel.
		return BoundSettings != nullptr && BoundSettings->HandleBack();
	case EMenuBackAction::ResumeRun:
		HideAll();
		if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
		{
			Mode->TogglePauseRun();
		}
		return true;
	case EMenuBackAction::Ignored:
	default:
		return false;
	}
}

FReply UGameFlowWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// [2026-11-04] Back chain (widget step): Esc / Circle bubbling
	// up from the focused button land here. Consumed only when
	// HandleBack acts; otherwise fall through to normal UI keys.
	if (UtmMenuStyle::IsMenuBackKey(InKeyEvent.GetKey()) && HandleBack())
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGameFlowWidget::ShowOnly(UWidget* Panel)
{
	// [2026-11-04] Collapsed-hidden invariant: exactly one panel
	// Visible, the rest Collapsed (never Hidden/HitTestInvisible,
	// so a hidden panel takes no layout and eats no clicks).
	VisiblePanel = Panel == TitlePanel.Get()   ? EGameFlowPanel::Title
				   : Panel == PausePanel.Get() ? EGameFlowPanel::Pause
				   : Panel == EndPanel.Get()   ? EGameFlowPanel::End
											   : EGameFlowPanel::None;
	for (UWidget* Candidate : {TitlePanel.Get(), PausePanel.Get(), EndPanel.Get()})
	{
		if (Candidate != nullptr)
		{
			Candidate->SetVisibility(Candidate == Panel ? ESlateVisibility::Visible
														: ESlateVisibility::Collapsed);
		}
	}
}

void UGameFlowWidget::ShowTitle()
{
	ShowOnly(TitlePanel.Get());
	// [2026-11-04] Pad focus starts on the first button (pad nav).
	if (TitleFirstButton != nullptr)
	{
		TitleFirstButton->SetKeyboardFocus();
	}
}

void UGameFlowWidget::ShowPause()
{
	ShowOnly(PausePanel.Get());
	if (PauseFirstButton != nullptr)
	{
		PauseFirstButton->SetKeyboardFocus();
	}
}

FString UGameFlowWidget::FormatOutcomeVerdict(EGameFlowOutcome Outcome)
{
	// [2026-11-04 T-135] Three endings, three wordings: defended,
	// the pyrrhic gutted-hoard slink-away, and lost. Collapsing
	// pyrrhic into "defended" here is exactly the T-135 bug.
	switch (Outcome)
	{
	case EGameFlowOutcome::Won:
		return TEXT("HOARD DEFENDED");
	case EGameFlowOutcome::PyrrhicWin:
		return TEXT("PYRRHIC WIN - HOARD GUTTED");
	case EGameFlowOutcome::Lost:
		return TEXT("HOARD LOST");
	case EGameFlowOutcome::InProgress:
	default:
		return TEXT("RUN IN PROGRESS");
	}
}

FString UGameFlowWidget::FormatResolutionScore(int32 InsideKills, int32 OutsideKills)
{
	// [2026-11-04 T-135] The section 1 resolution score is the
	// split itself (inside kills were invisible, outside kills
	// raised notoriety — the debrief is which kind the dragon
	// dealt), so the line prints both counts, never a blended total.
	return FString::Printf(TEXT("Resolution: %d inside / %d outside kills"), InsideKills,
						   OutsideKills);
}

FString UGameFlowWidget::FormatEndScreen(const FGameFlowResult& Result)
{
	// [2026-11-04 T-135] Verdict, the hoard it cost, then the
	// resolution split — ShowEnd renders exactly this string.
	return FString::Printf(TEXT("%s\nHoard %lld of %lld  —  %d stars  —  wave %d\n%s"),
						   *FormatOutcomeVerdict(Result.Outcome), Result.CurrentHoard,
						   Result.StartingHoard, Result.Stars, Result.FinalWave,
						   *FormatResolutionScore(Result.InsideKills, Result.OutsideKills));
}

void UGameFlowWidget::ShowEnd(const FGameFlowResult& Result)
{
	if (EndOutcomeText != nullptr)
	{
		EndOutcomeText->SetText(FText::FromString(FormatEndScreen(Result)));
	}
	ShowOnly(EndPanel.Get());
	if (EndFirstButton != nullptr)
	{
		EndFirstButton->SetKeyboardFocus();
	}
}

void UGameFlowWidget::HideAll()
{
	ShowOnly(nullptr);
}

void UGameFlowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}
	UCanvasPanel* Canvas =
		WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FlowCanvas"));
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	const FLinearColor Ember = UtmMenuStyle::Ember();
	const FLinearColor Slate = UtmMenuStyle::Slate();
	const FLinearColor Gold = UtmMenuStyle::Gold();
	const FLinearColor Muted = UtmMenuStyle::Muted();
	// [2026-11-04] Footer hint on every panel, DualSense names
	// first (the showcase pad), keyboard alongside.
	const FString FooterHint = TEXT("Cross select  /  Circle back  /  Esc back");

	// [2026-11-04] Title hierarchy: large gold name, Rainier
	// subtitle, gold rule, generous spacing, then the actions.
	UVerticalBox* TitleBox = nullptr;
	TitlePanel = MakeFlowPanel(WidgetTree, Canvas, TEXT("FlowTitle"), TitleBox,
							   FVector2D(640.0f, 560.0f));
	if (TitleBox != nullptr)
	{
		TitleBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowTitleHead"),
										TEXT("UNDER THE MOUNTAIN"), 48, Gold, true));
		UtmMenuStyle::AddSpacer(WidgetTree, TitleBox, TEXT("FlowTitleSpacerA"), 8.0f);
		TitleBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowTitleSub"),
										TEXT("You are the dragon of Rainier. Guard your hoard."),
										17, Muted));
		UtmMenuStyle::AddDivider(WidgetTree, TitleBox, TEXT("FlowTitleDivider"));
		UtmMenuStyle::AddSpacer(WidgetTree, TitleBox, TEXT("FlowTitleSpacerB"), 10.0f);
		if (UButton* Start = MakeFlowButton(WidgetTree, TEXT("FlowStart"), TEXT("START RUN"),
											Ember, 20))
		{
			Start->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleStart);
			TitleFirstButton = Start;
			TitleBox->AddChild(Start);
		}
		// [2026-11-04 T-138] Sandbox sits under Start on the title:
		// same one-click entry, opposite contract (free flight, no
		// game rules) — the label says so before the player commits.
		if (UButton* Sandbox = MakeFlowButton(WidgetTree, TEXT("FlowStartSandbox"),
											  TEXT("START SANDBOX (NO RULES)"), Slate, 20))
		{
			Sandbox->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleStartSandbox);
			TitleBox->AddChild(Sandbox);
		}
		// [2026-11-04] SETTINGS on the title (the pause panel has its
		// own below): opens the game mode's settings widget, whose
		// Close returns to this panel with focus restored.
		if (UButton* Settings = MakeFlowButton(WidgetTree, TEXT("FlowTitleSettings"),
											   TEXT("SETTINGS"), Slate, 20))
		{
			Settings->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleOpenSettingsFromTitle);
			TitleBox->AddChild(Settings);
		}
		if (UButton* Quit = MakeFlowButton(WidgetTree, TEXT("FlowTitleQuit"), TEXT("QUIT"),
										   Slate, 20))
		{
			Quit->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuit);
			TitleBox->AddChild(Quit);
		}
		UtmMenuStyle::AddSpacer(WidgetTree, TitleBox, TEXT("FlowTitleSpacerC"), 14.0f);
		TitleBox->AddChild(
			MakeFlowText(WidgetTree, TEXT("FlowTitleFooter"), FooterHint, 13, Muted));
	}
	// Pause: resume the run, restart it clean, back out, or leave.
	UVerticalBox* PauseBox = nullptr;
	PausePanel = MakeFlowPanel(WidgetTree, Canvas, TEXT("FlowPause"), PauseBox);
	if (PauseBox != nullptr)
	{
		PauseBox->AddChild(
			MakeFlowText(WidgetTree, TEXT("FlowPauseHead"), TEXT("PAUSED"), 36, Gold, true));
		UtmMenuStyle::AddDivider(WidgetTree, PauseBox, TEXT("FlowPauseDivider"));
		UtmMenuStyle::AddSpacer(WidgetTree, PauseBox, TEXT("FlowPauseSpacerA"), 8.0f);
		if (UButton* Resume = MakeFlowButton(WidgetTree, TEXT("FlowResume"), TEXT("RESUME"),
											 Ember, 20))
		{
			Resume->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleResume);
			PauseFirstButton = Resume;
			PauseBox->AddChild(Resume);
		}
		// [2026-11-04] SETTINGS on pause, same return contract as title.
		if (UButton* Settings = MakeFlowButton(WidgetTree, TEXT("FlowPauseSettings"),
											   TEXT("SETTINGS"), Slate, 20))
		{
			Settings->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleOpenSettingsFromPause);
			PauseBox->AddChild(Settings);
		}
		if (UButton* Restart = MakeFlowButton(WidgetTree, TEXT("FlowPauseRestart"),
											  TEXT("RESTART RUN"), Slate, 20))
		{
			Restart->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleRestart);
			PauseBox->AddChild(Restart);
		}
		if (UButton* Title = MakeFlowButton(WidgetTree, TEXT("FlowPauseTitle"),
											TEXT("QUIT TO TITLE"), Slate, 20))
		{
			Title->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuitToTitle);
			PauseBox->AddChild(Title);
		}
		if (UButton* Quit = MakeFlowButton(WidgetTree, TEXT("FlowPauseQuit"), TEXT("QUIT GAME"),
										   Slate, 20))
		{
			Quit->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuit);
			PauseBox->AddChild(Quit);
		}
		UtmMenuStyle::AddSpacer(WidgetTree, PauseBox, TEXT("FlowPauseSpacerB"), 14.0f);
		PauseBox->AddChild(
			MakeFlowText(WidgetTree, TEXT("FlowPauseFooter"), FooterHint, 13, Muted));
	}
	// End: outcome line rewritten per run, then the ways back.
	UVerticalBox* EndBox = nullptr;
	EndPanel = MakeFlowPanel(WidgetTree, Canvas, TEXT("FlowEnd"), EndBox);
	if (EndBox != nullptr)
	{
		EndBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowEndHead"), TEXT("THE RUN IS OVER"), 36,
									  Gold, true));
		UtmMenuStyle::AddDivider(WidgetTree, EndBox, TEXT("FlowEndDivider"));
		EndOutcomeText = MakeFlowText(WidgetTree, TEXT("FlowEndOutcome"), TEXT(""), 18, Muted);
		EndBox->AddChild(EndOutcomeText);
		if (UButton* Restart = MakeFlowButton(WidgetTree, TEXT("FlowEndRestart"),
											  TEXT("RESTART RUN"), Ember, 20))
		{
			Restart->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleRestart);
			EndFirstButton = Restart;
			EndBox->AddChild(Restart);
		}
		if (UButton* Title = MakeFlowButton(WidgetTree, TEXT("FlowEndTitle"),
											TEXT("QUIT TO TITLE"), Slate, 20))
		{
			Title->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuitToTitle);
			EndBox->AddChild(Title);
		}
		if (UButton* Quit = MakeFlowButton(WidgetTree, TEXT("FlowEndQuit"), TEXT("QUIT GAME"),
										   Slate, 20))
		{
			Quit->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuit);
			EndBox->AddChild(Quit);
		}
		UtmMenuStyle::AddSpacer(WidgetTree, EndBox, TEXT("FlowEndSpacer"), 14.0f);
		EndBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowEndFooter"), FooterHint, 13, Muted));
	}
	HideAll();
}

void UGameFlowWidget::HandleStart()
{
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->StartRun();
	}
}

void UGameFlowWidget::HandleStartSandbox()
{
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->StartSandboxRun();
	}
}

void UGameFlowWidget::HandleResume()
{
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->TogglePauseRun();
	}
}

void UGameFlowWidget::HandleRestart()
{
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->RestartRun();
	}
}

void UGameFlowWidget::HandleQuitToTitle()
{
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->QuitToTitle();
	}
}

void UGameFlowWidget::HandleQuit()
{
	if (AUnderTheMountainGameMode* Mode = BoundMode.Get())
	{
		Mode->QuitGame();
	}
}
