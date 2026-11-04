// Copyright Under the Mountain contributors.
// Purpose: the HUD (T-019) - modern game readout: HP and stamina
// as filled bars (bottom-left), notoriety as star pips, hoard
// gold and wave state top-right, all on translucent dark panels.
// Code-built (no WBP asset to drift). It owns no state: the pawn
// feeds it the subsystems' numbers every tick. Magic joins when
// T-080's pool exists; there is no placeholder for a stat the
// game does not have yet.
// Created 2026-11-01 (T-019); modernized same day at owner
// direction (bars/pips/panels replaced the debug text lines).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragonHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class UNDERTHEMOUNTAIN_API UDragonHUDWidget : public UUserWidget
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-019] Show one frame of game state: bar
	// fractions, star pips, gold and wave. WaveStateText is the
	// caller's word so this widget stays presentation-only.
	// [2026-11-04 T-138] SandboxBadgeLabel rides Refresh as its own
	// line (not folded into the testing badge): a sandbox run and
	// god mode are different facts and can be on together.
	void Refresh(float HP, float MaxHP, float Stamina, float MaxStamina, int32 Stars, int32 Points,
				 int64 Hoard, int64 StartingHoard, int32 Wave, int32 XP, const FString& WaveStateText,
				 const FString& FlightStateLabel, const FString& TestingBadgeLabel,
				 const FString& SandboxBadgeLabel);

	// [2026-11-01 T-019] Pure formatting, tested verbatim:
	// star pips (filled/empty of 5) with the point total, the
	// gold line, and bar fractions clamped to [0,1] (a stat
	// over its max reads full, never overflowing the bar).
	static FString FormatStars(int32 Stars, int32 Points);
	static FString FormatHoard(int64 Hoard, int64 StartingHoard, int32 Wave, int32 XP,
							   const FString& WaveStateText);
	static float BarFraction(float Value, float Max);

	// [2026-11-03 T-064] Pure flight-state naming, tested verbatim: the
	// player-terms line the HUD shows while airborne (dive names its
	// recovery, glide names how to enter a dive). Dive wins ties — it is
	// the urgent state. All false is level cruise (glide); the pawn
	// passes empty while grounded so the line collapses.
	static FString FormatFlightState(bool bDiving, bool bBraking, bool bHovering);

	// [2026-11-04 T-124] Pure testing-badge text, tested verbatim: the
	// god-mode indicator the HUD shows while utm.testing.infinitestamina
	// is on (off collapses the line, like the grounded flight state).
	static FString FormatTestingBadge(bool bInfiniteStamina);

	// [2026-11-04 T-138] Pure sandbox-badge text, tested verbatim:
	// "SANDBOX" while a sandbox run is live, empty (line collapsed
	// by Refresh, same contract as the testing badge) in a ruled
	// run, so free flight is never mistaken for a scored session.
	static FString FormatSandboxBadge(bool bSandbox);

  protected:
	virtual void NativeOnInitialized() override;

  private:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HPBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HPText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StaminaText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StarsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HoardText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WaveText;

	// [2026-11-03 T-064] Flight-state line (Dive/Hover/Brake/Glide in
	// player terms); hidden while grounded (empty text collapses it).
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FlightStateText;

	// [2026-11-04 T-124] Testing badge (god mode); same empty-collapses
	// contract as the flight-state line above.
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TestingBadgeText;

	// [2026-11-04 T-138] Sandbox badge; same empty-collapses
	// contract, in a cool tint that reads as mode, not warning.
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SandboxBadgeText;
};
