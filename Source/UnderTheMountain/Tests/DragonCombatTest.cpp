// Copyright Under the Mountain contributors.
// Purpose: T-015 dragon combat contract tests — fire-cone geometry,
// held-breath kill/stamina economics, roar radius + cooldown, and
// the eat channel (thief corpse and goat food values) — all
// exercised headless with no world spawned, because
// UDragonCombatComponent takes its brains/locations and clock as
// injected inputs by design.
// Created 2026-10-28 (T-015).

#include "Misc/AutomationTest.h"
#include "Dragon/DragonCombatComponent.h"
#include "Dragon/DragonStatsComponent.h"
#include "Livestock/GoatActor.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
// [2026-10-28 T-015] An exit far from every scenario position so a
// Fleeing/Scared brain never accidentally "arrives" mid-scenario.
const FVector CombatExitLocation(-9000.0f, 0.0f, 0.0f);

// [2026-10-28 T-015] Configs are built in code (NewObject + fields),
// burglar-shaped like ThiefBrainTest's MakeConfig(1, true, false):
// 20 HP, carries one treasure, drops all loot on a roar, no traps.
UThiefDataAsset* MakeBurglarConfig()
{
	UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
	Config->MaxHP = 20.0f;
	Config->GrabTimeSeconds = 1.5f;
	Config->CarriedCap = 1;
	Config->bDropsAllLootOnRoar = true;
	Config->bCanPlaceTraps = false;
	Config->MaxTraps = 3;
	return Config;
}

UThiefBrain* MakeCombatBrain(UThiefDataAsset* Config, UTreasureRegistry* Registry, const TCHAR* Name)
{
	UThiefBrain* Brain = NewObject<UThiefBrain>();
	Brain->Init(Config, Registry, FName(Name));
	Brain->SetExitLocation(CombatExitLocation);
	return Brain;
}

// [2026-10-28 T-015] A registry holding exactly one treasure: enough
// for the fire scenario, where looting is not under test but the
// brain still needs a real registry to initialise against.
UTreasureRegistry* MakeSingleTreasureRegistry(const FVector& Location)
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();

	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1200;
	Crown.Location = Location;
	Registry->Register(Crown);

	return Registry;
}

// [2026-10-28 T-015] Drives one full grab of whatever the brain thinks
// is richest, standing exactly on it: one tick to arrive (Sneak->Grab)
// and one tick of the config's grab time (Grab->next state) — the
// proven ThiefBrainTest pattern.
void GrabBestTreasureForCombat(UThiefBrain* Brain, const FVector& TreasureLocation)
{
	Brain->Tick(0.1f, TreasureLocation);
	Brain->Tick(1.5f, TreasureLocation);
}
} // namespace

