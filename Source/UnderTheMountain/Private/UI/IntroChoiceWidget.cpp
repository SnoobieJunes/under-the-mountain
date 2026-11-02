// Copyright Under the Mountain contributors.
// [2026-11-01 T-018] UIntroChoiceWidget implementation: a title
// and two buttons, built in code (the tint picker's pattern).

#include "UI/IntroChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Dragon/DragonCharacter.h"

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
	UVerticalBox* Root =
		WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroRoot"));
	if (!Root)
	{
		return;
	}
	WidgetTree->RootWidget = Root;

	UTextBlock* Title =
		WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntroTitle"));
	LetGoButton =
		WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("IntroLetGo"));
	ChaseButton =
		WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("IntroChase"));
	if (!Title || !LetGoButton || !ChaseButton)
	{
		return;
	}
	Title->SetText(FText::FromString(TEXT("A gnome is stealing your gold!")));
	UTextBlock* LetGoLabel =
		WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntroLetGoLabel"));
	LetGoLabel->SetText(FText::FromString(TEXT("Let it go (it will talk...)")));
	LetGoButton->AddChild(LetGoLabel);
	UTextBlock* ChaseLabel =
		WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntroChaseLabel"));
	ChaseLabel->SetText(FText::FromString(TEXT("CHASE IT")));
	ChaseButton->AddChild(ChaseLabel);
	LetGoButton->OnClicked.AddDynamic(this, &UIntroChoiceWidget::HandleLetGo);
	ChaseButton->OnClicked.AddDynamic(this, &UIntroChoiceWidget::HandleChase);

	Root->AddChildToVerticalBox(Title);
	Root->AddChildToVerticalBox(LetGoButton);
	Root->AddChildToVerticalBox(ChaseButton);
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
