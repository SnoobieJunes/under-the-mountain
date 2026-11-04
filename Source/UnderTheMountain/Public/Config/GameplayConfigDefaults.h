// Copyright Under the Mountain contributors.
// Purpose: [2026-11-04 T-136] code-side DataAsset seam: config structs whose
// defaults mirror the shipped C++ literals, so a future DA_SessionLoop /
// DA_DragonStats / DA_ScentConfig / DT_Waves asset can be authored in the
// editor and loaded over these defaults without changing a tuned number.
// What remains editor-side (honest): creating Content/Gameplay/DA_*.uasset,
// pointing each owning component/subsystem at the asset, and deleting the
// duplicate literals once that load is proven in PIE. Until then the
// literals stay — this file does NOT delete working literals.
// Defaults below were grepped 2026-11-04 from: DragonStatsComponent.h,
// ScentComponent.h/ScentMath (brackets), WaveSubsystem.h, GAMEPLAY S1/S4.
#pragma once
#include "CoreMinimal.h"

struct UNDERTHEMOUNTAIN_API FSessionLoopConfig // DA_SessionLoop (GAMEPLAY S1)
{
	float SessionMinMinutes = 18.0f;
	float SessionMaxMinutes = 25.0f;
	float FreePlayGapMinSeconds = 90.0f;
	float FreePlayGapMaxSeconds = 150.0f;
	int32 WavesPerSession = 6;
	float IncursionIntervalMinSeconds = 60.0f;
	float IncursionIntervalMaxSeconds = 120.0f;
	int32 IncursionsBetweenWavesMin = 1;
	int32 IncursionsBetweenWavesMax = 2;
	int32 MaxConcurrentIncursions = 2;
	float WaveWarningSeconds = 20.0f;
	float WaveCapSeconds = 240.0f;
	static FSessionLoopConfig Default();
};
struct UNDERTHEMOUNTAIN_API FDragonStatsConfig // DA_DragonStats (GAMEPLAY S2)
{
	float MaxStamina = 100.0f;
	float MaxHP = 100.0f;
	float MaxMagic = 100.0f;
	float FlyingDrainPerSecond = 0.0f;
	float GlideDrainPerSecond = 0.0f;
	float HoverDrainPerSecond = 3.0f;
	float HoverSpeedThreshold = 150.0f;
	float ExhaustionHPDrainPerSecond = 2.0f;
	float GroundRegenPerSecond = 15.0f;
	float FlapStaminaCost = 3.0f;
	float ImpulseReserveCost = 10.0f;
	float LandingSpeedThreshold = 800.0f;
	float LandingSpeedMaxDamage = 2200.0f;
	float MinLandingDamage = 5.0f;
	float MaxLandingDamage = 25.0f;
	static FDragonStatsConfig Default();
};
struct UNDERTHEMOUNTAIN_API FScentConfig // DA_ScentConfig (GAMEPLAY S3)
{
	int32 MaxActiveRibbons = 64;
	float TrueSightMagicDrainPerSecond = 3.0f;
	float TrueSightRibbonOpacity = 0.3f;
	float NormalRibbonOpacity = 1.0f;
	static FScentConfig Default();
};
struct UNDERTHEMOUNTAIN_API FWavesConfig // DT_Waves (GAMEPLAY S4)
{
	float IntermissionSeconds = 25.0f;
	float SpawnGroupIntervalSeconds = 15.0f;
	int32 FinalWave = 6;
	static FWavesConfig Default();
};
