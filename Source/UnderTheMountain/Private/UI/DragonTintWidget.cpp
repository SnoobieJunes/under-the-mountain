// Copyright Under the Mountain contributors.
// [2026-10-28 T-027] Code-built tint picker. Constructing the rows in
// C++ (instead of a WBP asset) keeps the picker reviewable in the same
// diff as the component it drives; restyling later can move it to a
// Blueprint subclass without changing this class's API.

#include "UI/DragonTintWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Dragon/DragonAppearanceComponent.h"
#include "GameFramework/Pawn.h"

void UDragonTintWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// [2026-10-28 T-027] WidgetTree can be null for a widget created
	// outside the normal CreateWidget path; building is skipped then
	// and every other method here already null-guards the sliders.
	if (!WidgetTree)
	{
		return;
	}

	UVerticalBox* Root =
		WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TintRoot"));
	if (!Root)
	{
		return;
	}
	WidgetTree->RootWidget = Root;

	RedSlider = BuildSliderRow(Root, TEXT("Scale R"), TEXT("ScaleRSlider"));
	GreenSlider = BuildSliderRow(Root, TEXT("Scale G"), TEXT("ScaleGSlider"));
	BlueSlider = BuildSliderRow(Root, TEXT("Scale B"), TEXT("ScaleBSlider"));

	// [2026-10-28 T-027] Show the dragon's actual tint on open rather
	// than the sliders' zero defaults. Harmless when no component is
	// reachable yet — BindToAppearance refreshes again.
	InitializeFromCurrent();
}

USlider* UDragonTintWidget::BuildSliderRow(UVerticalBox* Parent, const FString& Label, FName SliderName)
{
	UHorizontalBox* Row =
		WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), SliderName);
	if (!Row || !LabelBlock || !Slider)
	{
		return nullptr;
	}

	LabelBlock->SetText(FText::FromString(Label));

	Slider->SetMinValue(0.0f);
	Slider->SetMaxValue(1.0f);
	Slider->SetStepSize(0.01f);
	Slider->SetValue(0.0f);
	Slider->OnValueChanged.AddDynamic(this, &UDragonTintWidget::HandleSliderValueChanged);

	Row->AddChildToHorizontalBox(LabelBlock);
	Row->AddChildToHorizontalBox(Slider);
	Parent->AddChildToVerticalBox(Row);
	return Slider;
}

void UDragonTintWidget::BindToAppearance(UDragonAppearanceComponent* Appearance)
{
	BoundAppearance = Appearance;
	InitializeFromCurrent();
}

void UDragonTintWidget::InitializeFromCurrent()
{
	UDragonAppearanceComponent* Appearance = ResolveAppearance();
	if (!Appearance || !RedSlider || !GreenSlider || !BlueSlider)
	{
		return;
	}

	// [2026-10-28 T-027] Guard flag: writes below must not round-trip
	// through HandleSliderValueChanged into a save of the value we
	// just loaded.
	bInitializing = true;
	const FLinearColor Tint = Appearance->GetScaleTint();
	RedSlider->SetValue(Tint.R);
	GreenSlider->SetValue(Tint.G);
	BlueSlider->SetValue(Tint.B);
	bInitializing = false;
}

void UDragonTintWidget::HandleSliderValueChanged(float Value)
{
	// [2026-10-28 T-027] Value is the moved slider's own channel; the
	// tint is recomposed from all three sliders instead so the three
	// bindings can share this one handler.
	(void)Value;
	if (bInitializing || !RedSlider || !GreenSlider || !BlueSlider)
	{
		return;
	}
	UDragonAppearanceComponent* Appearance = ResolveAppearance();
	if (!Appearance)
	{
		return;
	}
	// [2026-10-28 T-027] Default bSave=true: a drag persists as it
	// goes, matching SetScaleTint's contract. The component clamps,
	// so raw slider values are safe to pass through.
	Appearance->SetScaleTint(
		FLinearColor(RedSlider->GetValue(), GreenSlider->GetValue(), BlueSlider->GetValue(), 1.0f));
}

UDragonAppearanceComponent* UDragonTintWidget::ResolveAppearance()
{
	if (BoundAppearance.IsValid())
	{
		return BoundAppearance.Get();
	}
	// [2026-10-28 T-027] Fallback for widgets opened without an
	// explicit bind: the player's pawn carries the component.
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UDragonAppearanceComponent>();
	}
	return nullptr;
}
