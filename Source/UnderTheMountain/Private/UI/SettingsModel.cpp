// Copyright Under the Mountain contributors.
// [2026-11-04 T-088] Settings model implementation: defaults,
// sanitize, live apply, and config persistence. Only Apply* touches
// live engine state; everything else is pure value logic.

#include "UI/SettingsModel.h"

#include "AudioDevice.h"
#include "AudioDeviceHandle.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
// [2026-11-04 T-088] Live sensitivity cvars (see header): Apply writes
// them, UtmLookSensitivity/UtmFlightSensitivity read them, and the
// defaults match DefaultUtmSettings() so an unapplied run feels 1:1.
TAutoConsoleVariable<float> CVarLookSensitivity(TEXT("utm.settings.lookSensitivity"), 1.0f,
												TEXT("Settings look sensitivity multiplier."));
TAutoConsoleVariable<float> CVarFlightSensitivity(TEXT("utm.settings.flightSensitivity"), 1.0f,
												  TEXT("Settings flight sensitivity multiplier."));

// [2026-11-04 T-088] The config section for the fields
// UGameUserSettings has no slot for, written into GameUserSettings.ini
// alongside the display/quality values it owns.
const TCHAR* UtmSettingsSection = TEXT("/Script/UnderTheMountain.UtmSettings");

FString ResolveIniFilename(const FString& IniFilename)
{
	return IniFilename.IsEmpty() ? GGameUserSettingsIni : IniFilename;
}

void SetCvarInt(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByConsole);
	}
}

void SetCvarFloat(const TCHAR* Name, float Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByConsole);
	}
}
} // namespace

FUtmSettings DefaultUtmSettings()
{
	// [2026-11-04 T-088] The struct's member initializers ARE the
	// defaults (see header); this function exists so callers never
	// depend on that implementation detail.
	return FUtmSettings();
}

FUtmSettings SanitizeUtmSettings(const FUtmSettings& In)
{
	FUtmSettings Out = In;
	Out.Resolution.X = FMath::Max(Out.Resolution.X, 640);
	Out.Resolution.Y = FMath::Max(Out.Resolution.Y, 360);
	Out.QualityPreset = GraphicsQualityPresetFromInt(static_cast<int32>(Out.QualityPreset));
	Out.MasterVolume = FMath::Clamp(Out.MasterVolume, 0.0f, 1.0f);
	Out.LookSensitivity = FMath::Clamp(Out.LookSensitivity, 0.1f, 5.0f);
	Out.FlightSensitivity = FMath::Clamp(Out.FlightSensitivity, 0.1f, 5.0f);
	const int32 CameraIndex = FMath::Clamp(static_cast<int32>(Out.CameraMode), 0,
										   static_cast<int32>(EDragonCameraMode::Eyes));
	Out.CameraMode = static_cast<EDragonCameraMode>(CameraIndex);
	return Out;
}

bool UtmSettingsEqual(const FUtmSettings& A, const FUtmSettings& B)
{
	return A.Resolution == B.Resolution && A.bFullscreen == B.bFullscreen &&
		   A.QualityPreset == B.QualityPreset && A.MasterVolume == B.MasterVolume &&
		   A.LookSensitivity == B.LookSensitivity && A.FlightSensitivity == B.FlightSensitivity &&
		   A.bInvertMouseFlightPitch == B.bInvertMouseFlightPitch &&
		   A.bInvertPadFlightPitch == B.bInvertPadFlightPitch &&
		   A.bInvertPadCameraPitch == B.bInvertPadCameraPitch && A.CameraMode == B.CameraMode &&
		   A.bInfiniteStamina == B.bInfiniteStamina;
}

TArray<FIntPoint> UtmSettingsResolutionChoices()
{
	return {FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080),
			FIntPoint(2560, 1440)};
}

float UtmLookSensitivity()
{
	return CVarLookSensitivity.GetValueOnGameThread();
}

float UtmFlightSensitivity()
{
	return CVarFlightSensitivity.GetValueOnGameThread();
}

