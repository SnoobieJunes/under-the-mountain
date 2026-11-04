// Copyright Under the Mountain contributors.
// Purpose: T-128 graphics preset table + T-088 settings model tests —
// the pure preset->settings table, defaults/sanitize, the cvar seam
// (incl. the shipped T-124 stamina cvar), and the config round-trip.
// Headless: no renderer state is applied; live GameUserSettings state
// is only read back through the explicit test ini for the model half.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "UI/GraphicsQualityPreset.h"
#include "UI/SettingsModel.h"
#include "UI/SettingsWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-04 T-128] The preset table contract: Ultra is the default,
// every group is monotonic down the ladder, and the console command
// set names every sg.* group the presets promise to drive.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraphicsQualityPresetTest,
								 "UnderTheMountain.Settings.GraphicsQuality",
								 EAutomationTestFlags::EditorContext |
									 EAutomationTestFlags::EngineFilter)

bool FGraphicsQualityPresetTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("default preset is Ultra (highest authored)"),
			 DefaultGraphicsQualityPreset() == EGraphicsQualityPreset::Ultra);
	TestEqual(TEXT("default settings are Ultra settings"),
			  GraphicsQualitySettingsForPreset(DefaultGraphicsQualityPreset()).ShadowQuality, 3);

	const EGraphicsQualityPreset Ladder[] = {EGraphicsQualityPreset::Low,
											 EGraphicsQualityPreset::Medium,
											 EGraphicsQualityPreset::High,
											 EGraphicsQualityPreset::Ultra};
	int32 PreviousShadow = -1;
	int32 PreviousGI = -1;
	int32 PreviousReflection = -1;
	int32 PreviousResolution = -1;
	for (const EGraphicsQualityPreset Preset : Ladder)
	{
		const FGraphicsQualitySettings S = GraphicsQualitySettingsForPreset(Preset);
		TestTrue(TEXT("shadow never drops down the ladder"), S.ShadowQuality >= PreviousShadow);
		TestTrue(TEXT("GI never drops down the ladder"),
				 S.GlobalIlluminationQuality >= PreviousGI);
		TestTrue(TEXT("reflections never drop down the ladder"),
				 S.ReflectionQuality >= PreviousReflection);
		TestTrue(TEXT("resolution scale never drops down the ladder"),
				 S.ResolutionQualityPercent >= PreviousResolution);
		PreviousShadow = S.ShadowQuality;
		PreviousGI = S.GlobalIlluminationQuality;
		PreviousReflection = S.ReflectionQuality;
		PreviousResolution = S.ResolutionQualityPercent;
	}
	TestEqual(TEXT("Ultra shadow is the authored ceiling"),
			  GraphicsQualitySettingsForPreset(EGraphicsQualityPreset::Ultra).ShadowQuality, 3);
	TestEqual(TEXT("Low shadow is the floor"),
			  GraphicsQualitySettingsForPreset(EGraphicsQualityPreset::Low).ShadowQuality, 0);
	TestEqual(TEXT("Ultra resolution scale is full"),
			  GraphicsQualitySettingsForPreset(EGraphicsQualityPreset::Ultra)
				  .ResolutionQualityPercent,
			  100);

	for (const EGraphicsQualityPreset Preset : Ladder)
	{
		TestTrue(TEXT("preset name round-trips"),
				 GraphicsQualityPresetFromName(GraphicsQualityPresetName(Preset)) == Preset);
		const TArray<FString> Commands = GraphicsQualityConsoleCommands(Preset);
		auto HasGroup = [&Commands](const TCHAR* Group)
		{
			for (const FString& Command : Commands)
			{
				if (Command.StartsWith(Group))
				{
					return true;
				}
			}
			return false;
		};
		TestTrue(TEXT("commands drive resolution"), HasGroup(TEXT("sg.ResolutionQuality")));
		TestTrue(TEXT("commands drive GI"), HasGroup(TEXT("sg.GlobalIlluminationQuality")));
		TestTrue(TEXT("commands drive reflections"), HasGroup(TEXT("sg.ReflectionQuality")));
		TestTrue(TEXT("commands drive shadows"), HasGroup(TEXT("sg.ShadowQuality")));
	}
	TestTrue(TEXT("unknown preset name falls back to Ultra, not Low"),
			 GraphicsQualityPresetFromName(TEXT("Potato")) == EGraphicsQualityPreset::Ultra);
	TestTrue(TEXT("out-of-range preset int clamps to Ultra"),
			 GraphicsQualityPresetFromInt(99) == EGraphicsQualityPreset::Ultra);
	TestTrue(TEXT("negative preset int clamps to Low"),
			 GraphicsQualityPresetFromInt(-2) == EGraphicsQualityPreset::Low);
	return true;
}