// [2026-10-28 T-015] Fire contract: the pure cone test (front/back/
// range/angle edges), then a held breath killing a 20 HP burglar in
// the cone while a brain behind the dragon stays untouched, at the
// documented stamina cost — and the minimum-stamina start gate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCombatFireTest, "UnderTheMountain.Dragon.CombatFire",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCombatFireTest::RunTest(const FString& Parameters)
{
	// --- Cone math: dragon at the origin facing +X, 1400 cm range,
	// 30 degree half-angle (the component's authored defaults). ---
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward(1.0f, 0.0f, 0.0f);
	const float Range = 1400.0f;
	const float HalfAngleDeg = 30.0f;

	TestTrue(TEXT("straight ahead inside range is in the cone"),
			 UDragonCombatComponent::IsInFireCone(Origin, Forward, FVector(500.0f, 0.0f, 0.0f),
												  Range, HalfAngleDeg));
	TestFalse(TEXT("directly behind the dragon is outside the cone"),
			  UDragonCombatComponent::IsInFireCone(Origin, Forward, FVector(-500.0f, 0.0f, 0.0f),
												   Range, HalfAngleDeg));
	TestFalse(TEXT("straight ahead beyond range is outside the cone"),
			  UDragonCombatComponent::IsInFireCone(Origin, Forward, FVector(2000.0f, 0.0f, 0.0f),
												   Range, HalfAngleDeg));

	// Off-axis targets at 1000 cm: 25 degrees is inside the 30 degree
	// half-angle, 35 degrees is outside it.
	const float Rad25 = FMath::DegreesToRadians(25.0f);
	const FVector At25Degrees(1000.0f * FMath::Cos(Rad25), 1000.0f * FMath::Sin(Rad25), 0.0f);
	TestTrue(TEXT("25 degrees off-axis is inside the cone"),
			 UDragonCombatComponent::IsInFireCone(Origin, Forward, At25Degrees, Range,
												  HalfAngleDeg));

	const float Rad35 = FMath::DegreesToRadians(35.0f);
	const FVector At35Degrees(1000.0f * FMath::Cos(Rad35), 1000.0f * FMath::Sin(Rad35), 0.0f);
	TestFalse(TEXT("35 degrees off-axis is outside the cone"),
			  UDragonCombatComponent::IsInFireCone(Origin, Forward, At35Degrees, Range,
												   HalfAngleDeg));

	// --- Held breath kills: 18 DPS for 1.2 s (21.6) against 20 HP,
	// costing 5 stamina/s (6 over the tick: 50 -> ~44). ---
	UTreasureRegistry* Registry = MakeSingleTreasureRegistry(FVector(500.0f, 0.0f, 0.0f));
	UThiefBrain* Burglar = MakeCombatBrain(MakeBurglarConfig(), Registry, TEXT("FireBurglar"));
	UThiefBrain* Behind = MakeCombatBrain(MakeBurglarConfig(), Registry, TEXT("BehindBurglar"));
	TestEqual(TEXT("burglar starts at config HP"), Burglar->HP(), 20.0f);

	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->Stamina = 50.0f;

	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	TestTrue(TEXT("fire starts with enough stamina"), Combat->TryStartFire(Stats));
	TestTrue(TEXT("combat reports firing after a successful start"), Combat->IsFiring());

	const TArray<UThiefBrain*> Brains = {Burglar, Behind};
	const TArray<FVector> BrainLocations = {FVector(500.0f, 0.0f, 0.0f),
											FVector(-500.0f, 0.0f, 0.0f)};
	const int32 Damaged = Combat->TickFire(1.2f, Stats, Origin, Forward, Brains, BrainLocations);
	TestEqual(TEXT("exactly one brain was damaged by the breath"), Damaged, 1);
	TestTrue(TEXT("1.2 s of held breath kills the burglar in the cone"),
			 Burglar->State() == EThiefState::Dead);
	TestTrue(TEXT("1.2 s of breath drains about 6 stamina (50 -> ~44)"),
			 FMath::IsNearlyEqual(Stats->Stamina, 44.0f, 0.1f));

	TestFalse(TEXT("brain behind the dragon is not dead"), Behind->State() == EThiefState::Dead);
	TestEqual(TEXT("brain behind the dragon kept full HP"), Behind->HP(), 20.0f);

	Combat->StopFire();
	TestFalse(TEXT("combat reports not firing after StopFire"), Combat->IsFiring());

	// [2026-10-28 T-015] Start gate: 5 stamina is under the 10 minimum
	// a breath needs to ignite, so no one-frame puffs from an
	// almost-empty pool.
	UDragonStatsComponent* LowStats = NewObject<UDragonStatsComponent>();
	LowStats->Stamina = 5.0f;
	UDragonCombatComponent* LowCombat = NewObject<UDragonCombatComponent>();
	TestFalse(TEXT("fire refuses to start below the minimum stamina"),
			  LowCombat->TryStartFire(LowStats));
	TestFalse(TEXT("a refused start is not firing"), LowCombat->IsFiring());

	return true;
}

