// Copyright Under the Mountain contributors.
// Purpose: T-072 game-flow screens — title, pause, and won/lost end
// screens. Code-built like the intro choice (no WBP asset to drift).
// It owns no game state: buttons forward to the game mode, which owns
// the UGameFlowMachine and applies its verdicts (pause, reload, quit).
// Created 2026-11-03 (T-072).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/GameFlowMachine.h"
#include "GameFlowWidget.generated.h"

class AUnderTheMountainGameMode;
class UButton;
class UTextBlock;
class UWidget;

UCLASS()
class UNDERTHEMOUNTAIN_API UGameFlowWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-03 T-072] The game mode that receives the clicks (weak:
	// a UI widget never keeps its game mode alive).
	void BindToGameMode(AUnderTheMountainGameMode* Mode);

	// [2026-11-03 T-072] Screen switches; each hides the other two.
	// ShowEnd renders the run outcome into the end screen.
	void ShowTitle();
	void ShowPause();
	void ShowEnd(const FGameFlowResult& Result);
	void HideAll();

  protected:
	virtual void NativeOnInitialized() override;

  private:
	UFUNCTION()
	void HandleStart();
	UFUNCTION()
	void HandleResume();
	UFUNCTION()
	void HandleRestart();
	UFUNCTION()
	void HandleQuitToTitle();
	UFUNCTION()
	void HandleQuit();

	// [2026-11-03 T-072] One canvas, three switchable panels (only one
	// visible at a time); the end screen's outcome line is rewritten
	// per run.
	UPROPERTY(Transient)
	TObjectPtr<UWidget> TitlePanel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> PausePanel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> EndPanel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EndOutcomeText = nullptr;

	void ShowOnly(UWidget* Panel);

	TWeakObjectPtr<AUnderTheMountainGameMode> BoundMode;
};
