// Copyright Under the Mountain contributors.
// [2026-11-03 T-073] Code-built onboarding UI: a bottom prompt line
// plus a toggleable help panel rendered from the single binding table.

#include "UI/OnboardingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/OnboardingContent.h"
#include "Styling/SlateColor.h"

void UOnboardingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}
	UCanvasPanel* Canvas =
		WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("OnboardCanvas"));
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	// Prompt line along the bottom: big, gold, readable mid-flight.
	PromptLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
														 TEXT("OnboardPrompt"));
	if (PromptLine != nullptr)
	{
		PromptLine->SetText(FText::GetEmpty());
		FSlateFontInfo Font = PromptLine->GetFont();
		Font.Size = 22;
		PromptLine->SetFont(Font);
		PromptLine->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.78f, 0.3f)));
		PromptLine->SetJustification(ETextJustify::Center);
		PromptLine->SetVisibility(ESlateVisibility::Hidden);
		// Never intercepts clicks: teaching text, not a button.
		PromptLine->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(PromptLine.Get()))
		{
			Slot->SetAnchors(FAnchors(0.1f, 0.85f, 0.9f, 0.95f));
		}
	}
	BuildHelpPanel();
}

void UOnboardingWidget::ShowPrompt(const FOnboardingPromptDef& Prompt)
{
	if (PromptLine != nullptr)
	{
		PromptLine->SetText(FText::FromString(Prompt.KeyboardText + TEXT("\n") + Prompt.PadText));
		PromptLine->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UOnboardingWidget::ClearPrompt()
{
	if (PromptLine != nullptr)
	{
		PromptLine->SetText(FText::GetEmpty());
		PromptLine->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UOnboardingWidget::BuildHelpPanel()
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (Canvas == nullptr)
	{
		return;
	}
	UScrollBox* Scroller =
		WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OnboardHelp"));
	if (Scroller == nullptr)
	{
		return;
	}
	HelpPanel = Scroller;
	for (const FOnboardingBindingDef& Row : GetOnboardingBindings())
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), *(FString(TEXT("HelpRow_")) + Row.ActionName));
		if (Line == nullptr)
		{
			continue;
		}
		UTextBlock* Action = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *(FString(TEXT("HelpAction_")) + Row.ActionName));
		UTextBlock* Keys = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *(FString(TEXT("HelpKeys_")) + Row.ActionName));
		if (Action != nullptr)
		{
			Action->SetText(FText::FromString(Row.ActionName + TEXT(":  ") + Row.KeyboardKeys));
			FSlateFontInfo Font = Action->GetFont();
			Font.Size = 16;
			Action->SetFont(Font);
			Action->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Line->AddChild(Action);
		}
		if (Keys != nullptr)
		{
			Keys->SetText(FText::FromString(Row.PadKeys));
			FSlateFontInfo Font = Keys->GetFont();
			Font.Size = 16;
			Keys->SetFont(Font);
			Keys->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.78f, 0.85f)));
			Line->AddChild(Keys);
		}
		Scroller->AddChild(Line);
	}
	HelpPanel->SetVisibility(ESlateVisibility::Hidden);
	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(HelpPanel.Get()))
	{
		Slot->SetAnchors(FAnchors(0.15f, 0.1f, 0.85f, 0.8f));
	}
}

void UOnboardingWidget::ToggleHelp()
{
	bHelpVisible = !bHelpVisible;
	if (HelpPanel != nullptr)
	{
		// Non-modal by construction: HitTestInvisible plus a scroll
		// box the game keeps running behind.
		HelpPanel->SetVisibility(bHelpVisible ? ESlateVisibility::HitTestInvisible
											  : ESlateVisibility::Hidden);
	}
}

bool UOnboardingWidget::IsHelpVisible() const
{
	return bHelpVisible;
}
