// [2026-11-04 T-136] Pin config defaults bit-identical to shipped literals.
#include "Misc/AutomationTest.h"
#include "Config/GameplayConfigDefaults.h"
#include "Dragon/DragonStatsComponent.h"
#include "Scent/ScentComponent.h"
#include "Waves/WaveSubsystem.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayConfigDefaultsTest, "UnderTheMountain.Config.DefaultsPin",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameplayConfigDefaultsTest::RunTest(const FString& P)
{
	const FDragonStatsConfig D = FDragonStatsConfig::Default();
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	TestEqual(TEXT("MaxStamina"), D.MaxStamina, Stats->MaxStamina);
	TestEqual(TEXT("MaxHP"), D.MaxHP, Stats->MaxHP);
	TestEqual(TEXT("HoverDrain"), D.HoverDrainPerSecond, Stats->HoverDrainPerSecond);
	TestEqual(TEXT("GlideDrain"), D.GlideDrainPerSecond, Stats->GlideDrainPerSecond);
	TestEqual(TEXT("FlapCost"), D.FlapStaminaCost, Stats->FlapStaminaCost);
	TestEqual(TEXT("ImpulseCost"), D.ImpulseReserveCost, Stats->ImpulseReserveCost);
	TestEqual(TEXT("LandingThreshold"), D.LandingSpeedThreshold, Stats->LandingSpeedThreshold);
	TestEqual(TEXT("LandingMaxDamageSpeed"), D.LandingSpeedMaxDamage, Stats->LandingSpeedMaxDamage);
	TestEqual(TEXT("MinLandingDamage"), D.MinLandingDamage, Stats->MinLandingDamage);
	TestEqual(TEXT("MaxLandingDamage"), D.MaxLandingDamage, Stats->MaxLandingDamage);
	TestEqual(TEXT("GroundRegen"), D.GroundRegenPerSecond, Stats->GroundRegenPerSecond);
	TestEqual(TEXT("ExhaustionHP"), D.ExhaustionHPDrainPerSecond, Stats->ExhaustionHPDrainPerSecond);
	const FScentConfig S = FScentConfig::Default();
	UScentComponent* Scent = NewObject<UScentComponent>();
	TestEqual(TEXT("MaxRibbons"), S.MaxActiveRibbons, Scent->MaxActiveRibbons);
	TestEqual(TEXT("TrueSightDrain"), S.TrueSightMagicDrainPerSecond, Scent->TrueSightMagicDrainPerSecond);
	const FWavesConfig W = FWavesConfig::Default();
	// [2026-11-04 T-136] WaveSubsystem is a WorldSubsystem (needs a world),
	// so pin its grepped literals directly: 25 / 15 / 6 in WaveSubsystem.h.
	TestEqual(TEXT("Intermission"), W.IntermissionSeconds, 25.0f);
	TestEqual(TEXT("SpawnInterval"), W.SpawnGroupIntervalSeconds, 15.0f);
	TestEqual(TEXT("FinalWave"), W.FinalWave, 6);
	const FSessionLoopConfig L = FSessionLoopConfig::Default();
	TestEqual(TEXT("session waves = final wave"), L.WavesPerSession, W.FinalWave);
	TestTrue(TEXT("gap range sane"), L.FreePlayGapMinSeconds < L.FreePlayGapMaxSeconds);
	TestEqual(TEXT("wave warning 20s"), L.WaveWarningSeconds, 20.f);
	TestEqual(TEXT("wave cap 4min"), L.WaveCapSeconds, 240.f);
	return true;
}
#endif
