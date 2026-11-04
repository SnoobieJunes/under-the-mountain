// Copyright Under the Mountain contributors.
// [2026-11-04 T-088] Code-built settings screen: one canvas, one
// panel, one row per FUtmSettings field. Built exactly like
// UGameFlowWidget (flat-colour buttons, centred labels, no WBP).

#include "UI/SettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Reply.h"
#include "UI/MenuStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace
{
// [2026-11-04] Shared menu style with the flow screens (see
// UtmMenuStyle): same buttons, same text, same hover-focus.
UButton* MakeSettingsButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
							FLinearColor Normal, int32 FontSize)
{
	return UtmMenuStyle::MakeButton(Tree, Name, Label, Normal, FontSize);
}

UTextBlock* MakeSettingsText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text,
							 int32 Size, FLinearColor Color, bool bBold = false)
{
	return UtmMenuStyle::MakeText(Tree, Name, Text, Size, Color, bBold);
}
} // namespace

USettingsWidget::USettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// [2026-11-04 T-088] See header (T-112 focusable precedent).
	SetIsFocusable(true);
}

FString USettingsWidget::FormatOnOff(bool bOn)
{
	return bOn ? TEXT("On") : TEXT("Off");
}

FString USettingsWidget::FormatResolution(FIntPoint Resolution, bool bFullscreen)
{
	return FString::Printf(TEXT("%d x %d (%s)"), Resolution.X, Resolution.Y,
						   bFullscreen ? TEXT("Fullscreen") : TEXT("Windowed"));
}

FString USettingsWidget::FormatPercent(float Value01)
{
	return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(FMath::Clamp(Value01, 0.0f, 1.0f) * 100.0f));
}

FString USettingsWidget::FormatSensitivity(float Multiplier)
{
	return FString::Printf(TEXT("%.1fx"), Multiplier);
}

FString USettingsWidget::FormatCamera(EDragonCameraMode Mode)
{
	switch (Mode)
	{
	case EDragonCameraMode::Far:
		return TEXT("Far scenic chase");
	case EDragonCameraMode::Eyes:
		return TEXT("First-person at the dragon's eyes");
	case EDragonCameraMode::Near:
	default:
		return TEXT("Near chase above the wings");
	}
}

void USettingsWidget::ReloadFromSaved()
{
	Current = LoadUtmSettings();
	RefreshLabels();
}

void USettingsWidget::SetSettingsForTesting(const FUtmSettings& InSettings)
{
	Current = SanitizeUtmSettings(InSettings);
	RefreshLabels();
}

void USettingsWidget::ShowSettings()
{
	ReloadFromSaved();
	bSettingsOpen = true;
	if (Panel != nullptr)
	{
		Panel->SetVisibility(ESlateVisibility::Visible);
	}
	// [2026-11-04] Pad navigation starts on the first row's button.
	if (FirstButton != nullptr)
	{
		FirstButton->SetKeyboardFocus();
	}
}

