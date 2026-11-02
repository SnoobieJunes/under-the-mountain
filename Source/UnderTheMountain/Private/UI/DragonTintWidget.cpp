// Copyright Under the Mountain contributors.
// [2026-11-01 T-060] Code-built HSV tint picker: one draggable
// saturation/value pad plus a hue strip, replacing the T-027 three-
// slider stack. Constructing in C++ (instead of a WBP asset) keeps
// the picker reviewable in the same diff as the component it drives;
// the pad/strip art is a generated gradient texture, so no Content/
// assets are needed. Every drag move writes through
// UDragonAppearanceComponent::SetScaleTint with its default save, so
// the dragon recolours live and the choice persists to the same slot
// the sliders used — existing saves load unchanged.

#include "UI/DragonTintWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Dragon/DragonAppearanceComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"

namespace
{
// [2026-11-01 T-060] Pad/strip texture resolution. Chosen so a hue
// drag (which redraws the whole pad) stays well under a frame while
// banding stays invisible at the picker's on-screen size.
constexpr int32 PadResolution = 128;
constexpr int32 StripWidth = 256;
constexpr int32 StripHeight = 16;

// [2026-11-01 T-060] Fill a transient texture's first mip from a
// BGRA pixel array and push it to the GPU. The textures are picker
// UI only and never saved, so Transient + never-stream is correct.
void UploadPixels(UTexture2D* Texture, const TArray<FColor>& Pixels)
{
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty())
	{
		return;
	}
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (Data)
	{
		FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	}
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
}
} // namespace

FLinearColor UDragonTintWidget::HsvToRgb(float InHueDegrees, float InSaturation, float InValue)
{
	// [2026-11-01 T-060] Wrap before clamp: hue is circular (360 == 0,
	// -120 == 240) while S/V are ranges and clamp to [0,1].
	float Hue = FMath::Fmod(InHueDegrees, 360.0f);
	if (Hue < 0.0f)
	{
		Hue += 360.0f;
	}
	const float Saturation = FMath::Clamp(InSaturation, 0.0f, 1.0f);
	const float Value = FMath::Clamp(InValue, 0.0f, 1.0f);

	const float Chroma = Value * Saturation;
	const float HueSector = Hue / 60.0f;
	const float X = Chroma * (1.0f - FMath::Abs(FMath::Fmod(HueSector, 2.0f) - 1.0f));
	const float Match = Value - Chroma;

	float R = 0.0f, G = 0.0f, B = 0.0f;
	if (HueSector < 1.0f)
	{
		R = Chroma;
		G = X;
	}
	else if (HueSector < 2.0f)
	{
		R = X;
		G = Chroma;
	}
	else if (HueSector < 3.0f)
	{
		G = Chroma;
		B = X;
	}
	else if (HueSector < 4.0f)
	{
		G = X;
		B = Chroma;
	}
	else if (HueSector < 5.0f)
	{
		R = X;
		B = Chroma;
	}
	else
	{
		R = Chroma;
		B = X;
	}
	return FLinearColor(R + Match, G + Match, B + Match, 1.0f);
}

void UDragonTintWidget::RgbToHsv(FLinearColor Colour, float& OutHueDegrees, float& OutSaturation,
								 float& OutValue)
{
	const float Max = FMath::Max3(Colour.R, Colour.G, Colour.B);
	const float Min = FMath::Min3(Colour.R, Colour.G, Colour.B);
	const float Delta = Max - Min;

	OutValue = Max;
	// [2026-11-01 T-060] Grey (Delta == 0) has no hue or saturation;
	// report 0 for both so a grey tint opens with the cursor on the
	// pad's left edge instead of at a phantom hue.
	OutSaturation = (Max > 0.0f) ? (Delta / Max) : 0.0f;
	if (Delta <= 0.0f)
	{
		OutHueDegrees = 0.0f;
		return;
	}
	float Hue;
	if (Max == Colour.R)
	{
		Hue = 60.0f * FMath::Fmod((Colour.G - Colour.B) / Delta, 6.0f);
	}
	else if (Max == Colour.G)
	{
		Hue = 60.0f * (((Colour.B - Colour.R) / Delta) + 2.0f);
	}
	else
	{
		Hue = 60.0f * (((Colour.R - Colour.G) / Delta) + 4.0f);
	}
	if (Hue < 0.0f)
	{
		Hue += 360.0f;
	}
	OutHueDegrees = Hue;
}

FLinearColor UDragonTintWidget::ColourForPadPosition(float InHueDegrees, float PadX, float PadY)
{
	// [2026-11-01 T-060] Pad space: X is saturation left-to-right,
	// Y is value top-to-bottom (screen Y grows downward, value does
	// not), so (0,0) is white and the bottom edge is black.
	return HsvToRgb(InHueDegrees, FMath::Clamp(PadX, 0.0f, 1.0f),
					1.0f - FMath::Clamp(PadY, 0.0f, 1.0f));
}

