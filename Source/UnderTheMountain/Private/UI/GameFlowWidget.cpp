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

UVerticalBox* MakeFlowPanel(UWidgetTree* Tree, const TCHAR* Name)
{
	UBorder* Panel =
		Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(FString(Name) + TEXT("Border")));
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
	if (Panel != nullptr && Box != nullptr)
	{
		Panel->SetContent(Box);
		Panel->SetVisibility(ESlateVisibility::Hidden);
	}
	return Box;
}
} // namespace

void UGameFlowWidget::BindToGameMode(AUnderTheMountainGameMode* Mode)
{
	BoundMode = Mode;
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
}

void UGameFlowWidget::ShowPause()
{
	ShowOnly(PausePanel.Get());
}

void UGameFlowWidget::ShowEnd(const FGameFlowResult& Result)
{
	if (EndOutcomeText != nullptr)
	{
		const TCHAR* Verdict =
			Result.Outcome == EGameFlowOutcome::Won ? TEXT("HOARD DEFENDED") : TEXT("HOARD LOST");
		EndOutcomeText->SetText(FText::FromString(FString::Printf(
			TEXT("%s\nHoard %lld of %lld  —  %d stars  —  wave %d"), Verdict, Result.CurrentHoard,
			Result.StartingHoard, Result.Stars, Result.FinalWave)));
	}
	ShowOnly(EndPanel.Get());
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
	TitlePanel = MakeFlowPanel(WidgetTree, TEXT("FlowTitle"));
	if (UVerticalBox* Box = Cast<UVerticalBox>(TitlePanel.Get()))
	{
		Box->AddChild(MakeFlowText(WidgetTree, TEXT("FlowTitleHead"), TEXT("UNDER THE MOUNTAIN"),
								   40, Gold));
		Box->AddChild(MakeFlowText(WidgetTree, TEXT("FlowTitleSub"),
								   TEXT("You are the dragon. Guard your hoard."), 16, Muted));
		if (UButton* Start = MakeFlowButton(WidgetTree, TEXT("FlowStart"), TEXT("START RUN"),
											Ember, 20))
		{
			Start->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleStart);
			Box->AddChild(Start);
		}
		if (UButton* Quit = MakeFlowButton(WidgetTree, TEXT("FlowTitleQuit"), TEXT("QUIT"),
										   Slate, 20))
		{
			Quit->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuit);
			Box->AddChild(Quit);
		}
	}
	// Pause: resume the run, restart it clean, back out, or leave.
	PausePanel = MakeFlowPanel(WidgetTree, TEXT("FlowPause"));
	if (UVerticalBox* Box = Cast<UVerticalBox>(PausePanel.Get()))
	{
		Box->AddChild(MakeFlowText(WidgetTree, TEXT("FlowPauseHead"), TEXT("PAUSED"), 32, Gold));
		if (UButton* Resume = MakeFlowButton(WidgetTree, TEXT("FlowResume"), TEXT("RESUME"),
											 Ember, 20))
		{
			Resume->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleResume);
			Box->AddChild(Resume);
		}
		if (UButton* Restart = MakeFlowButton(WidgetTree, TEXT("FlowPauseRestart"),
											  TEXT("RESTART RUN"), Slate, 20))
		{
			Restart->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleRestart);
			Box->AddChild(Restart);
		}
		if (UButton* Title = MakeFlowButton(WidgetTree, TEXT("FlowPauseTitle"),
											TEXT("QUIT TO TITLE"), Slate, 20))
		{
			Title->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuitToTitle);
			Box->AddChild(Title);
		}
		if (UButton* Quit = MakeFlowButton(WidgetTree, TEXT("FlowPauseQuit"), TEXT("QUIT GAME"),
										   Slate, 20))
		{
			Quit->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuit);
			Box->AddChild(Quit);
		}
	}
	// End: outcome line rewritten per run, then the ways back.
	EndPanel = MakeFlowPanel(WidgetTree, TEXT("FlowEnd"));
	if (UVerticalBox* Box = Cast<UVerticalBox>(EndPanel.Get()))
	{
		Box->AddChild(
			MakeFlowText(WidgetTree, TEXT("FlowEndHead"), TEXT("THE RUN IS OVER"), 32, Gold));
		EndOutcomeText = MakeFlowText(WidgetTree, TEXT("FlowEndOutcome"), TEXT(""), 18, Muted);
		Box->AddChild(EndOutcomeText);
		if (UButton* Restart = MakeFlowButton(WidgetTree, TEXT("FlowEndRestart"),
											  TEXT("RESTART RUN"), Ember, 20))
		{
			Restart->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleRestart);
			Box->AddChild(Restart);
		}
		if (UButton* Title = MakeFlowButton(WidgetTree, TEXT("FlowEndTitle"),
											TEXT("QUIT TO TITLE"), Slate, 20))
		{
			Title->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuitToTitle);
			Box->AddChild(Title);
		}
		if (UButton* Quit = MakeFlowButton(WidgetTree, TEXT("FlowEndQuit"), TEXT("QUIT GAME"),
										   Slate, 20))
		{
			Quit->OnClicked.AddDynamic(this, &UGameFlowWidget::HandleQuit);
			Box->AddChild(Quit);
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