// [2026-10-28 T-015] Roar + eat contract: a roar scares (and empties
// the hands of) only the in-radius thief, costs 10 stamina, and is
// cooldown-blocked 1 s later; the 2 s eat channel then pays out the
// thief-corpse (+60/+15) and goat (+50/+10) food values exactly once,
// at completion and not before.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCombatRoarEatTest, "UnderTheMountain.Dragon.CombatRoarEat",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCombatRoarEatTest::RunTest(const FString& Parameters)
{
	// --- Roar: two burglars each carrying one treasure, one inside
	// the 1200 cm radius and one far outside it. The treasures sit at
	// the thieves' roar positions so the grab ticks leave each brain
	// standing where the roar scenario needs it. ---
	const FVector Origin = FVector::ZeroVector;
	const FVector NearLocation(500.0f, 0.0f, 0.0f);
	const FVector FarLocation(5000.0f, 0.0f, 0.0f);

	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	{
		FTreasureEntry Crown;
		Crown.Id = FName(TEXT("Crown"));
		Crown.GoldValue = 1200;
		Crown.Location = NearLocation;
		Registry->Register(Crown);

		FTreasureEntry Goblet;
		Goblet.Id = FName(TEXT("Goblet"));
		Goblet.GoldValue = 250;
		Goblet.Location = FarLocation;
		Registry->Register(Goblet);
	}

	// The near brain grabs first, so it takes the richest piece (the
	// crown, at its own location); the far brain then takes the goblet.
	UThiefBrain* NearBrain = MakeCombatBrain(MakeBurglarConfig(), Registry, TEXT("NearBurglar"));
	UThiefBrain* FarBrain = MakeCombatBrain(MakeBurglarConfig(), Registry, TEXT("FarBurglar"));
	GrabBestTreasureForCombat(NearBrain, NearLocation);
	GrabBestTreasureForCombat(FarBrain, FarLocation);
	TestEqual(TEXT("near burglar carries one before the roar"), NearBrain->CarriedItems().Num(),
			  1);
	TestEqual(TEXT("far burglar carries one before the roar"), FarBrain->CarriedItems().Num(), 1);
	TestTrue(TEXT("far burglar is fleeing before the roar"),
			 FarBrain->State() == EThiefState::Flee);

	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->Stamina = 100.0f;

	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	const TArray<UThiefBrain*> Brains = {NearBrain, FarBrain};
	const TArray<FVector> BrainLocations = {NearLocation, FarLocation};

	TestTrue(TEXT("roar succeeds at t=100"),
			 Combat->TryRoar(100.0f, Stats, Origin, Brains, BrainLocations));
	TestTrue(TEXT("roar scares the in-radius burglar"),
			 NearBrain->State() == EThiefState::Scared);
	TestEqual(TEXT("burglar drops all loot on the roar"), NearBrain->CarriedItems().Num(), 0);
	TestTrue(TEXT("far burglar state is unchanged by the roar"),
			 FarBrain->State() == EThiefState::Flee);
	TestEqual(TEXT("far burglar keeps its loot"), FarBrain->CarriedItems().Num(), 1);
	TestEqual(TEXT("roar costs 10 stamina (100 -> 90)"), Stats->Stamina, 90.0f);

	TestFalse(TEXT("second roar 1 s later is blocked by the cooldown"),
			  Combat->TryRoar(101.0f, Stats, Origin, Brains, BrainLocations));
	TestEqual(TEXT("a blocked roar spends no stamina"), Stats->Stamina, 90.0f);

	// --- Eat a thief corpse: +60 stamina / +15 HP after the full
	// 2 s channel, nothing at the halfway tick. ---
	{
		UTreasureRegistry* MealRegistry =
			MakeSingleTreasureRegistry(FVector(100.0f, 0.0f, 0.0f));
		UThiefBrain* DeadBrain = MakeCombatBrain(MakeBurglarConfig(), MealRegistry, TEXT("MealThief"));
		TestTrue(TEXT("lethal damage kills the meal thief"),
				 DeadBrain->ApplyDamage(999.0f, FVector(100.0f, 0.0f, 0.0f)));
		TestTrue(TEXT("meal thief is dead"), DeadBrain->State() == EThiefState::Dead);

		UDragonCombatComponent* EatCombat = NewObject<UDragonCombatComponent>();
		TestTrue(TEXT("eat channel starts on a dead thief"), EatCombat->StartEatThief(DeadBrain));

		UDragonStatsComponent* EatStats = NewObject<UDragonStatsComponent>();
		EatStats->Stamina = 0.0f;
		EatStats->HP = 50.0f;

		TestTrue(TEXT("half-way tick is still chewing"),
				 EatCombat->TickEat(1.0f, EatStats) == EEatResult::InProgress);
		TestTrue(TEXT("tick past the 2 s channel eats the thief"),
				 EatCombat->TickEat(1.1f, EatStats) == EEatResult::AteThief);
		TestEqual(TEXT("thief corpse restores 60 stamina"), EatStats->Stamina, 60.0f);
		TestEqual(TEXT("thief corpse restores 15 HP (50 -> 65)"), EatStats->HP, 65.0f);
	}

	// --- Eat a goat: the goat's own food values (+50 stamina /
	// +10 HP), and eating marks the goat consumed. ---
	{
		AGoatActor* Goat = NewObject<AGoatActor>();
		TestTrue(TEXT("lethal damage reports the goat down"), Goat->ApplyGoatDamage(999.0f));
		TestTrue(TEXT("goat is dead"), Goat->IsDead());

		UDragonCombatComponent* EatCombat = NewObject<UDragonCombatComponent>();
		TestTrue(TEXT("eat channel starts on the goat"), EatCombat->StartEatGoat(Goat));

		UDragonStatsComponent* EatStats = NewObject<UDragonStatsComponent>();
		EatStats->Stamina = 0.0f;
		EatStats->HP = 50.0f;

		TestTrue(TEXT("tick past the 2 s channel eats the goat"),
				 EatCombat->TickEat(2.1f, EatStats) == EEatResult::AteGoat);
		TestEqual(TEXT("goat restores 50 stamina"), EatStats->Stamina, 50.0f);
		TestEqual(TEXT("goat restores 10 HP (50 -> 60)"), EatStats->HP, 60.0f);
		TestTrue(TEXT("goat is eaten after the channel"), Goat->IsEaten());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
