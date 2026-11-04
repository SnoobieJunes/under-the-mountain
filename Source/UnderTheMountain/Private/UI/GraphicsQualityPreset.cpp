// Copyright Under the Mountain contributors.
// [2026-11-04 T-128] Graphics preset table + live apply. The table is
// pure data; ApplyGraphicsQualityPreset is the only function here
// that touches live engine state.

#include "UI/GraphicsQualityPreset.h"

#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"

EGraphicsQualityPreset DefaultGraphicsQualityPreset()
{
	// [2026-11-04 T-128] Owner pick: highest authored quality ships.
	return EGraphicsQualityPreset::Ultra;
}

FGraphicsQualitySettings GraphicsQualitySettingsForPreset(EGraphicsQualityPreset Preset)
{
	FGraphicsQualitySettings Settings;
	// [2026-11-04 T-128] One level per preset step for every group, so
	// the ladder is uniform and monotonic. Resolution scale is the
	// extra lever at the bottom end, where group levels bottom out.
	switch (Preset)
	{
	case EGraphicsQualityPreset::Low:
		Settings.ResolutionQualityPercent = 50;
		Settings.ViewDistanceQuality = 0;
		Settings.AntiAliasingQuality = 0;
		Settings.ShadowQuality = 0;
		Settings.GlobalIlluminationQuality = 0;
		Settings.ReflectionQuality = 0;
		Settings.PostProcessQuality = 0;
		Settings.TextureQuality = 0;
		Settings.EffectsQuality = 0;
		Settings.FoliageQuality = 0;
		Settings.ShadingQuality = 0;
		break;
	case EGraphicsQualityPreset::Medium:
		Settings.ResolutionQualityPercent = 71;
		Settings.ViewDistanceQuality = 1;
		Settings.AntiAliasingQuality = 1;
		Settings.ShadowQuality = 1;
		Settings.GlobalIlluminationQuality = 1;
		Settings.ReflectionQuality = 1;
		Settings.PostProcessQuality = 1;
		Settings.TextureQuality = 1;
		Settings.EffectsQuality = 1;
		Settings.FoliageQuality = 1;
		Settings.ShadingQuality = 1;
		break;
	case EGraphicsQualityPreset::High:
		Settings.ResolutionQualityPercent = 85;
		Settings.ViewDistanceQuality = 2;
		Settings.AntiAliasingQuality = 2;
		Settings.ShadowQuality = 2;
		Settings.GlobalIlluminationQuality = 2;
		Settings.ReflectionQuality = 2;
		Settings.PostProcessQuality = 2;
		Settings.TextureQuality = 2;
		Settings.EffectsQuality = 2;
		Settings.FoliageQuality = 2;
		Settings.ShadingQuality = 2;
		break;
	case EGraphicsQualityPreset::Ultra:
	default:
		// [2026-11-04 T-128] Ultra = the authored sg.* ceiling (3/Epic
		// on every group, full resolution scale).
		Settings.ResolutionQualityPercent = 100;
		Settings.ViewDistanceQuality = 3;
		Settings.AntiAliasingQuality = 3;
		Settings.ShadowQuality = 3;
		Settings.GlobalIlluminationQuality = 3;
		Settings.ReflectionQuality = 3;
		Settings.PostProcessQuality = 3;
		Settings.TextureQuality = 3;
		Settings.EffectsQuality = 3;
		Settings.FoliageQuality = 3;
		Settings.ShadingQuality = 3;
		break;
	}
	return Settings;
}

FString GraphicsQualityPresetName(EGraphicsQualityPreset Preset)
{
	switch (Preset)
	{
	case EGraphicsQualityPreset::Low:
		return TEXT("Low");
	case EGraphicsQualityPreset::Medium:
		return TEXT("Medium");
	case EGraphicsQualityPreset::High:
		return TEXT("High");
	case EGraphicsQualityPreset::Ultra:
	default:
		return TEXT("Ultra");
	}
}