void ApplyUtmSettingsCvars(const FUtmSettings& Settings)
{
	const FUtmSettings S = SanitizeUtmSettings(Settings);
	// [2026-11-04 T-088] The three invert lanes map 1:1 onto the pawn's
	// shipped cvars (T-065), and Infinite Stamina onto T-124's shipped
	// cvar — the Settings screen is a new writer of existing seams,
	// not a parallel state the pawn would have to reconcile.
	SetCvarInt(TEXT("utm.camera.invertMousePitch"), S.bInvertMouseFlightPitch ? 1 : 0);
	SetCvarInt(TEXT("utm.camera.invertPadFlight"), S.bInvertPadFlightPitch ? 1 : 0);
	SetCvarInt(TEXT("utm.camera.invertPadCamera"), S.bInvertPadCameraPitch ? 1 : 0);
	SetCvarInt(TEXT("utm.testing.infinitestamina"), S.bInfiniteStamina ? 1 : 0);
	SetCvarFloat(TEXT("utm.settings.lookSensitivity"), S.LookSensitivity);
	SetCvarFloat(TEXT("utm.settings.flightSensitivity"), S.FlightSensitivity);
}

void ApplyUtmSettings(const FUtmSettings& Settings)
{
	const FUtmSettings S = SanitizeUtmSettings(Settings);

	// [2026-11-04 T-128] Quality first (it sets the sg.* groups and
	// saves GameUserSettings), then this snapshot's resolution and
	// fullscreen on the same object.
	ApplyGraphicsQualityPreset(S.QualityPreset);
	if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
	{
		UserSettings->SetScreenResolution(S.Resolution);
		UserSettings->SetFullscreenMode(S.bFullscreen ? EWindowMode::Fullscreen
													  : EWindowMode::Windowed);
		UserSettings->ApplySettings(false);
		UserSettings->SaveSettings();
	}

	// [2026-11-04 T-088] Master volume: the main audio device's
	// transient primary volume. No audio device (headless, or before
	// T-071 audio lands) is a silent no-op — the value still persists.
	FAudioDeviceHandle AudioHandle = FAudioDevice::GetMainAudioDevice();
	if (AudioHandle.IsValid())
	{
		if (FAudioDevice* AudioDevice = AudioHandle.GetAudioDevice())
		{
			AudioDevice->SetTransientPrimaryVolume(S.MasterVolume);
		}
	}

	// [2026-11-04 T-088] Camera choice through the pawn's existing
	// utm.camera.mode seam (T-111: the pawn adopts + persists the
	// forced mode on its next rig tick). Applying settings does not
	// own the pawn's own save slot.
	SetCvarInt(TEXT("utm.camera.mode"), static_cast<int32>(S.CameraMode));

	ApplyUtmSettingsCvars(S);
}

