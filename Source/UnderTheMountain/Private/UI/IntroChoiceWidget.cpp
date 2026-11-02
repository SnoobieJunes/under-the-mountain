// Copyright Under the Mountain contributors.
// [2026-11-01 T-018] UIntroChoiceWidget implementation.
// [2026-11-01 T-019] Modernized at owner direction: a centred
// translucent panel with a gold display title, a muted subtitle
// and two full-width buttons - Chase in ember accent, Let go in
// slate - instead of bare text on an opaque box.

#include "UI/IntroChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Dragon/DragonCharacter.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace
{
// [2026-11-01 T-019] Button styling in code: flat colour brushes
// for normal/hovered/pressed, so the choice reads as designed UI
// rather than engine-default grey buttons.
UButton* MakeChoiceButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
						  FLinearColor Normal, FLinearColor Hovered, int32 FontSize)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	if (!Button)
	{
		return nullptr;
	}
	FButtonStyle Style = Button->GetStyle();
	Style.Normal = FSlateColorBrush(Normal);
	Style.Hovered = FSlateColorBrush(Hovered);
	Style.Pressed = FSlateColorBrush(Hovered * 0.8f);
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

UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size,
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
		Label->SetJustification(ETextJustify::Center);
	}
	return Label;
}
} // namespace

void UIntroChoiceWidget::BindToPawn(ADragonCharacter* Pawn)
{
	BoundPawn = Pawn;
}

void UIntroChoiceWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}
	UCanvasPanel* Canvas =
		WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("IntroCanvas"));
	if (!Canvas)
	{
		return;
	}
	WidgetTree->RootWidget = Canvas;

	UBorder* Panel =
		WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntroPanel"));
	UVerticalBox* Box =
		WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroBox"));
	UTextBlock* Title = MakeText(WidgetTree, TEXT("IntroTitle"),
								 TEXT("A GNOME IS STEALING YOUR GOLD"), 26,
								 FLinearColor(1.0f, 0.78f, 0.3f));
	UTextBlock* Subtitle = MakeText(
		WidgetTree, TEXT("IntroSubtitle"),
		TEXT("It snatched a prize from your hoard and is making for the exit.\nWhat do you do?"),
		14, FLinearColor(0.75f, 0.78f, 0.85f));
	ChaseButton = MakeChoiceButton(WidgetTree, TEXT("IntroChase"), TEXT("CHASE IT DOWN"),
								   FLinearColor(0.75f, 0.25f, 0.08f), FLinearColor(0.95f, 0.35f, 0.1f),
								   17);
	LetGoButton = MakeChoiceButton(WidgetTree, TEXT("IntroLetGo"), TEXT("Let it go"),
								   FLinearColor(0.16f, 0.18f, 0.24f), FLinearColor(0.25f, 0.28f, 0.36f),
								   15);
	if (!Panel || !Box || !Title || !Subtitle || !ChaseButton || !LetGoButton)
	{
		return;
	}
	Panel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
	Panel->SetPadding(FMargin(36.0f, 28.0f));

	Box->AddChildToVerticalBox(Title);
	if (UVerticalBoxSlot* SubSlot = Box->AddChildToVerticalBox(Subtitle))
	{
		SubSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 22.0f));
	}
	if (UVerticalBoxSlot* ChaseSlot = Box->AddChildToVerticalBox(ChaseButton))
	{
		ChaseSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	Box->AddChildToVerticalBox(LetGoButton);
	Panel->SetContent(Box);

	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Panel))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D(0.0f, 0.0f));
		Slot->SetSize(FVector2D(560.0f, 320.0f));
	}

	ChaseButton->OnClicked.AddDynamic(this, &UIntroChoiceWidget::HandleChase);
	LetGoButton->OnClicked.AddDynamic(this, &UIntroChoiceWidget::HandleLetGo);
}

void UIntroChoiceWidget::HandleLetGo()
{
	if (ADragonCharacter* Pawn = BoundPawn.Get())
	{
		Pawn->HandleIntroChoice(0);
	}
}

void UIntroChoiceWidget::HandleChase()
{
	if (ADragonCharacter* Pawn = BoundPawn.Get())
	{
		Pawn->HandleIntroChoice(1);
	}
}
