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
	// [2026-11-03 T-064] Flight-state line under the stamina bar (Dive /
	// Hover / Brake / Glide in player terms); collapsed while grounded.
	FlightStateText = MakeLabel(WidgetTree, TEXT("HUDFlightStateText"), TEXT(""), 13,
								FLinearColor(1.0f, 0.9f, 0.55f));
	// [2026-11-04 T-124] Testing badge under the flight-state line (god
	// mode only); collapsed while off. Magenta reads as neither HP
	// (crimson) nor stamina (green) nor flight (gold).
	TestingBadgeText = MakeLabel(WidgetTree, TEXT("HUDTestingBadgeText"), TEXT(""), 13,
								 FLinearColor(1.0f, 0.45f, 0.9f));
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
		if (FlightStateText != nullptr)
		{
			VitalsBox->AddChildToVerticalBox(FlightStateText);
		}
		if (TestingBadgeText != nullptr)
		{
			VitalsBox->AddChildToVerticalBox(TestingBadgeText);
		}
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

FString UDragonHUDWidget::FormatFlightState(bool bDiving, bool bBraking, bool bHovering)
{
	// [2026-11-03 T-064] Player-terms flight line. A dive always wins:
	// speed into terrain is the urgent state, and its line carries the
	// recovery ("pull up or Brake"). Glide names its own entry ("nose
	// down to dive") so the verb is taught, not just labelled.
	if (bDiving)
	{
		return TEXT("DIVING - pull up or Brake");
	}
	if (bBraking)
	{
		return TEXT("BRAKING");
	}
	if (bHovering)
	{
		return TEXT("HOVER - precision hold");
	}
	return TEXT("GLIDING - nose down to dive");
}

FString UDragonHUDWidget::FormatTestingBadge(bool bInfiniteStamina)
{
	// [2026-11-04 T-124] The badge names what is on: a tester seeing a
	// full stamina bar needs to know it is the cvar, not the economy.
	return bInfiniteStamina ? TEXT("TESTING - INFINITE STAMINA") : TEXT("");
}

FString UDragonHUDWidget::FormatHoard(int64 Hoard, int64 StartingHoard, int32 Wave, int32 XP,
									  const FString& WaveStateText)
{
	// Wave/state render on their own line (WaveText in Refresh);
	// the gold line is gold plus the M1 XP readout (T-083).
	(void)Wave;
	(void)WaveStateText;
	return FString::Printf(TEXT("%lld / %lld gold \u00b7 %d XP"), Hoard, StartingHoard, XP);
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
							   int32 Points, int64 Hoard, int64 StartingHoard, int32 Wave, int32 XP,
							   const FString& WaveStateText, const FString& FlightStateLabel,
							   const FString& TestingBadgeLabel)
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
		HoardText->SetText(FText::FromString(FormatHoard(Hoard, StartingHoard, Wave, XP, WaveStateText)));
	}
	if (WaveText)
	{
		WaveText->SetText(FText::FromString(
			FString::Printf(TEXT("Wave %d - %s"), Wave, *WaveStateText)));
	}
	if (FlightStateText)
	{
		// [2026-11-03 T-064] Empty (grounded) collapses the line; airborne
		// always shows exactly one state.
		FlightStateText->SetText(FText::FromString(FlightStateLabel));
		FlightStateText->SetVisibility(FlightStateLabel.IsEmpty() ? ESlateVisibility::Collapsed
																  : ESlateVisibility::Visible);
	}
	if (TestingBadgeText)
	{
		// [2026-11-04 T-124] Same contract: empty (god mode off) collapses.
		TestingBadgeText->SetText(FText::FromString(TestingBadgeLabel));
		TestingBadgeText->SetVisibility(TestingBadgeLabel.IsEmpty() ? ESlateVisibility::Collapsed
																	: ESlateVisibility::Visible);
	}
}
