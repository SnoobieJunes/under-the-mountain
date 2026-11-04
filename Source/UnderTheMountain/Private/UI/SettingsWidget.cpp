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
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace
{
// [2026-11-04 T-088] Same code-built styling contract as GameFlowWidget.
UButton* MakeSettingsButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
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

UTextBlock* MakeSettingsText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text,
							 int32 Size, FLinearColor Color)
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
	if (Panel != nullptr)
	{
		Panel->SetVisibility(ESlateVisibility::Hidden);
	}
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

	const FLinearColor Ember(0.75f, 0.25f, 0.08f);
	const FLinearColor Slate(0.25f, 0.28f, 0.35f);
	const FLinearColor Gold(1.0f, 0.78f, 0.3f);

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
																TEXT("SettingsPanelBorder"));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
																  TEXT("SettingsPanel"));
	if (PanelBorder == nullptr || Box == nullptr)
	{
		return;
	}
	PanelBorder->SetContent(Box);
	PanelBorder->SetVisibility(ESlateVisibility::Hidden);
	PanelBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
	PanelBorder->SetPadding(FMargin(36.0f, 28.0f));
	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(PanelBorder))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D(0.0f, 0.0f));
		Slot->SetSize(FVector2D(640.0f, 720.0f));
	}
	Panel = PanelBorder;

	Box->AddChild(MakeSettingsText(WidgetTree, TEXT("SettingsHead"), TEXT("SETTINGS"), 32, Gold));

	// [2026-11-04 T-088] One value label + buttons per field; every
	// handler mutates Current then CommitAndRefresh (GameFlow pattern:
	// explicit AddDynamic per button, no generic delegate plumbing).
	auto AddRow = [this, Box](TObjectPtr<UTextBlock>& OutLabel, const TCHAR* LabelName)
	{
		OutLabel = MakeSettingsText(WidgetTree, LabelName, TEXT(""), 16,
									FLinearColor(0.75f, 0.78f, 0.85f));
		Box->AddChild(OutLabel);
	};

	AddRow(ResolutionText, TEXT("SettingsResolution"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsCycleResolution"), TEXT("CYCLE RESOLUTION"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleCycleResolution);
		FirstButton = B;
		Box->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleFullscreen"), TEXT("TOGGLE FULLSCREEN"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleFullscreen);
		Box->AddChild(B);
	}
	AddRow(QualityText, TEXT("SettingsQuality"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsCycleQuality"), TEXT("CYCLE QUALITY"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleCycleQuality);
		Box->AddChild(B);
	}
	AddRow(VolumeText, TEXT("SettingsVolume"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsVolumeDown"), TEXT("VOLUME -"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleVolumeDown);
		Box->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsVolumeUp"), TEXT("VOLUME +"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleVolumeUp);
		Box->AddChild(B);
	}
	AddRow(LookSensitivityText, TEXT("SettingsLookSensitivity"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsLookDown"), TEXT("LOOK SENSITIVITY -"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleLookSensitivityDown);
		Box->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsLookUp"), TEXT("LOOK SENSITIVITY +"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleLookSensitivityUp);
		Box->AddChild(B);
	}
	AddRow(FlightSensitivityText, TEXT("SettingsFlightSensitivity"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsFlightDown"), TEXT("FLIGHT SENSITIVITY -"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleFlightSensitivityDown);
		Box->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsFlightUp"), TEXT("FLIGHT SENSITIVITY +"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleFlightSensitivityUp);
		Box->AddChild(B);
	}
	AddRow(InvertMouseText, TEXT("SettingsInvertMouse"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleInvertMouse"), TEXT("TOGGLE MOUSE INVERT"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInvertMouse);
		Box->AddChild(B);
	}
	AddRow(InvertPadFlightText, TEXT("SettingsInvertPadFlight"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleInvertPadFlight"), TEXT("TOGGLE PAD FLIGHT INVERT"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInvertPadFlight);
		Box->AddChild(B);
	}
	AddRow(InvertPadCameraText, TEXT("SettingsInvertPadCamera"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleInvertPadCamera"), TEXT("TOGGLE PAD CAMERA INVERT"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInvertPadCamera);
		Box->AddChild(B);
	}
	AddRow(CameraText, TEXT("SettingsCamera"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsCycleCamera"), TEXT("CYCLE CAMERA"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleCycleCamera);
		Box->AddChild(B);
	}
	AddRow(InfiniteStaminaText, TEXT("SettingsInfiniteStamina"));
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsToggleStamina"), TEXT("TOGGLE INFINITE STAMINA"), Ember, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleToggleInfiniteStamina);
		Box->AddChild(B);
	}
	if (UButton* B = MakeSettingsButton(WidgetTree, TEXT("SettingsReset"), TEXT("RESET DEFAULTS"), Slate, 16))
	{
		B->OnClicked.AddDynamic(this, &USettingsWidget::HandleResetDefaults);
		Box->AddChild(B);
	}
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