void USettingsWidget::HideSettings()
{
	bSettingsOpen = false;
	if (Panel != nullptr)
	{
		// [2026-11-04] Collapsed, never Hidden/HitTestInvisible:
		// a closed settings panel takes no layout and eats no
		// clicks on the flow panel beneath it (viewport 101 > 100).
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool USettingsWidget::HandleBack()
{
	// [2026-11-04] Back chain step 1 (see header / game mode).
	if (!bSettingsOpen)
	{
		return false;
	}
	HandleClose();
	return true;
}

FReply USettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// [2026-11-04] Esc / Circle from any focused row button bubble
	// here; HandleBack closes and the opener restores its panel.
	if (UtmMenuStyle::IsMenuBackKey(InKeyEvent.GetKey()) && HandleBack())
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USettingsWidget::CommitAndRefresh()
{
	Current = SanitizeUtmSettings(Current);
	ApplyUtmSettings(Current);
	SaveUtmSettings(Current);
	RefreshLabels();
}

void USettingsWidget::RefreshLabels()
{
	auto SetText = [](UTextBlock* Block, const FString& Value)
	{
		if (Block != nullptr)
		{
			Block->SetText(FText::FromString(Value));
		}
	};
	SetText(ResolutionText,
			FString::Printf(TEXT("Resolution: %s"),
							*FormatResolution(Current.Resolution, Current.bFullscreen)));
	SetText(QualityText, FString::Printf(TEXT("Quality: %s"),
										 *GraphicsQualityPresetName(Current.QualityPreset)));
	SetText(VolumeText,
			FString::Printf(TEXT("Master volume: %s"), *FormatPercent(Current.MasterVolume)));
	SetText(LookSensitivityText, FString::Printf(TEXT("Look sensitivity: %s"),
												 *FormatSensitivity(Current.LookSensitivity)));
	SetText(FlightSensitivityText,
			FString::Printf(TEXT("Flight sensitivity: %s"),
							*FormatSensitivity(Current.FlightSensitivity)));
	SetText(InvertMouseText, FString::Printf(TEXT("Invert mouse flight pitch: %s"),
											 *FormatOnOff(Current.bInvertMouseFlightPitch)));
	SetText(InvertPadFlightText,
			FString::Printf(TEXT("Invert gamepad flight pitch: %s"),
							*FormatOnOff(Current.bInvertPadFlightPitch)));
	SetText(InvertPadCameraText,
			FString::Printf(TEXT("Invert gamepad camera pitch: %s"),
							*FormatOnOff(Current.bInvertPadCameraPitch)));
	SetText(CameraText,
			FString::Printf(TEXT("Camera: %s"), *FormatCamera(Current.CameraMode)));
	SetText(InfiniteStaminaText,
			FString::Printf(TEXT("Testing - Infinite Stamina: %s"),
							*FormatOnOff(Current.bInfiniteStamina)));
}

void USettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}
	UCanvasPanel* Canvas =
		WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsCanvas"));
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	const FLinearColor Ember = UtmMenuStyle::Ember();
	const FLinearColor Slate = UtmMenuStyle::Slate();
	const FLinearColor Gold = UtmMenuStyle::Gold();
	const FLinearColor Muted = UtmMenuStyle::Muted();

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
																TEXT("SettingsPanelBorder"));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
																  TEXT("SettingsPanel"));
	if (PanelBorder == nullptr || Box == nullptr)
	{
		return;
	}
	PanelBorder->SetContent(Box);
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
	PanelBorder->SetBrushColor(UtmMenuStyle::PanelTint());
	PanelBorder->SetPadding(FMargin(40.0f, 30.0f));
	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(PanelBorder))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D(0.0f, 0.0f));
		// [2026-11-04] Fixed panel height (560, was 720 and still
		// overflowing): the panel never grows — the scrollbox
		// inside absorbs every row past the fold.
		Slot->SetSize(FVector2D(680.0f, 560.0f));
	}
	Panel = PanelBorder;

	Box->AddChild(MakeSettingsText(WidgetTree, TEXT("SettingsHead"), TEXT("SETTINGS"), 36, Gold, true));
	Box->AddChild(MakeSettingsText(WidgetTree, TEXT("SettingsSub"),
								   TEXT("Display, sound and dragon handling."), 14, Muted));
	UtmMenuStyle::AddDivider(WidgetTree, Box, TEXT("SettingsDivider"));

	// [2026-11-04] Rows scroll inside a UScrollBox that fills the
	// fixed panel between the heading and the pinned Close row.
	// Mouse wheel scrolls it (default consume-when-scrollable),
	// the visible scrollbar drags, and pad navigation scrolls the
	// focused row into view — the bottom rows are reachable by
	// every input, which the bare 720-tall VerticalBox denied.
	SettingsScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),
																TEXT("SettingsScrollBox"));
	UVerticalBox* RowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
																	  TEXT("SettingsRows"));
	if (SettingsScrollBox == nullptr || RowsBox == nullptr)
	{
		return;
	}
	SettingsScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	SettingsScrollBox->AddChild(RowsBox);
	if (UVerticalBoxSlot* ScrollSlot = Box->AddChildToVerticalBox(SettingsScrollBox))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 6.0f));
	}

	// [2026-11-04 T-088] One value label + buttons per field; every
	// handler mutates Current then CommitAndRefresh (GameFlow pattern:
	// explicit AddDynamic per button, no generic delegate plumbing).
	auto AddRow = [this, RowsBox](TObjectPtr<UTextBlock>& OutLabel, const TCHAR* LabelName)
	{
		OutLabel = MakeSettingsText(WidgetTree, LabelName, TEXT(""), 16,
									FLinearColor(0.75f, 0.78f, 0.85f));
		RowsBox->AddChild(OutLabel);
	};

	AddRow(ResolutionText, TEXT("SettingsResolution"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsCycleResolution"), TEXT("CYCLE RESOLUTION"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleCycleResolution);
		FirstButton = B;
		RowsBox->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleFullscreen"), TEXT("TOGGLE FULLSCREEN"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleFullscreen);
		RowsBox->AddChild(B);
	}
	AddRow(QualityText, TEXT("SettingsQuality"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsCycleQuality"), TEXT("CYCLE QUALITY"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleCycleQuality);
		RowsBox->AddChild(B);
	}
	AddRow(VolumeText, TEXT("SettingsVolume"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsVolumeDown"), TEXT("VOLUME -"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleVolumeDown);
		RowsBox->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsVolumeUp"), TEXT("VOLUME +"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleVolumeUp);
		RowsBox->AddChild(B);
	}
	AddRow(LookSensitivityText, TEXT("SettingsLookSensitivity"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsLookDown"), TEXT("LOOK SENSITIVITY -"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleLookSensitivityDown);
		RowsBox->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsLookUp"), TEXT("LOOK SENSITIVITY +"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleLookSensitivityUp);
		RowsBox->AddChild(B);
	}
	AddRow(FlightSensitivityText, TEXT("SettingsFlightSensitivity"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsFlightDown"), TEXT("FLIGHT SENSITIVITY -"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleFlightSensitivityDown);
		RowsBox->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsFlightUp"), TEXT("FLIGHT SENSITIVITY +"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleFlightSensitivityUp);
		RowsBox->AddChild(B);
	}
	AddRow(InvertMouseText, TEXT("SettingsInvertMouse"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleInvertMouse"), TEXT("TOGGLE MOUSE INVERT"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInvertMouse);
		RowsBox->AddChild(B);
	}
	AddRow(InvertPadFlightText, TEXT("SettingsInvertPadFlight"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleInvertPadFlight"), TEXT("TOGGLE PAD FLIGHT INVERT"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInvertPadFlight);
		RowsBox->AddChild(B);
	}
	AddRow(InvertPadCameraText, TEXT("SettingsInvertPadCamera"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleInvertPadCamera"), TEXT("TOGGLE PAD CAMERA INVERT"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInvertPadCamera);
		RowsBox->AddChild(B);
	}
	AddRow(CameraText, TEXT("SettingsCamera"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsCycleCamera"), TEXT("CYCLE CAMERA"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleCycleCamera);
		RowsBox->AddChild(B);
	}
	AddRow(InfiniteStaminaText, TEXT("SettingsInfiniteStamina"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleStamina"), TEXT("TOGGLE INFINITE STAMINA"), Ember, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInfiniteStamina);
		RowsBox->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsReset"), TEXT("RESET DEFAULTS"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleResetDefaults);
		RowsBox->AddChild(B);
	}
	// [2026-11-04] Close is pinned BELOW the scrollbox (never
	// scrolled away) with the DualSense footer hint above it.
	Box->AddChild(MakeSettingsText(WidgetTree, TEXT("SettingsFooter"),
								   TEXT("Cross select  /  Circle back  /  Esc back"), 13, Muted));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsClose"), TEXT("CLOSE"), Ember, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleClose);
		Box->AddChild(B);
	}

	RefreshLabels();
	HideSettings();
}

void USettingsWidget::HandleCycleResolution()
{
	const TArray<FIntPoint> Choices = UtmSettingsResolutionChoices();
	const int32 Index = Choices.Find(Current.Resolution);
	Current.Resolution = Choices[(Index + 1) % Choices.Num()];
	CommitAndRefresh();
}

void USettingsWidget::HandleToggleFullscreen()
{
	Current.bFullscreen = !Current.bFullscreen;
	CommitAndRefresh();
}

void USettingsWidget::HandleCycleQuality()
{
	const int32 Next = (static_cast<int32>(Current.QualityPreset) + 1) %
					   (static_cast<int32>(EGraphicsQualityPreset::Ultra) + 1);
	Current.QualityPreset = static_cast<EGraphicsQualityPreset>(Next);
	CommitAndRefresh();
}

void USettingsWidget::HandleVolumeDown()
{
	Current.MasterVolume -= 0.1f;
	CommitAndRefresh();
}

void USettingsWidget::HandleVolumeUp()
{
	Current.MasterVolume += 0.1f;
	CommitAndRefresh();
}

void USettingsWidget::HandleLookSensitivityDown()
{
	Current.LookSensitivity -= 0.1f;
	CommitAndRefresh();
}

void USettingsWidget::HandleLookSensitivityUp()
{
	Current.LookSensitivity += 0.1f;
	CommitAndRefresh();
}

void USettingsWidget::HandleFlightSensitivityDown()
{
	Current.FlightSensitivity -= 0.1f;
	CommitAndRefresh();
}

void USettingsWidget::HandleFlightSensitivityUp()
{
	Current.FlightSensitivity += 0.1f;
	CommitAndRefresh();
}

void USettingsWidget::HandleToggleInvertMouse()
{
	Current.bInvertMouseFlightPitch = !Current.bInvertMouseFlightPitch;
	CommitAndRefresh();
}

void USettingsWidget::HandleToggleInvertPadFlight()
{
	Current.bInvertPadFlightPitch = !Current.bInvertPadFlightPitch;
	CommitAndRefresh();
}

void USettingsWidget::HandleToggleInvertPadCamera()
{
	Current.bInvertPadCameraPitch = !Current.bInvertPadCameraPitch;
	CommitAndRefresh();
}

void USettingsWidget::HandleCycleCamera()
{
	const int32 Next = (static_cast<int32>(Current.CameraMode) + 1) %
					   (static_cast<int32>(EDragonCameraMode::Eyes) + 1);
	Current.CameraMode = static_cast<EDragonCameraMode>(Next);
	CommitAndRefresh();
}

void USettingsWidget::HandleToggleInfiniteStamina()
{
	Current.bInfiniteStamina = !Current.bInfiniteStamina;
	CommitAndRefresh();
}

void USettingsWidget::HandleResetDefaults()
{
	Current = DefaultUtmSettings();
	CommitAndRefresh();
}

void USettingsWidget::HandleClose()
{
	HideSettings();
	// [2026-11-04] The opener (title/pause via UGameFlowWidget) restores
	// its panel and pad focus from this broadcast.
	OnClosed.Broadcast();
}
