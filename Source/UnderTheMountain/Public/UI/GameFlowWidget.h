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
class USettingsWidget;
class UTextBlock;
class UWidget;

// [2026-11-04] Which flow panel is on screen (Collapsed panels are
// not on screen). Tracked so the Back chain can decide from state,
// never from guessing at Slate visibility.
enum class EGameFlowPanel : uint8
{
	None,
	Title,
	Pause,
	End
};

// [2026-11-04] What one Back press (Esc / DualSense Circle) must do.
// The pure ResolveBackAction below returns this; HandleBack acts on
// it. Split so the whole chain is headless-testable without Slate.
enum class EMenuBackAction : uint8
{
	Ignored,
	CloseSettingsToTitle,
	CloseSettingsToPause,
	ResumeRun
};

UCLASS()
class UNDERTHEMOUNTAIN_API UGameFlowWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-03 T-112] Focusable at construction: the game mode points
	// UIOnly focus at this widget, and an unfocusable target logs a
	// LogPlayerController Error on every menu open.
	UGameFlowWidget(const FObjectInitializer& ObjectInitializer);

	// [2026-11-03 T-072] The game mode that receives the clicks (weak:
	// a UI widget never keeps its game mode alive).
	void BindToGameMode(AUnderTheMountainGameMode* Mode);

	// [2026-11-04] The settings screen (created by the game mode one
	// viewport layer above this one). SETTINGS buttons on title and
	// pause open it; its OnClosed returns here, and the originating
	// panel (title or pause) is re-shown with pad focus restored.
	void BindSettingsWidget(USettingsWidget* Settings);

	// [2026-11-03 T-072] Screen switches; each hides the other two.
	// ShowEnd renders the run outcome into the end screen.
	void ShowTitle();
	void ShowPause();
	void ShowEnd(const FGameFlowResult& Result);
	void HideAll();

	// [2026-11-04] Back chain, step 2 (the game mode arbitrates
	// step 1 — see AUnderTheMountainGameMode::HandleMenuBack):
	//   settings open  -> close it, return to its origin panel
	//   pause visible  -> resume the run
	//   title / end    -> ignored (Start / end buttons own those)
	// Returns true when the press was consumed. NativeOnKeyDown
	// (Esc / Circle) and the pawn's IA_Pause both funnel here via
	// the game mode, so no path can double-toggle pause.
	bool HandleBack();
	bool IsSettingsOpen() const;
	EGameFlowPanel GetVisiblePanel() const
	{
		return VisiblePanel;
	}

	// [2026-11-04] The pure Back decision (see EMenuBackAction):
	// settings beats pause beats nothing. No widget state read —
	// callers pass the facts, tests pin every combination.
	static EMenuBackAction ResolveBackAction(bool bSettingsOpen, bool bSettingsFromTitle,
											 EGameFlowPanel VisiblePanel);

	// [2026-11-04 T-135] Pure end-screen text, tested verbatim:
	// the verdict word for one outcome (Won / PyrrhicWin / Lost
	// read as three different endings, never two), the resolution
	// line carrying the inside/outside kill split, and the full
	// end-screen body ShowEnd renders. Presentation only — the
	// snapshot it formats is UGameFlowMachine::MakeResult's.
	static FString FormatOutcomeVerdict(EGameFlowOutcome Outcome);
	// [2026-11-04 T-135] The resolution-score line for the end
	// screen (see FormatOutcomeVerdict for the pure-text contract).
	static FString FormatResolutionScore(int32 InsideKills, int32 OutsideKills);
	// [2026-11-04 T-135] The full end-screen body for a snapshot
	// (see FormatOutcomeVerdict for the pure-text contract).
	static FString FormatEndScreen(const FGameFlowResult& Result);

	// [2026-11-03 T-112] Public (the base keeps it protected) so the
	// UI.FlowWidget automation test can drive the code-built tree
	// headless: Initialize skips this without a player context.
	// Production still reaches it only via Initialize.
	virtual void NativeOnInitialized() override;

  protected:
	// [2026-11-04] Esc / Circle arrive here from whichever button
	// holds focus (key events bubble up from the focused control);
	// the game-mode focus widget is this widget at menus, so no
	// Back press can fall through to the pawn while a menu is up.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,
								   const FKeyEvent& InKeyEvent) override;

  private:
	UFUNCTION()
	void HandleStart();
	// [2026-11-04 T-138] Title sandbox button: starts a run with
	// the GameFlow sandbox flag armed (the normal Start button
	// starts a ruled run; the game mode owns applying the flag).
	UFUNCTION()
	void HandleStartSandbox();
	UFUNCTION()
	void HandleResume();
	UFUNCTION()
	void HandleRestart();
	UFUNCTION()
	void HandleQuitToTitle();
	UFUNCTION()
	void HandleQuit();
	// [2026-11-04] SETTINGS entries (title / pause) + the settings
	// close return (not a button handler: the settings widget's
	// plain multicast delegate calls it via AddUObject).
	UFUNCTION()
	void HandleOpenSettingsFromTitle();
	UFUNCTION()
	void HandleOpenSettingsFromPause();
	void HandleSettingsClosed();
	void OpenSettings(bool bFromTitle);

	// [2026-11-03 T-072] One canvas, three switchable panels (only one
	// visible at a time); the end screen's outcome line is rewritten
	// per run.
	// [2026-11-03 T-112] Each entry is the attached Border (not the
	// content Box inside it) — ShowOnly toggles these.
	UPROPERTY(Transient)
	TObjectPtr<UWidget> TitlePanel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> PausePanel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> EndPanel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EndOutcomeText = nullptr;
	// [2026-11-04] First button of each panel: Show* points pad
	// keyboard focus here so D-pad navigation starts somewhere sane.
	UPROPERTY(Transient)
	TObjectPtr<UButton> TitleFirstButton = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UButton> PauseFirstButton = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UButton> EndFirstButton = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<USettingsWidget> BoundSettings = nullptr;
	// [2026-11-04] Which panel opened settings (its panel + focus are
	// restored on close). True = title, false = pause.
	bool bSettingsFromTitle = true;
	// [2026-11-04] Panel currently Visible; all others Collapsed.
	EGameFlowPanel VisiblePanel = EGameFlowPanel::None;

	void ShowOnly(UWidget* Panel);

	TWeakObjectPtr<AUnderTheMountainGameMode> BoundMode;
};
