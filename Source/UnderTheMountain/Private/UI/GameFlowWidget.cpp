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
#include "UI/SettingsWidget.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace
{
// [2026-11-03 T-072] Same code-built styling contract as the intro
// choice: flat brushes, centred labels, no engine-default grey.
UButton* MakeFlowButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
						FLinearColor Normal, int32 FontSize)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	if (!Button)
	{
		return nullptr;
	}
	FButtonStyle Style = Button->GetStyle();
	Style.Normal = FSlateColorBrush(Normal);
	Style.Hovered = FSlateColorBrush(Normal * 1.15f);
	Style.Pressed = FSlateColorBrush(Normal * 0.8f);
	Style.NormalPadding = FMargin(18.0f, 12.0f);
	Style.PressedPadding = FMargin(18.0f, 12.0f);
	Button->SetStyle(Style);
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
														 *(FString(Name) + TEXT("Label")));
	if (Text)
	{
		Text->SetText(FText::FromString(Label));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
	}
	return Button;
}

UTextBlock* MakeFlowText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size,
						 FLinearColor Color)
{
	UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	if (Label)
	{
		Label->SetText(FText::FromString(Text));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetJustification(ETextJustify::Center);
	}
	return Label;
}

// [2026-11-03 T-112] One screen panel: a dim Border holding the content
// Box, attached to the canvas centred (the T-019 intro-choice pattern —
// T-072 built these but never attached them, so the root canvas rendered
// empty). Returns the Border (the widget ShowOnly toggles); the Box comes
// back via OutBox for filling.
UBorder* MakeFlowPanel(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name,
					   UVerticalBox*& OutBox)
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
	Panel->SetVisibility(ESlateVisibility::Hidden);
	Panel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
	Panel->SetPadding(FMargin(36.0f, 28.0f));
	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Panel))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D(0.0f, 0.0f));
		Slot->SetSize(FVector2D(560.0f, 480.0f));
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
}

void UGameFlowWidget::ShowOnly(UWidget* Panel)
{
	for (UWidget* Candidate : {TitlePanel.Get(), PausePanel.Get(), EndPanel.Get()})
	{
		if (Candidate != nullptr)
		{
			Candidate->SetVisibility(Candidate == Panel ? ESlateVisibility::Visible
														: ESlateVisibility::Hidden);
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

	const FLinearColor Ember(0.75f, 0.25f, 0.08f);
	const FLinearColor Slate(0.25f, 0.28f, 0.35f);
	const FLinearColor Gold(1.0f, 0.78f, 0.3f);
	const FLinearColor Muted(0.75f, 0.78f, 0.85f);

	// Title: game name, Start (begin the run), Quit (leave).
	UVerticalBox* TitleBox = nullptr;
	TitlePanel = MakeFlowPanel(WidgetTree, Canvas, TEXT("FlowTitle"), TitleBox);
	if (TitleBox != nullptr)
	{
		TitleBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowTitleHead"), TEXT("UNDER THE MOUNTAIN"),
										40, Gold));
		TitleBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowTitleSub"),
										TEXT("You are the dragon. Guard your hoard."), 16, Muted));
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
	}
	// Pause: resume the run, restart it clean, back out, or leave.
	UVerticalBox* PauseBox = nullptr;
	PausePanel = MakeFlowPanel(WidgetTree, Canvas, TEXT("FlowPause"), PauseBox);
	if (PauseBox != nullptr)
	{
		PauseBox->AddChild(MakeFlowText(WidgetTree, TEXT("FlowPauseHead"), TEXT("PAUSED"), 32, Gold));
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
	}
	// End: outcome line rewritten per run, then the ways back.
	UVerticalBox* EndBox = nullptr;
	EndPanel = MakeFlowPanel(WidgetTree, Canvas, TEXT("FlowEnd"), EndBox);
	if (EndBox != nullptr)
	{
		EndBox->AddChild(
			MakeFlowText(WidgetTree, TEXT("FlowEndHead"), TEXT("THE RUN IS OVER"), 32, Gold));
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
