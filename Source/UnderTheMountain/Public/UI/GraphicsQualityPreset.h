// Copyright Under the Mountain contributors.
// Purpose: T-128 graphics quality presets — one pure preset -> settings
// table (Low/Medium/High/Ultra) that the Settings screen, the live
// console/cvar apply, and persistence all read, so the three can never
// disagree about what "High" means. Created 2026-11-04 (T-128).

#pragma once

#include "CoreMinimal.h"
#include "GraphicsQualityPreset.generated.h"

// [2026-11-04 T-128] Preset ladder. Ultra is the highest authored
// quality and the shipped default (owner pick 2026-11-03: ship maximum
// fidelity for the demo; lower presets are the relief valve, not the
// default). Enumerator order is quality order, so clamping an
// out-of-range int into [Low, Ultra] is the only validation needed.
UENUM(BlueprintType)
enum class EGraphicsQualityPreset : uint8
{
	Low UMETA(DisplayName = "Low"),
	Medium UMETA(DisplayName = "Medium"),
	High UMETA(DisplayName = "High"),
	Ultra UMETA(DisplayName = "Ultra"),
};

// [2026-11-04 T-128] One row of the preset table: every sg.* scalability
// group the preset sets, in the engine's 0 (Low) .. 3 (Epic) group
// scale, plus the resolution scale in percent. Plain struct (no
// UPROPERTY ceremony): it is a value returned by the pure table below,
// never authored in the editor or saved directly.
struct FGraphicsQualitySettings
{
	int32 ResolutionQualityPercent = 100;
	int32 ViewDistanceQuality = 3;
	int32 AntiAliasingQuality = 3;
	int32 ShadowQuality = 3;
	int32 GlobalIlluminationQuality = 3;
	int32 ReflectionQuality = 3;
	int32 PostProcessQuality = 3;
	int32 TextureQuality = 3;
	int32 EffectsQuality = 3;
	int32 FoliageQuality = 3;
	int32 ShadingQuality = 3;
};

// [2026-11-04 T-128] The shipped default: highest authored quality.
EGraphicsQualityPreset DefaultGraphicsQualityPreset();

// [2026-11-04 T-128] Pure preset -> settings table. Monotonic by
// construction: no group ever rises as the preset steps down, so a
// weaker machine trading down never pays more for any single group.
FGraphicsQualitySettings GraphicsQualitySettingsForPreset(EGraphicsQualityPreset Preset);

// [2026-11-04 T-128] Display name ("Ultra") and its inverse. Parsing
// an unknown name falls back to the default preset, never to Low:
// a corrupt persisted value must not silently tank the demo fidelity.
FString GraphicsQualityPresetName(EGraphicsQualityPreset Preset);
EGraphicsQualityPreset GraphicsQualityPresetFromName(const FString& Name);

// [2026-11-04 T-128] Clamp an arbitrary int (e.g. a persisted overall
// scalability level) into a valid preset.
EGraphicsQualityPreset GraphicsQualityPresetFromInt(int32 Value);

// [2026-11-04 T-128] The live-apply seam as data: the exact sg.*
// console commands a preset executes ("sg.ShadowQuality 3", ...),
// including sg.ResolutionQuality. Pure, so tests pin the command set
// without touching the renderer's live cvars.
TArray<FString> GraphicsQualityConsoleCommands(EGraphicsQualityPreset Preset);

// [2026-11-04 T-128] Apply a preset live and persist it. Persistence
// goes through UGameUserSettings (GGameUserSettings / scalability),
// not a bespoke SaveGame: resolution, fullscreen and the sg.* groups
// are exactly what GameUserSettings.ini already owns, ApplySettings
// pushes them onto the live sg.* cvars itself, and a second save slot
// would be a second source of truth for the same values. The sg.*
// commands above are also executed through IConsoleManager first, so
// the change is visible in the console/log and applies even where a
// group has a command-line override GameUserSettings would respect.
// Safe headless (no world needed); GameUserSettings may be null very
// early in startup, in which case only the cvar half runs.
void ApplyGraphicsQualityPreset(EGraphicsQualityPreset Preset);