float UDragonTintWidget::HueForStripPosition(float StripX)
{
	// [2026-11-01 T-060] The right edge is hue 360, which wraps to 0
	// so the strip's two ends are the same red — matching the drawn
	// gradient and keeping a clamped off-strip drag on red, not on a
	// nonexistent 361st degree.
	const float Hue = FMath::Clamp(StripX, 0.0f, 1.0f) * 360.0f;
	return FMath::Fmod(Hue, 360.0f);
}

void UDragonTintWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// [2026-10-28 T-027] WidgetTree can be null for a widget created
	// outside the normal CreateWidget path; building is skipped then
	// and every other method here already null-guards its controls.
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

	UTextBlock* Title =
		WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TintTitle"));
	PadImage =
		WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TintPad"));
	StripImage =
		WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TintHueStrip"));
	PreviewSwatch =
		WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TintPreview"));
	if (!Title || !PadImage || !StripImage || !PreviewSwatch)
	{
		return;
	}

	Title->SetText(FText::FromString(TEXT("Scale colour — drag the pad, slide the hue")));
	PadImage->SetDesiredSizeOverride(FVector2D(240.0f, 180.0f));
	StripImage->SetDesiredSizeOverride(FVector2D(240.0f, 24.0f));
	PreviewSwatch->SetBrushColor(HsvToRgb(HueDegrees, Saturation, Value));
	// [2026-11-01 T-060] UBorder has no desired-size override (that is
	// a UImage affordance); a text child gives the swatch its height.
	UTextBlock* PreviewLabel =
		WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TintPreviewLabel"));
	if (PreviewLabel)
	{
		PreviewLabel->SetText(FText::FromString(TEXT("Preview")));
		PreviewSwatch->SetContent(PreviewLabel);
	}

	Root->AddChildToVerticalBox(Title);
	Root->AddChildToVerticalBox(PadImage);
	Root->AddChildToVerticalBox(StripImage);
	Root->AddChildToVerticalBox(PreviewSwatch);

	BuildPickerTextures();
	if (PadTexture)
	{
		PadImage->SetBrushFromTexture(PadTexture);
	}
	if (StripTexture)
	{
		StripImage->SetBrushFromTexture(StripTexture);
	}

	// [2026-11-01 T-060] Show the dragon's actual tint on open rather
	// than the cursor defaults. Harmless when no component is
	// reachable yet — BindToAppearance refreshes again.
	InitializeFromCurrent();
}

void UDragonTintWidget::BuildPickerTextures()
{
	PadTexture = UTexture2D::CreateTransient(PadResolution, PadResolution, PF_B8G8R8A8);
	StripTexture = UTexture2D::CreateTransient(StripWidth, StripHeight, PF_B8G8R8A8);
	if (StripTexture)
	{
		// [2026-11-01 T-060] The strip never changes after this: every
		// pixel in a column shares the column's hue at S=V=1.
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(StripWidth * StripHeight);
		for (int32 X = 0; X < StripWidth; ++X)
		{
			const FColor Column =
				HsvToRgb(HueForStripPosition(static_cast<float>(X) / (StripWidth - 1)), 1.0f, 1.0f)
					.ToFColor(false);
			for (int32 Y = 0; Y < StripHeight; ++Y)
			{
				Pixels[Y * StripWidth + X] = Column;
			}
		}
		UploadPixels(StripTexture, Pixels);
	}
	RedrawPadTexture();
}

void UDragonTintWidget::RedrawPadTexture()
{
	if (!PadTexture)
	{
		return;
	}
	// [2026-11-01 T-060] Drawn through ColourForPadPosition so the
	// texture the player sees is the same mapping the drag applies —
	// the tint under the cursor is the tint the dragon gets.
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(PadResolution * PadResolution);
	for (int32 Y = 0; Y < PadResolution; ++Y)
	{
		for (int32 X = 0; X < PadResolution; ++X)
		{
			const FLinearColor Colour = ColourForPadPosition(
				HueDegrees, static_cast<float>(X) / (PadResolution - 1),
				static_cast<float>(Y) / (PadResolution - 1));
			Pixels[Y * PadResolution + X] = Colour.ToFColor(false);
		}
	}
	UploadPixels(PadTexture, Pixels);
}

void UDragonTintWidget::BindToAppearance(UDragonAppearanceComponent* Appearance)
{
	BoundAppearance = Appearance;
	InitializeFromCurrent();
}

