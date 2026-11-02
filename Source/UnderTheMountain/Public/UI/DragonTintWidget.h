// Copyright Under the Mountain contributors.
// Purpose: the scale-colour picker — one draggable saturation/value pad
// plus a hue strip (T-060) that writes straight to
// UDragonAppearanceComponent::SetScaleTint. Replaces the three R/G/B
// sliders built for T-027 on 2026-10-28; owner playtest found three
// sliders clumsy next to the one draggable picker players expect.
// Created 2026-10-28 (T-027); picker rework 2026-11-01 (T-060).
// Invariants: this widget owns no tint state; the component is the
// single source of truth. The Hue/Saturation/Value members below are
// only the picker's cursor position (where the player's finger is),
// re-derived from the component on open and written straight back to
// it on every drag move. The widget never touches materials or the
// save slot itself.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragonTintWidget.generated.h"

class UDragonAppearanceComponent;
class UBorder;
class UImage;
class UTexture2D;
class UVerticalBox;

UCLASS()
class UNDERTHEMOUNTAIN_API UDragonTintWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-060] HSV (hue in degrees, S/V in [0,1]) to linear
	// RGB with alpha 1. Hue wraps (360 is 0, -120 is 240), S/V clamp —
	// hostile input from a drag outside the pad can never produce an
	// out-of-gamut colour. Pure function: this is the exact mapping
	// the drag handlers use, so the Dragon.Tint tests exercise the
	// shipped math, not a copy of it.
	static FLinearColor HsvToRgb(float HueDegrees, float Saturation, float Value);

	// [2026-11-01 T-060] Inverse of HsvToRgb. Greys report S=0 and
	// keep their value, so dragging through grey does not invent a
	// hue on the way back out. Hue is in [0, 360).
	static void RgbToHsv(FLinearColor Colour, float& OutHueDegrees, float& OutSaturation,
						 float& OutValue);

	// [2026-11-01 T-060] The pad mapping: PadX is saturation (left 0,
	// right 1), PadY is value inverted (top 1, bottom 0), so the
	// top-left corner is white and the bottom edge is black at any
	// hue. Out-of-range positions clamp to the nearest edge.
	static FLinearColor ColourForPadPosition(float HueDegrees, float PadX, float PadY);

	// [2026-11-01 T-060] The hue-strip mapping: StripX in [0,1] sweeps
	// 0..360 degrees and the right edge wraps back to hue 0, matching
	// the strip texture drawn left-to-right through the spectrum.
	static float HueForStripPosition(float StripX);

	// [2026-10-28 T-027] Point the picker at a specific component
	// (weak ref: the widget must never keep a pawn's component alive)
	// and refresh the picker cursor from it. Pass nullptr to fall
	// back to looking the component up on the owning player pawn.
	void BindToAppearance(UDragonAppearanceComponent* Appearance);

	// [2026-11-01 T-060] Set the picker cursor (hue + pad position +
	// preview swatch) from the bound (or pawn-found) component's
	// current tint, without writing anything back — a programmatic
	// refresh must not re-save the value it just read.
	void InitializeFromCurrent();

  protected:
	// [2026-11-01 T-060] Builds the whole UI in code (no Blueprint
	// asset to keep in sync): label, the SV pad image, the hue strip
	// image, and a preview swatch. Pad/strip are generated textures
	// (see BuildPickerTextures) rather than materials, so the picker
	// needs no new Content/ assets.
	virtual void NativeOnInitialized() override;

	// [2026-11-01 T-060] Drag entry: press inside the pad starts a
	// pad drag, inside the strip a hue drag; the mouse is captured so
	// a fast drag outside the widget still tracks until release.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,
										   const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry,
									 const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry,
										 const FPointerEvent& InMouseEvent) override;

  private:
	// [2026-11-01 T-060] Which control a live drag belongs to; a drag
	// that starts on the pad never jumps to the strip mid-gesture.
	enum class EPickTarget : uint8
	{
		None,
		Pad,
		Strip
	};

	// [2026-10-28 T-027] Bound component if still valid, else the
	// owning pawn's appearance component, else nullptr.
	UDragonAppearanceComponent* ResolveAppearance();

	// [2026-11-01 T-060] (Re)build the two gradient textures. The pad
	// is redrawn whenever the hue changes (every pad pixel depends on
	// it); the strip is hue-only and built once. 128px pad / 256px
	// strip: fine enough that banding is invisible at picker size,
	// small enough that a hue drag redraws it in well under a frame.
	void BuildPickerTextures();
	void RedrawPadTexture();

	// [2026-11-01 T-060] Apply a pad/strip pointer position: update
	// the cursor, redraw what changed, refresh the swatch, and write
	// through to the component (which clamps, applies and saves).
	// LocalPosition is in the target control's own geometry space.
	void ApplyPadPosition(FVector2D LocalPosition, FVector2D LocalSize);
	void ApplyStripPosition(FVector2D LocalPosition, FVector2D LocalSize);
	void WriteThrough();

	UPROPERTY(Transient)
	TObjectPtr<UImage> PadImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> StripImage;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PreviewSwatch;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PadTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> StripTexture;

	// [2026-10-28 T-027] Weak on purpose (see BindToAppearance).
	TWeakObjectPtr<UDragonAppearanceComponent> BoundAppearance;

	// [2026-11-01 T-060] Picker cursor only — see the file header.
	// The component's tint remains the single source of truth.
	float HueDegrees = 0.0f;
	float Saturation = 1.0f;
	float Value = 1.0f;

	EPickTarget DragTarget = EPickTarget::None;
};
