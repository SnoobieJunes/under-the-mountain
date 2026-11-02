// Copyright Under the Mountain contributors.
// Purpose: the intro choice (T-018) - "The gnome has your gold"
// with Let go / Chase buttons. Code-built like the tint picker
// (no WBP asset to drift). It owns no game state: clicks forward
// to the pawn, which forwards to UIntroSubsystem::Choose.
// Created 2026-11-01 (T-018).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IntroChoiceWidget.generated.h"

class ADragonCharacter;
class UButton;

UCLASS()
class UNDERTHEMOUNTAIN_API UIntroChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-018] The pawn that receives the choice (weak:
	// a UI widget never keeps its pawn alive).
	void BindToPawn(ADragonCharacter* Pawn);

  protected:
	virtual void NativeOnInitialized() override;

  private:
	UFUNCTION()
	void HandleLetGo();

	UFUNCTION()
	void HandleChase();

	UPROPERTY(Transient)
	TObjectPtr<UButton> LetGoButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ChaseButton;

	TWeakObjectPtr<ADragonCharacter> BoundPawn;
};
