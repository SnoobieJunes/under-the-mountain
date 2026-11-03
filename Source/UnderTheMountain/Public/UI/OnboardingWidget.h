// Copyright Under the Mountain contributors.
// Purpose: T-073 onboarding UI — one contextual prompt line plus the
// help overlay. Code-built (no WBP asset to drift). Never blocks input:
// the prompt line is hit-test invisible, and the help overlay is a
// non-modal panel (the game keeps running behind it).
// Created 2026-11-03 (T-073).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/OnboardingDefs.h"
#include "OnboardingWidget.generated.h"

UCLASS()
class UNDERTHEMOUNTAIN_API UOnboardingWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-03 T-073] Shows one prompt row (keyboard + pad lines) or
	// clears it. Hit-test invisible: prompts teach, never intercept.
	void ShowPrompt(const FOnboardingPromptDef& Prompt);
	void ClearPrompt();

	// [2026-11-03 T-073] Help overlay: every shipped binding, keyboard
	// and pad columns, rendered from GetOnboardingBindings (the single
	// source). Non-modal: the game keeps running behind it.
	void ToggleHelp();
	bool IsHelpVisible() const;

  protected:
	virtual void NativeOnInitialized() override;

  private:
	void BuildHelpPanel();

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> PromptLine = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<class UWidget> HelpPanel = nullptr;
	bool bHelpVisible = false;
};