void UDragonTintWidget::InitializeFromCurrent()
{
	UDragonAppearanceComponent* Appearance = ResolveAppearance();
	if (!Appearance)
	{
		return;
	}
	// [2026-11-01 T-060] Cursor follows the component (never the
	// reverse): opening the picker on a saved blue dragon shows a
	// blue cursor, and nothing is written or saved by opening.
	RgbToHsv(Appearance->GetScaleTint(), HueDegrees, Saturation, Value);
	RedrawPadTexture();
	if (PreviewSwatch)
	{
		PreviewSwatch->SetBrushColor(HsvToRgb(HueDegrees, Saturation, Value));
	}
}

void UDragonTintWidget::ApplyPadPosition(FVector2D LocalPosition, FVector2D LocalSize)
{
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return;
	}
	// [2026-11-01 T-060] Keep the cursor in HSV (not the clamped RGB)
	// so the next strip drag starts from where the finger actually
	// is; positions clamp inside ColourForPadPosition already.
	Saturation = FMath::Clamp(LocalPosition.X / LocalSize.X, 0.0f, 1.0f);
	Value = 1.0f - FMath::Clamp(LocalPosition.Y / LocalSize.Y, 0.0f, 1.0f);
	WriteThrough();
}

void UDragonTintWidget::ApplyStripPosition(FVector2D LocalPosition, FVector2D LocalSize)
{
	if (LocalSize.X <= 0.0f)
	{
		return;
	}
	HueDegrees = HueForStripPosition(LocalPosition.X / LocalSize.X);
	// [2026-11-01 T-060] A hue change recolours every pad pixel.
	RedrawPadTexture();
	WriteThrough();
}

void UDragonTintWidget::WriteThrough()
{
	if (PreviewSwatch)
	{
		PreviewSwatch->SetBrushColor(HsvToRgb(HueDegrees, Saturation, Value));
	}
	UDragonAppearanceComponent* Appearance = ResolveAppearance();
	if (!Appearance)
	{
		return;
	}
	// [2026-11-01 T-060] Default bSave=true, as with the old sliders:
	// a drag previews live on the dragon and persists as it goes, to
	// the same slot existing saves already use. The component clamps.
	Appearance->SetScaleTint(HsvToRgb(HueDegrees, Saturation, Value));
}

FReply UDragonTintWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
												  const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	// [2026-11-01 T-060] Hit-test pad first, then strip, in each
	// control's own cached geometry — the press point decides the
	// whole gesture (DragTarget), so crossing the other control
	// mid-drag never switches what is being edited.
	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (PadImage)
	{
		const FGeometry PadGeometry = PadImage->GetCachedGeometry();
		const FVector2D Local = PadGeometry.AbsoluteToLocal(ScreenPosition);
		if (Local.X >= 0.0f && Local.Y >= 0.0f && Local.X <= PadGeometry.GetLocalSize().X &&
			Local.Y <= PadGeometry.GetLocalSize().Y)
		{
			DragTarget = EPickTarget::Pad;
			ApplyPadPosition(Local, PadGeometry.GetLocalSize());
			return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
		}
	}
	if (StripImage)
	{
		const FGeometry StripGeometry = StripImage->GetCachedGeometry();
		const FVector2D Local = StripGeometry.AbsoluteToLocal(ScreenPosition);
		if (Local.X >= 0.0f && Local.Y >= 0.0f && Local.X <= StripGeometry.GetLocalSize().X &&
			Local.Y <= StripGeometry.GetLocalSize().Y)
		{
			DragTarget = EPickTarget::Strip;
			ApplyStripPosition(Local, StripGeometry.GetLocalSize());
			return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UDragonTintWidget::NativeOnMouseMove(const FGeometry& InGeometry,
											const FPointerEvent& InMouseEvent)
{
	if (DragTarget == EPickTarget::None)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	// [2026-11-01 T-060] Positions are recomputed against the target
	// control's cached geometry (mouse capture keeps events flowing
	// even off-widget), and the mapping functions clamp, so dragging
	// past an edge pins the cursor to that edge instead of losing it.
	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (DragTarget == EPickTarget::Pad && PadImage)
	{
		const FGeometry PadGeometry = PadImage->GetCachedGeometry();
		ApplyPadPosition(PadGeometry.AbsoluteToLocal(ScreenPosition), PadGeometry.GetLocalSize());
	}
	else if (DragTarget == EPickTarget::Strip && StripImage)
	{
		const FGeometry StripGeometry = StripImage->GetCachedGeometry();
		ApplyStripPosition(StripGeometry.AbsoluteToLocal(ScreenPosition),
						   StripGeometry.GetLocalSize());
	}
	return FReply::Handled();
}

FReply UDragonTintWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
												const FPointerEvent& InMouseEvent)
{
	if (DragTarget != EPickTarget::None &&
		InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		DragTarget = EPickTarget::None;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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