void SaveUtmSettings(const FUtmSettings& Settings, const FString& IniFilename)
{
	const FUtmSettings S = SanitizeUtmSettings(Settings);
	// [2026-11-04 T-088] Display/quality persistence is
	// UGameUserSettings' job (it owns those fields in the ini).
	// An explicit test ini skips this entirely:
	// tests must never write the player's real config or settings.
	if (IniFilename.IsEmpty())
	{
		ApplyGraphicsQualityPreset(S.QualityPreset);
		if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
		{
			UserSettings->SetScreenResolution(S.Resolution);
			UserSettings->SetFullscreenMode(S.bFullscreen ? EWindowMode::Fullscreen
														  : EWindowMode::Windowed);
			UserSettings->SaveSettings();
		}
	}
	// [2026-11-04 T-088] The rest ride in the same ini under our own
	// section, written to the same ini so an explicit test filename
	// never touches the player's real config.
	const FString Ini = ResolveIniFilename(IniFilename);
	// [2026-11-04 T-088] Direct FConfigFile I/O, deliberately not
	// GConfig: GConfig's per-filename branch cache made a same-process
	// save->load round-trip read stale defaults (proven by
	// Settings.Model). FConfigFile reads/writes the named file exactly.
	FConfigFile Config;
	Config.Read(Ini);
	Config.SetFloat(UtmSettingsSection, TEXT("MasterVolume"), S.MasterVolume);
	Config.SetFloat(UtmSettingsSection, TEXT("LookSensitivity"), S.LookSensitivity);
	Config.SetFloat(UtmSettingsSection, TEXT("FlightSensitivity"), S.FlightSensitivity);
	Config.SetBool(UtmSettingsSection, TEXT("InvertMouseFlightPitch"), S.bInvertMouseFlightPitch);
	Config.SetBool(UtmSettingsSection, TEXT("InvertPadFlightPitch"), S.bInvertPadFlightPitch);
	Config.SetBool(UtmSettingsSection, TEXT("InvertPadCameraPitch"), S.bInvertPadCameraPitch);
	Config.SetString(UtmSettingsSection, TEXT("CameraMode"),
					 *FString::FromInt(static_cast<int32>(S.CameraMode)));
	Config.SetBool(UtmSettingsSection, TEXT("InfiniteStamina"), S.bInfiniteStamina);
	// [2026-11-04 T-128] The preset name is recorded too, so a human
	// reading the ini sees the choice without decoding sg.* levels.
	Config.SetString(UtmSettingsSection, TEXT("QualityPreset"),
					 *GraphicsQualityPresetName(S.QualityPreset));
	Config.Write(Ini);
}

FUtmSettings LoadUtmSettings(const FString& IniFilename)
{
	FUtmSettings Out = DefaultUtmSettings();
	const FString Ini = ResolveIniFilename(IniFilename);
	// [2026-11-04 T-088] Same direct-file contract as Save (see there):
	// read the named ini exactly, never a cached GConfig branch.
	FConfigFile Config;
	Config.Read(Ini);
	// [2026-11-04 T-088] Display/quality come back from
	// UGameUserSettings' fields when reading the default ini; with an
	// explicit (test) ini there is no live GameUserSettings backed by
	// that file, so only the section keys below are read.
	if (IniFilename.IsEmpty())
	{
		if (const UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
		{
			Out.Resolution = UserSettings->GetScreenResolution();
			Out.bFullscreen = UserSettings->GetFullscreenMode() != EWindowMode::Windowed;
			Out.QualityPreset =
				GraphicsQualityPresetFromInt(UserSettings->GetOverallScalabilityLevel());
		}
	}
	Config.GetFloat(UtmSettingsSection, TEXT("MasterVolume"), Out.MasterVolume);
	Config.GetFloat(UtmSettingsSection, TEXT("LookSensitivity"), Out.LookSensitivity);
	Config.GetFloat(UtmSettingsSection, TEXT("FlightSensitivity"), Out.FlightSensitivity);
	Config.GetBool(UtmSettingsSection, TEXT("InvertMouseFlightPitch"),
				   Out.bInvertMouseFlightPitch);
	Config.GetBool(UtmSettingsSection, TEXT("InvertPadFlightPitch"), Out.bInvertPadFlightPitch);
	Config.GetBool(UtmSettingsSection, TEXT("InvertPadCameraPitch"), Out.bInvertPadCameraPitch);
	int32 CameraIndex = static_cast<int32>(Out.CameraMode);
	FString CameraText;
	if (Config.GetString(UtmSettingsSection, TEXT("CameraMode"), CameraText))
	{
		CameraIndex = FCString::Atoi(*CameraText);
		Out.CameraMode = static_cast<EDragonCameraMode>(CameraIndex);
	}
	Config.GetBool(UtmSettingsSection, TEXT("InfiniteStamina"), Out.bInfiniteStamina);
	FString PresetName;
	if (Config.GetString(UtmSettingsSection, TEXT("QualityPreset"), PresetName))
	{
		Out.QualityPreset = GraphicsQualityPresetFromName(PresetName);
	}
	return SanitizeUtmSettings(Out);
}