// [2026-11-04 T-088] The settings model contract: sensible defaults
// (Ultra, nose-up inverts, Near camera, stamina off), sanitize clamps,
// the cvar seam writes the shipped cvar names, and config persists.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsModelTest, "UnderTheMountain.Settings.Model",
								 EAutomationTestFlags::EditorContext |
									 EAutomationTestFlags::EngineFilter)

bool FSettingsModelTest::RunTest(const FString& Parameters)
{
	const FUtmSettings Defaults = DefaultUtmSettings();
	TestTrue(TEXT("default quality is Ultra"), Defaults.QualityPreset == EGraphicsQualityPreset::Ultra);
	TestTrue(TEXT("default resolution is the measured 1280x720"),
			 Defaults.Resolution == FIntPoint(1280, 720));
	TestFalse(TEXT("default is windowed"), Defaults.bFullscreen);
	TestEqual(TEXT("default master volume is full"), Defaults.MasterVolume, 1.0f);
	TestEqual(TEXT("default look sensitivity is 1"), Defaults.LookSensitivity, 1.0f);
	TestEqual(TEXT("default flight sensitivity is 1"), Defaults.FlightSensitivity, 1.0f);
	TestFalse(TEXT("mouse invert defaults nose-up (off)"), Defaults.bInvertMouseFlightPitch);
	TestFalse(TEXT("pad flight invert defaults nose-up (off)"), Defaults.bInvertPadFlightPitch);
	TestFalse(TEXT("pad camera invert defaults nose-up (off)"), Defaults.bInvertPadCameraPitch);
	TestTrue(TEXT("default camera is Near"), Defaults.CameraMode == EDragonCameraMode::Near);
	TestFalse(TEXT("infinite stamina defaults off"), Defaults.bInfiniteStamina);
	TestTrue(TEXT("defaults survive sanitize unchanged"),
			 UtmSettingsEqual(SanitizeUtmSettings(Defaults), Defaults));
	TestTrue(TEXT("resolution choices start at the default"),
			 UtmSettingsResolutionChoices()[0] == Defaults.Resolution);

	FUtmSettings Wild;
	Wild.MasterVolume = 7.0f;
	Wild.LookSensitivity = 0.0f;
	Wild.FlightSensitivity = 99.0f;
	Wild.Resolution = FIntPoint(10, 10);
	const FUtmSettings Clamped = SanitizeUtmSettings(Wild);
	TestEqual(TEXT("volume clamps to 1"), Clamped.MasterVolume, 1.0f);
	TestEqual(TEXT("look sensitivity clamps to floor"), Clamped.LookSensitivity, 0.1f);
	TestEqual(TEXT("flight sensitivity clamps to ceiling"), Clamped.FlightSensitivity, 5.0f);
	TestTrue(TEXT("resolution clamps to minimum"), Clamped.Resolution == FIntPoint(640, 360));

	// [2026-11-04 T-088] Cvar seam: the model writes the exact shipped
	// cvar names (T-065 inverts, T-124 stamina); restore afterwards so
	// the suite never leaks a god-mode or inverted run into later tests.
	auto GetCvarInt = [](const TCHAR* Name)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		return CVar ? CVar->GetInt() : -999;
	};
	const int32 OldStamina = GetCvarInt(TEXT("utm.testing.infinitestamina"));
	const int32 OldInvertMouse = GetCvarInt(TEXT("utm.camera.invertMousePitch"));
	FUtmSettings Applied = Defaults;
	Applied.bInfiniteStamina = true;
	Applied.bInvertMouseFlightPitch = true;
	Applied.LookSensitivity = 2.0f;
	ApplyUtmSettingsCvars(Applied);
	TestEqual(TEXT("stamina toggle drives utm.testing.infinitestamina"),
			  GetCvarInt(TEXT("utm.testing.infinitestamina")), 1);
	TestEqual(TEXT("mouse invert drives utm.camera.invertMousePitch"),
			  GetCvarInt(TEXT("utm.camera.invertMousePitch")), 1);
	TestEqual(TEXT("look sensitivity accessor reads the cvar"), UtmLookSensitivity(), 2.0f);
	ApplyUtmSettingsCvars(Defaults);
	TestEqual(TEXT("stamina restored off"), GetCvarInt(TEXT("utm.testing.infinitestamina")),
			  OldStamina);
	TestEqual(TEXT("mouse invert restored"), GetCvarInt(TEXT("utm.camera.invertMousePitch")),
			  OldInvertMouse);

	// [2026-11-04 T-088] Config round-trip through an explicit test
	// ini (never the player's GameUserSettings.ini).
	const FString TestIni =
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UtmSettingsModelTest.ini"));
	FUtmSettings Saved = Defaults;
	Saved.MasterVolume = 0.25f;
	Saved.LookSensitivity = 1.5f;
	Saved.bInvertPadFlightPitch = true;
	Saved.CameraMode = EDragonCameraMode::Eyes;
	Saved.QualityPreset = EGraphicsQualityPreset::High;
	Saved.bInfiniteStamina = true;
	SaveUtmSettings(Saved, TestIni);
	const FUtmSettings Loaded = LoadUtmSettings(TestIni);
	TestEqual(TEXT("volume round-trips"), Loaded.MasterVolume, 0.25f);
	TestEqual(TEXT("look sensitivity round-trips"), Loaded.LookSensitivity, 1.5f);
	TestTrue(TEXT("pad flight invert round-trips"), Loaded.bInvertPadFlightPitch);
	TestTrue(TEXT("camera round-trips"), Loaded.CameraMode == EDragonCameraMode::Eyes);
	TestTrue(TEXT("quality preset round-trips"), Loaded.QualityPreset == EGraphicsQualityPreset::High);
	TestTrue(TEXT("infinite stamina round-trips"), Loaded.bInfiniteStamina);
	IFileManager::Get().Delete(*TestIni, false, true, true);

	// [2026-11-04 T-088] Widget row labels, verbatim (presentation only).
	TestEqual(TEXT("on label"), USettingsWidget::FormatOnOff(true), FString(TEXT("On")));
	TestEqual(TEXT("off label"), USettingsWidget::FormatOnOff(false), FString(TEXT("Off")));
	TestEqual(TEXT("resolution label"),
			  USettingsWidget::FormatResolution(FIntPoint(1280, 720), false),
			  FString(TEXT("1280 x 720 (Windowed)")));
	TestEqual(TEXT("volume label"), USettingsWidget::FormatPercent(0.5f), FString(TEXT("50%")));
	TestEqual(TEXT("sensitivity label"), USettingsWidget::FormatSensitivity(1.5f),
			  FString(TEXT("1.5x")));
	TestEqual(TEXT("camera label"), USettingsWidget::FormatCamera(EDragonCameraMode::Near),
			  FString(TEXT("Near chase above the wings")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
