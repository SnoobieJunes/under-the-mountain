// Copyright Under the Mountain contributors.
// Purpose: T-088 settings screen — code-built like the game-flow
// screens (no WBP asset to drift). It owns no settings state of its
// own beyond the working snapshot: every control edits FUtmSettings,
// then ApplyUtmSettings + SaveUtmSettings (SettingsModel) do the
// live/persist work, so the screen is reachable-model only and the
// model stays headless-testable without it. Reachable from title +
// pause is the game mode's wiring (T-072 screens); this widget only
// needs Show/Hide and its snapshot.
// Created 2026-11-04 (T-088).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SettingsModel.h"
#include "SettingsWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS()
class UNDERTHEMOUNTAIN_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-04 T-088] Focusable at construction, same reason as
	// UGameFlowWidget (T-112): a UIOnly focus target that is not
	// focusable logs an error on every menu open.
	USettingsWidget(const FObjectInitializer& ObjectInitializer);

	// [2026-11-04 T-088] Load the persisted snapshot into the working
	// copy and refresh every row. Called on Show; also callable
	// headless so tests can drive the snapshot without a viewport.
	void ReloadFromSaved();

	void ShowSettings();
	void HideSettings();

	// [2026-11-04 T-088] The working snapshot (tests read this; the
	// screen's buttons are the only writers besides ReloadFromSaved).
	const FUtmSettings& GetSettings() const
	{
		return Current;
	}
	void SetSettingsForTesting(const FUtmSettings& InSettings);

	// [2026-11-04 T-088] Pure row labels, tested verbatim (the HUD
	// precedent): what a row reads is what the model holds.
	static FString FormatOnOff(bool bOn);
	static FString FormatResolution(FIntPoint Resolution, bool bFullscreen);
	static FString FormatPercent(float Value01);
	static FString FormatSensitivity(float Multiplier);
	static FString FormatCamera(EDragonCameraMode Mode);

	// [2026-11-04 T-088] Public like UGameFlowWidget (T-112) so the
	// automation test can build the code-built tree headless.
	virtual void NativeOnInitialized() override;

  private:
	UFUNCTION()
	void HandleCycleResolution();
	UFUNCTION()
	void HandleToggleFullscreen();
	UFUNCTION()
	void HandleCycleQuality();
	UFUNCTION()
	void HandleVolumeDown();
	UFUNCTION()
	void HandleVolumeUp();
	UFUNCTION()
	void HandleLookSensitivityDown();
	UFUNCTION()
	void HandleLookSensitivityUp();
	UFUNCTION()
	void HandleFlightSensitivityDown();
	UFUNCTION()
	void HandleFlightSensitivityUp();
	UFUNCTION()
	void HandleToggleInvertMouse();
	UFUNCTION()
	void HandleToggleInvertPadFlight();
	UFUNCTION()
	void HandleToggleInvertPadCamera();
	UFUNCTION()
	void HandleCycleCamera();
	UFUNCTION()
	void HandleToggleInfiniteStamina();
	UFUNCTION()
	void HandleResetDefaults();
	UFUNCTION()
	void HandleClose();

	// [2026-11-04 T-088] Every mutation funnels through here: sanitize,
	// apply live, persist, then re-render the rows — a control can
	// never show a value that was not applied.
	void CommitAndRefresh();
	void RefreshLabels();

	FUtmSettings Current = DefaultUtmSettings();

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Panel = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResolutionText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QualityText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VolumeText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LookSensitivityText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FlightSensitivityText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InvertMouseText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InvertPadFlightText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InvertPadCameraText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CameraText = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InfiniteStaminaText = nullptr;
};
