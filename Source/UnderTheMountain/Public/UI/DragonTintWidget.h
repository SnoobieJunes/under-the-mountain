// Copyright Under the Mountain contributors.
// Purpose: the scale-colour picker — three code-built sliders (R/G/B)
// that write straight to UDragonAppearanceComponent::SetScaleTint.
// Created 2026-10-28 (T-027).
// Invariants: this widget owns no tint state; the component is the
// single source of truth and the sliders are a view of it. The widget
// never touches materials or the save slot itself.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragonTintWidget.generated.h"

class UDragonAppearanceComponent;
class USlider;
class UVerticalBox;

UCLASS()
class UNDERTHEMOUNTAIN_API UDragonTintWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-027] Point the sliders at a specific component
	// (weak ref: the widget must never keep a pawn's component alive)
	// and refresh the slider positions from it. Pass nullptr to fall
	// back to looking the component up on the owning player pawn.
	void BindToAppearance(UDragonAppearanceComponent* Appearance);

	// [2026-10-28 T-027] Set the three sliders from the bound (or
	// pawn-found) component's current tint, without firing the
	// value-changed handlers — programmatic refresh must not re-save
	// the value it just read.
	void InitializeFromCurrent();

  protected:
	// [2026-10-28 T-027] Builds the whole UI in code (no Blueprint
	// asset to keep in sync): a vertical stack of three labelled
	// slider rows.
	virtual void NativeOnInitialized() override;

  private:
	// [2026-10-28 T-027] One handler for all three sliders: recompose
	// the tint from every slider's current value and hand it to the
	// component, which clamps, applies and saves.
	UFUNCTION()
	void HandleSliderValueChanged(float Value);

	// [2026-10-28 T-027] Bound component if still valid, else the
	// owning pawn's appearance component, else nullptr.
	UDragonAppearanceComponent* ResolveAppearance();

	// [2026-10-28 T-027] Build one "label + slider" row into Parent
	// and return the slider. Range 0..1, step 0.01 — fine enough for
	// colour work without a numeric entry box.
	USlider* BuildSliderRow(UVerticalBox* Parent, const FString& Label, FName SliderName);

	UPROPERTY(Transient)
	TObjectPtr<USlider> RedSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> GreenSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> BlueSlider;

	// [2026-10-28 T-027] Weak on purpose (see BindToAppearance).
	TWeakObjectPtr<UDragonAppearanceComponent> BoundAppearance;

	// [2026-10-28 T-027] True while InitializeFromCurrent is writing
	// slider values, so the handler ignores that refresh (no save
	// spam, no feedback loop).
	bool bInitializing = false;
};
