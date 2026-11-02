// Copyright Under the Mountain contributors.
// [2026-11-01 T-019] UDragonHUDWidget implementation: a canvas
// layout - vitals panel bottom-left (HP bar crimson, stamina bar
// green, numeric readouts right-aligned on the bars' row),
// hoard panel top-right (gold count, star pips, wave state).
// Panels are near-black translucent borders so the readout holds
// against bright sky and dark lair alike. Built entirely in code.

#include "UI/DragonHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateColor.h"

namespace
{
// [2026-11-01 T-019] Shared text styling: one helper so every
// label in the HUD carries the same size/colour language.
UTextBlock* MakeLabel(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size,
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
	}
	return Label;
}

UBorder* MakePanel(UWidgetTree* Tree, const TCHAR* Name)
{
	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	if (Panel)
	{
		// Translucent near-black with soft padding: readable over
		// sky, snow-bright terrain and the dark lair alike.
		Panel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
		Panel->SetPadding(FMargin(14.0f, 10.0f));
	}
	return Panel;
}

UProgressBar* MakeBar(UWidgetTree* Tree, const TCHAR* Name, FLinearColor Fill)
{
	UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
	if (Bar)
	{
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(1.0f);
	}
	return Bar;
}
} // namespace

void UDragonHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}
	UCanvasPanel* Canvas =
		WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	// --- Vitals panel, bottom-left ---
	UBorder* VitalsPanel = MakePanel(WidgetTree, TEXT("HUDVitalsPanel"));
	UVerticalBox* VitalsBox =
		WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HUDVitalsBox"));
	HPText = MakeLabel(WidgetTree, TEXT("HUDHPText"), TEXT("HP"), 13, FLinearColor(1, 0.85f, 0.8f));
	HPBar = MakeBar(WidgetTree, TEXT("HUDHPBar"), FLinearColor(0.85f, 0.18f, 0.12f));
	StaminaText =
		MakeLabel(WidgetTree, TEXT("HUDStaminaText"), TEXT("STAMINA"), 13, FLinearColor(0.8f, 1, 0.85f));
	StaminaBar = MakeBar(WidgetTree, TEXT("HUDStaminaBar"), FLinearColor(0.25f, 0.8f, 0.35f));
	if (VitalsPanel && VitalsBox && HPText && HPBar && StaminaText && StaminaBar)
	{
		VitalsBox->AddChildToVerticalBox(HPText);
		if (UVerticalBoxSlot* BarSlot = VitalsBox->AddChildToVerticalBox(HPBar))
		{
			BarSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 8.0f));
			BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		VitalsBox->AddChildToVerticalBox(StaminaText);
		VitalsBox->AddChildToVerticalBox(StaminaBar);
		VitalsPanel->SetContent(VitalsBox);
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(VitalsPanel))
		{
			Slot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			Slot->SetAlignment(FVector2D(0.0f, 1.0f));
			Slot->SetPosition(FVector2D(24.0f, -24.0f));
			Slot->SetSize(FVector2D(300.0f, 110.0f));
		}
	}

	// --- Hoard panel, top-right ---
	UBorder* HoardPanel = MakePanel(WidgetTree, TEXT("HUDHoardPanel"));
	UVerticalBox* HoardBox =
		WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HUDHoardBox"));
	HoardText = MakeLabel(WidgetTree, TEXT("HUDHoardText"), TEXT("HOARD"), 20,
						  FLinearColor(1.0f, 0.8f, 0.35f));
	StarsText = MakeLabel(WidgetTree, TEXT("HUDStarsText"), TEXT(""), 16,
						  FLinearColor(1.0f, 0.75f, 0.3f));
	WaveText = MakeLabel(WidgetTree, TEXT("HUDWaveText"), TEXT(""), 13,
						 FLinearColor(0.85f, 0.9f, 1.0f));
	if (HoardPanel && HoardBox && HoardText && StarsText && WaveText)
	{
		HoardBox->AddChildToVerticalBox(HoardText);
		HoardBox->AddChildToVerticalBox(StarsText);
		HoardBox->AddChildToVerticalBox(WaveText);
		HoardPanel->SetContent(HoardBox);
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(HoardPanel))
		{
			Slot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
			Slot->SetAlignment(FVector2D(1.0f, 0.0f));
			Slot->SetPosition(FVector2D(-24.0f, 24.0f));
			Slot->SetSize(FVector2D(300.0f, 110.0f));
		}
	}
}

FString UDragonHUDWidget::FormatStars(int32 Stars, int32 Points)
{
	// [2026-11-01 T-019] Five pips, filled for earned stars, with
	// the raw points small beside them (the bucket boundary is a
	// thing players ask about - showing both answers it).
	const int32 Filled = FMath::Clamp(Stars, 0, 5);
	FString Pips;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Pips += (Index < Filled) ? TEXT("\u2605") : TEXT("\u2606");
	}
	return FString::Printf(TEXT("%s  %d pts"), *Pips, Points);
}

FString UDragonHUDWidget::FormatHoard(int64 Hoard, int64 StartingHoard, int32 Wave,
									  const FString& WaveStateText)
{
	// Wave/state render on their own line (WaveText in Refresh);
	// the gold line is gold only.
	(void)Wave;
	(void)WaveStateText;
	return FString::Printf(TEXT("%lld / %lld gold"), Hoard, StartingHoard);
}

float UDragonHUDWidget::BarFraction(float Value, float Max)
{
	if (Max <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(Value / Max, 0.0f, 1.0f);
}

void UDragonHUDWidget::Refresh(float HP, float MaxHP, float Stamina, float MaxStamina, int32 Stars,
							   int32 Points, int64 Hoard, int64 StartingHoard, int32 Wave,
							   const FString& WaveStateText)
{
	if (HPBar)
	{
		HPBar->SetPercent(BarFraction(HP, MaxHP));
	}
	if (StaminaBar)
	{
		StaminaBar->SetPercent(BarFraction(Stamina, MaxStamina));
	}
	if (HPText)
	{
		HPText->SetText(FText::FromString(
			FString::Printf(TEXT("HP  %.0f / %.0f"), HP, MaxHP)));
	}
	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(
			FString::Printf(TEXT("STAMINA  %.0f / %.0f"), Stamina, MaxStamina)));
	}
	if (StarsText)
	{
		StarsText->SetText(FText::FromString(FormatStars(Stars, Points)));
	}
	if (HoardText)
	{
		HoardText->SetText(FText::FromString(FormatHoard(Hoard, StartingHoard, Wave, WaveStateText)));
	}
	if (WaveText)
	{
		WaveText->SetText(FText::FromString(
			FString::Printf(TEXT("Wave %d - %s"), Wave, *WaveStateText)));
	}
}