EGraphicsQualityPreset GraphicsQualityPresetFromName(const FString& Name)
{
	if (Name.Equals(TEXT("Low"), ESearchCase::IgnoreCase))
	{
		return EGraphicsQualityPreset::Low;
	}
	if (Name.Equals(TEXT("Medium"), ESearchCase::IgnoreCase))
	{
		return EGraphicsQualityPreset::Medium;
	}
	if (Name.Equals(TEXT("High"), ESearchCase::IgnoreCase))
	{
		return EGraphicsQualityPreset::High;
	}
	if (Name.Equals(TEXT("Ultra"), ESearchCase::IgnoreCase))
	{
		return EGraphicsQualityPreset::Ultra;
	}
	return DefaultGraphicsQualityPreset();
}

EGraphicsQualityPreset GraphicsQualityPresetFromInt(int32 Value)
{
	const int32 Clamped = FMath::Clamp(Value, static_cast<int32>(EGraphicsQualityPreset::Low),
									   static_cast<int32>(EGraphicsQualityPreset::Ultra));
	return static_cast<EGraphicsQualityPreset>(Clamped);
}

TArray<FString> GraphicsQualityConsoleCommands(EGraphicsQualityPreset Preset)
{
	const FGraphicsQualitySettings S = GraphicsQualitySettingsForPreset(Preset);
	// [2026-11-04 T-128] Every scalability group the task names is
	// present (GI / reflections / shadows included), plus the groups
	// GameUserSettings would otherwise leave at a stale level.
	return {
		FString::Printf(TEXT("sg.ResolutionQuality %d"), S.ResolutionQualityPercent),
		FString::Printf(TEXT("sg.ViewDistanceQuality %d"), S.ViewDistanceQuality),
		FString::Printf(TEXT("sg.AntiAliasingQuality %d"), S.AntiAliasingQuality),
		FString::Printf(TEXT("sg.ShadowQuality %d"), S.ShadowQuality),
		FString::Printf(TEXT("sg.GlobalIlluminationQuality %d"), S.GlobalIlluminationQuality),
		FString::Printf(TEXT("sg.ReflectionQuality %d"), S.ReflectionQuality),
		FString::Printf(TEXT("sg.PostProcessQuality %d"), S.PostProcessQuality),
		FString::Printf(TEXT("sg.TextureQuality %d"), S.TextureQuality),
		FString::Printf(TEXT("sg.EffectsQuality %d"), S.EffectsQuality),
		FString::Printf(TEXT("sg.FoliageQuality %d"), S.FoliageQuality),
		FString::Printf(TEXT("sg.ShadingQuality %d"), S.ShadingQuality),
	};
}

void ApplyGraphicsQualityPreset(EGraphicsQualityPreset Preset)
{
	// [2026-11-04 T-128] Console/cvar seam first: each "sg.<Group> <N>"
	// command is split into cvar + value and set through
	// IConsoleManager (the same seam the automation tests read back),
	// so the apply is immediate and log-visible.
	for (const FString& Command : GraphicsQualityConsoleCommands(Preset))
	{
		FString CVarName;
		FString CVarValue;
		if (Command.Split(TEXT(" "), &CVarName, &CVarValue))
		{
			if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName))
			{
				CVar->Set(*CVarValue, ECVF_SetByConsole);
			}
		}
	}

	// [2026-11-04 T-128] Persistence + renderer apply through
	// GameUserSettings (see header for why config, not a SaveGame):
	// setting every group then ApplySettings(false) makes UE push the
	// same values itself and SaveSettings writes GameUserSettings.ini.
	if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
	{
		const FGraphicsQualitySettings S = GraphicsQualitySettingsForPreset(Preset);
		UserSettings->SetViewDistanceQuality(S.ViewDistanceQuality);
		UserSettings->SetAntiAliasingQuality(S.AntiAliasingQuality);
		UserSettings->SetShadowQuality(S.ShadowQuality);
		UserSettings->SetGlobalIlluminationQuality(S.GlobalIlluminationQuality);
		UserSettings->SetReflectionQuality(S.ReflectionQuality);
		UserSettings->SetPostProcessingQuality(S.PostProcessQuality);
		UserSettings->SetTextureQuality(S.TextureQuality);
		UserSettings->SetVisualEffectQuality(S.EffectsQuality);
		UserSettings->SetFoliageQuality(S.FoliageQuality);
		UserSettings->SetShadingQuality(S.ShadingQuality);
		UserSettings->SetResolutionScaleValueEx(static_cast<float>(S.ResolutionQualityPercent));
		UserSettings->ApplySettings(false);
		UserSettings->SaveSettings();
	}
}
