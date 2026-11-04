// Copyright Under the Mountain contributors.
// [2026-11-04] Shared code-built menu styling (see MenuStyle.h).

#include "UI/MenuStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

void UMenuButton::HandleHoverFocus()
{
	SetKeyboardFocus();
}

namespace UtmMenuStyle
{
FLinearColor Gold()
{
	return FLinearColor(1.0f, 0.78f, 0.3f);
}

FLinearColor Ember()
{
	return FLinearColor(0.75f, 0.25f, 0.08f);
}

FLinearColor Slate()
{
	return FLinearColor(0.25f, 0.28f, 0.35f);
}

FLinearColor Muted()
{
	return FLinearColor(0.75f, 0.78f, 0.85f);
}

FLinearColor PanelTint()
{
	return FLinearColor(0.02f, 0.02f, 0.03f, 0.78f);
}

bool IsMenuBackKey(const FKey& Key)
{
	// [2026-11-04] One Back definition (see header): Esc on the
	// keyboard, Circle (FaceButton_Right) on DualSense.
	return Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
}

UButton* MakeButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
					FLinearColor Normal, int32 FontSize)
{
	if (Tree == nullptr)
	{
		return nullptr;
	}
	UMenuButton* Button = Tree->ConstructWidget<UMenuButton>(UMenuButton::StaticClass(), Name);
	if (Button == nullptr)
	{
		return nullptr;
	}
	FButtonStyle Style = Button->GetStyle();
	// [2026-11-04] Distinct states a player can tell apart at a
	// glance: hover lifts toward gold-white, press sinks dark.
	const FLinearColor Hovered(
		FMath::Min(1.0f, Normal.R + 0.22f), FMath::Min(1.0f, Normal.G + 0.20f),
		FMath::Min(1.0f, Normal.B + 0.16f), Normal.A);
	Style.Normal = FSlateColorBrush(Normal);
	Style.Hovered = FSlateColorBrush(Hovered);
	Style.Pressed = FSlateColorBrush(Normal * 0.55f);
	Style.Disabled = FSlateColorBrush(Normal * 0.4f);
	Style.NormalPadding = FMargin(22.0f, 13.0f);
	Style.PressedPadding = FMargin(22.0f, 13.0f);
	Button->SetStyle(Style);
	// [2026-11-04] Hover-focus: mouse and pad share one focus
	// cursor. Without this the pad focus sat on the first button
	// while the mouse hovered another, and a first-frame click
	// could land on a button Slate had never focused. UMenuButton
	// binds its own hover handler (see MenuStyle.h).
	Button->OnHovered.AddDynamic(Button, &UMenuButton::HandleHoverFocus);
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *(FString(Name) + TEXT("Label")));
	if (Text != nullptr)
	{
		Text->SetText(FText::FromString(Label));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetJustification(ETextJustify::Center);
		// [2026-11-04] The label must never eat the button's click.
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->AddChild(Text);
	}
	return Button;
}

UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size,
					 FLinearColor Color, bool bBold)
{
	if (Tree == nullptr)
	{
		return nullptr;
	}
	UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	if (Label != nullptr)
	{
		Label->SetText(FText::FromString(Text));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		if (bBold)
		{
			Font.TypefaceFontName = TEXT("Bold");
		}
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Label;
}

void AddSpacer(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, float Height)
{
	if (Tree == nullptr || Box == nullptr)
	{
		return;
	}
	if (USpacer* Spacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass(), Name))
	{
		Spacer->SetSize(FVector2D(1.0f, Height));
		Box->AddChild(Spacer);
	}
}

void AddDivider(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name)
{
	if (Tree == nullptr || Box == nullptr)
	{
		return;
	}
	UBorder* Rule = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	if (Rule == nullptr)
	{
		return;
	}
	FLinearColor Color = Gold();
	Color.A = 0.55f;
	Rule->SetBrushColor(Color);
	Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Rule))
	{
		Slot->SetPadding(FMargin(48.0f, 10.0f, 48.0f, 10.0f));
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	// [2026-11-04] A border with no content has no desired height;
	// the slot padding above is what gives the rule its presence,
	// so pin a minimum via the rule's own padding.
	Rule->SetPadding(FMargin(0.0f, 1.5f));
}

void AddPadded(UVerticalBox* Box, UWidget* Child, float Top, float Bottom)
{
	if (Box == nullptr || Child == nullptr)
	{
		return;
	}
	if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child))
	{
		Slot->SetPadding(FMargin(0.0f, Top, 0.0f, Bottom));
	}
}
} // namespace UtmMenuStyle
