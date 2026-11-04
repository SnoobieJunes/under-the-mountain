// Copyright Under the Mountain contributors.
// Purpose: T-088 settings model — the pure value every Settings-screen
// control reads and writes: display (resolution/fullscreen), quality
// preset (T-128), master volume, look + flight sensitivity, the three
// pitch-invert toggles, camera choice, and the Testing -> Infinite
// Stamina toggle. The widget is presentation only; apply / persist /
// defaults live here so they are headless-testable. Persistence is
// config (GameUserSettings.ini via UGameUserSettings + GConfig — see
// SettingsModel.cpp), not a SaveGame: display/quality already belong
// to UGameUserSettings, and one config file beats a second save slot
// as the single source of truth for the same values.
// Created 2026-11-04 (T-088).

#pragma once

#include "CoreMinimal.h"
#include "Dragon/DragonCharacter.h"
#include "UI/GraphicsQualityPreset.h"
#include "SettingsModel.generated.h"

// [2026-11-04 T-088] One settings snapshot. Plain value semantics:
// copy it, edit the copy, Apply + Save the copy. Defaults are
// DefaultUtmSettings() below, never a second literal elsewhere.
USTRUCT(BlueprintType)
struct FUtmSettings
{
	GENERATED_BODY()

	// [2026-11-04 T-088] Display. 1280x720 windowed is the measured
	// configuration on the reference Mac (T-020/T-115); fullscreen is
	// off by default (the no-flags fullscreen launch is still the
	// watched path, T-115/T-130).
	UPROPERTY(EditAnywhere, Category = "Settings")
	FIntPoint Resolution = FIntPoint(1280, 720);

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bFullscreen = false;

	// [2026-11-04 T-128] Quality preset; default is the highest
	// authored quality (see GraphicsQualityPreset.h).
	UPROPERTY(EditAnywhere, Category = "Settings")
	EGraphicsQualityPreset QualityPreset = EGraphicsQualityPreset::Ultra;

	// [2026-11-04 T-088] Master volume in [0, 1]; applied to the audio
	// device's transient primary volume (no-op until audio lands).
	UPROPERTY(EditAnywhere, Category = "Settings")
	float MasterVolume = 1.0f;

	// [2026-11-04 T-088] Sensitivity multipliers in [0.1, 5]; 1 = the
	// shipped feel. Look = mouse/right-stick aim, Flight = the pitch
	// authority while flying.
	UPROPERTY(EditAnywhere, Category = "Settings")
	float LookSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float FlightSensitivity = 1.0f;

	// [2026-11-04 T-088] Three separate invert toggles, all default
	// off = nose-up (stick/mouse up pitches the nose up,
	// docs/FLIGHT-SPIKE.md; the pawn's utm.camera.invert* cvars carry
	// the same defaults). True inverts that one lane only.
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInvertMouseFlightPitch = false;

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInvertPadFlightPitch = false;

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInvertPadCameraPitch = false;

	// [2026-11-04 T-088] Camera choice; same value the pawn cycles
	// with V / R3 and persists in its own slot (T-065/T-111). Near is
	// the shipped default.
	UPROPERTY(EditAnywhere, Category = "Settings")
	EDragonCameraMode CameraMode = EDragonCameraMode::Near;

	// [2026-11-04 T-088] Testing -> Infinite Stamina (God Mode),
	// default off. Drives the shipped utm.testing.infinitestamina
	// cvar (T-124): disables stamina costs/drains + exhaustion HP
	// burn only, never HP damage; the HUD badge shows while on.
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInfiniteStamina = false;
};

// [2026-11-04 T-088] The defaults every first run and every corrupt
// persisted value fall back to.
FUtmSettings DefaultUtmSettings();

// [2026-11-04 T-088] Clamp a settings value into the ranges the
// controls can represent: volume [0,1], sensitivities [0.1, 5],
// resolution at least 640x360, preset/camera clamped to their enums.
// Idempotent; Apply and Load both sanitize so no entry path can
// smuggle an out-of-range value to a consumer.
FUtmSettings SanitizeUtmSettings(const FUtmSettings& In);

// [2026-11-04 T-088] Field-wise equality (tests and the widget's
// dirty check); FUtmSettings stays a plain USTRUCT, so this is the
// one comparison implementation.
bool UtmSettingsEqual(const FUtmSettings& A, const FUtmSettings& B);

// [2026-11-04 T-088] The resolutions the Settings screen cycles, in
// cycle order. Pure data; the first entry is the default above.
TArray<FIntPoint> UtmSettingsResolutionChoices();

// [2026-11-04 T-088] Live sensitivity seam: Apply writes these cvars
// and input code reads them through the accessors, so sensitivity has
// exactly one live home until input consumes it directly.
float UtmLookSensitivity();
float UtmFlightSensitivity();

// [2026-11-04 T-088] The cvar half of Apply, split out so tests can
// exercise it without touching renderer/audio state: writes the
// three utm.camera.invert* cvars, utm.testing.infinitestamina, and
// the two utm.settings.*Sensitivity cvars from the snapshot.
void ApplyUtmSettingsCvars(const FUtmSettings& Settings);

// [2026-11-04 T-088] Apply everything live: display + quality through
// UGameUserSettings (resolution, fullscreen, T-128 preset), volume
// to the main audio device when one exists, camera through the
// utm.camera.mode capture seam the pawn already honours, and the
// remaining values through ApplyUtmSettingsCvars. Sanitizes first.
void ApplyUtmSettings(const FUtmSettings& Settings);

// [2026-11-04 T-088] Persist to config: display/quality through
// UGameUserSettings::SaveSettings, the remaining fields as
// [/Script/UnderTheMountain.UtmSettings] keys in the same
// GameUserSettings ini (IniFilename overridable for tests, which
// must never write the player's real config).
void SaveUtmSettings(const FUtmSettings& Settings, const FString& IniFilename = FString());

// [2026-11-04 T-088] Load from the same config; any missing or
// unreadable field keeps its DefaultUtmSettings() value, and the
// result is sanitized, so a hand-edited ini degrades to defaults
// field-by-field instead of failing wholesale.
FUtmSettings LoadUtmSettings(const FString& IniFilename = FString());
