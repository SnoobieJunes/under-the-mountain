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
#include "World/BurnablePropActor.h"

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
	const TArray<AGoatActor*> NoGoats;
	const TArray<FVector> NoGoatLocations;
	const TArray<ABurnablePropActor*> NoProps;
	const TArray<FVector> NoPropLocations;
	const int32 Damaged = Combat->TickFire(1.2f, Stats, Origin, Forward, Brains, BrainLocations,
										   NoGoats, NoGoatLocations, NoProps, NoPropLocations);
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

// [2026-11-01 T-078] Fire vs goats: the same held breath that kills
// thieves burns goats in the cone — 18 DPS takes a 30 HP goat down
// across two 1 s ticks, a goat behind the dragon is untouched, and
// a corpse is never counted (or damaged) twice. Killed goats stay
// eatable: dead, not eaten.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCombatFireGoatTest, "UnderTheMountain.Dragon.CombatFireGoat",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCombatFireGoatTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward(1.0f, 0.0f, 0.0f);

	AGoatActor* InCone = NewObject<AGoatActor>();
	AGoatActor* Behind = NewObject<AGoatActor>();
	TestEqual(TEXT("goat starts at 30 HP"), InCone->HP, 30.0f);

	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->Stamina = 100.0f;
	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	TestTrue(TEXT("fire starts with enough stamina"), Combat->TryStartFire(Stats));

	const TArray<UThiefBrain*> NoBrains;
	const TArray<FVector> NoBrainLocations;
	const TArray<ABurnablePropActor*> NoProps;
	const TArray<FVector> NoPropLocations;
	const TArray<AGoatActor*> Goats = {InCone, Behind};
	const TArray<FVector> GoatLocations = {FVector(500.0f, 0.0f, 0.0f),
										   FVector(-500.0f, 0.0f, 0.0f)};

	TestEqual(TEXT("first second burns exactly the goat in the cone"),
			  Combat->TickFire(1.0f, Stats, Origin, Forward, NoBrains, NoBrainLocations, Goats,
							   GoatLocations, NoProps, NoPropLocations),
			  1);
	TestEqual(TEXT("18 DPS for 1 s leaves the goat at 12 HP"), InCone->HP, 12.0f);
	TestFalse(TEXT("goat still alive after one second"), InCone->IsDead());
	TestEqual(TEXT("goat behind the dragon kept full HP"), Behind->HP, 30.0f);

	TestEqual(TEXT("second second still burns the goat in the cone"),
			  Combat->TickFire(1.0f, Stats, Origin, Forward, NoBrains, NoBrainLocations, Goats,
							   GoatLocations, NoProps, NoPropLocations),
			  1);
	TestTrue(TEXT("fire-killed goat is dead"), InCone->IsDead());
	TestTrue(TEXT("fire-killed goat is eatable, not eaten"), !InCone->IsEaten());

	TestEqual(TEXT("a third second counts no corpse twice"),
			  Combat->TickFire(1.0f, Stats, Origin, Forward, NoBrains, NoBrainLocations, Goats,
							   GoatLocations, NoProps, NoPropLocations),
			  0);
	TestEqual(TEXT("corpse HP stays at zero"), InCone->HP, 0.0f);

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

// [2026-11-01 T-055] Burnable contract: exposure below the 1.2 s
// threshold does nothing, at it the prop ignites, burning/burnt
// never move backwards, and the pure rule agrees at the edges.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCombatBurnTest, "UnderTheMountain.Dragon.CombatBurn",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCombatBurnTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("half exposure does not ignite"),
			 ABurnablePropActor::StateAfterExposure(EBurnState::Intact, 0.6f, 1.2f) == EBurnState::Intact);
	TestTrue(TEXT("full exposure ignites"),
			 ABurnablePropActor::StateAfterExposure(EBurnState::Intact, 1.2f, 1.2f) == EBurnState::Burning);
	TestTrue(TEXT("burning never un-ignites"),
			 ABurnablePropActor::StateAfterExposure(EBurnState::Burning, 0.0f, 1.2f) == EBurnState::Burning);
	TestTrue(TEXT("burnt is permanent"),
			 ABurnablePropActor::StateAfterExposure(EBurnState::Burnt, 99.0f, 1.2f) == EBurnState::Burnt);

	ABurnablePropActor* Prop = NewObject<ABurnablePropActor>();
	TestTrue(TEXT("prop starts intact"), Prop->GetBurnState() == EBurnState::Intact);
	TestTrue(TEXT("0.6 s of breath does not ignite"),
			 Prop->AddFireExposure(0.6f) == EBurnState::Intact);
	TestTrue(TEXT("exposure accumulates"), FMath::IsNearlyEqual(Prop->GetExposure(), 0.6f, 0.001f));
	TestTrue(TEXT("second 0.6 s ignites"), Prop->AddFireExposure(0.6f) == EBurnState::Burning);
	TestTrue(TEXT("burning ignores more exposure"),
			 Prop->AddFireExposure(5.0f) == EBurnState::Burning);

	// Through the combat seam: a prop in the cone ignites after
	// 1.2 s of held breath, one behind the dragon never warms.
	ABurnablePropActor* InCone = NewObject<ABurnablePropActor>();
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->Stamina = 100.0f;
	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	TestTrue(TEXT("fire starts"), Combat->TryStartFire(Stats));
	const TArray<UThiefBrain*> NoBrains;
	const TArray<FVector> NoBrainLocations;
	const TArray<AGoatActor*> NoGoats;
	const TArray<FVector> NoGoatLocations;
	const TArray<ABurnablePropActor*> Props = {InCone};
	const TArray<FVector> PropLocations = {FVector(500.0f, 0.0f, 0.0f)};
	Combat->TickFire(1.0f, Stats, FVector::ZeroVector, FVector(1.0f, 0.0f, 0.0f), NoBrains,
					 NoBrainLocations, NoGoats, NoGoatLocations, Props, PropLocations);
	TestTrue(TEXT("1.0 s in cone is not yet lit"), InCone->GetBurnState() == EBurnState::Intact);
	Combat->TickFire(0.3f, Stats, FVector::ZeroVector, FVector(1.0f, 0.0f, 0.0f), NoBrains,
					 NoBrainLocations, NoGoats, NoGoatLocations, Props, PropLocations);
	TestTrue(TEXT("1.3 s in cone ignites through the seam"),
			 InCone->GetBurnState() == EBurnState::Burning);

	return true;
}

// [2026-11-01 T-077] Crush contract: the pure rule (below the 300
// cm/s charge threshold nothing is farmed, at it the full 40 from
// GAMEPLAY.md section 5.4 lands, sign never matters), then that 40
// applied through the exact seams the pawn uses — a 20 HP burglar
// brain and a 30 HP goat both die to one crush, and a corpse takes
// no second count.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCombatCrushTest, "UnderTheMountain.Dragon.CombatCrush",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCombatCrushTest::RunTest(const FString& Parameters)
{
	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	TestEqual(TEXT("spec crush damage is 40"), Combat->CrushDamage, 40.0f);
	TestEqual(TEXT("crush threshold is 300 cm/s"), Combat->CrushMinImpactSpeed, 300.0f);

	TestEqual(TEXT("gentle touchdown crushes nothing"),
			  UDragonCombatComponent::CrushDamageForImpact(100.0f, Combat->CrushDamage,
														   Combat->CrushMinImpactSpeed),
			  0.0f);
	TestEqual(TEXT("just below threshold crushes nothing"),
			  UDragonCombatComponent::CrushDamageForImpact(299.0f, Combat->CrushDamage,
														   Combat->CrushMinImpactSpeed),
			  0.0f);
	TestEqual(TEXT("at threshold the full crush lands"),
			  UDragonCombatComponent::CrushDamageForImpact(300.0f, Combat->CrushDamage,
														   Combat->CrushMinImpactSpeed),
			  40.0f);
	TestEqual(TEXT("a fast fall crushes the same as a charge"),
			  UDragonCombatComponent::CrushDamageForImpact(-1800.0f, Combat->CrushDamage,
														   Combat->CrushMinImpactSpeed),
			  40.0f);
	TestEqual(TEXT("negative damage tuning heals nobody"),
			  UDragonCombatComponent::CrushDamageForImpact(900.0f, -40.0f,
														   Combat->CrushMinImpactSpeed),
			  0.0f);

	// The pawn's landing path hands this damage to the brain seam.
	UTreasureRegistry* Registry = MakeSingleTreasureRegistry(FVector(0.0f, 0.0f, 0.0f));
	UThiefBrain* Burglar = MakeCombatBrain(MakeBurglarConfig(), Registry, TEXT("CrushBurglar"));
	const float ThiefDamage = UDragonCombatComponent::CrushDamageForImpact(
		900.0f, Combat->CrushDamage, Combat->CrushMinImpactSpeed);
	TestTrue(TEXT("one crush kills a 20 HP burglar"),
			 Burglar->ApplyDamage(ThiefDamage, FVector::ZeroVector));
	TestTrue(TEXT("crushed burglar is dead"), Burglar->State() == EThiefState::Dead);
	TestFalse(TEXT("a crushed corpse is not counted twice"),
			  Burglar->ApplyDamage(ThiefDamage, FVector::ZeroVector));

	// And to the goat seam: 30 HP against the same 40.
	AGoatActor* Goat = NewObject<AGoatActor>();
	TestTrue(TEXT("one crush kills a 30 HP goat"), Goat->ApplyGoatDamage(ThiefDamage));
	TestTrue(TEXT("crushed goat is dead and eatable"), Goat->IsDead() && !Goat->IsEaten());
	TestFalse(TEXT("a crushed goat corpse is not counted twice"), Goat->ApplyGoatDamage(ThiefDamage));

	// Below-threshold damage (0) leaves both seams untouched.
	UThiefBrain* Bystander = MakeCombatBrain(MakeBurglarConfig(), Registry, TEXT("GentleBurglar"));
	TestFalse(TEXT("a gentle touchdown kills no burglar"),
			  Bystander->ApplyDamage(UDragonCombatComponent::CrushDamageForImpact(
										 100.0f, Combat->CrushDamage, Combat->CrushMinImpactSpeed),
									 FVector::ZeroVector));
	TestEqual(TEXT("bystander kept full HP"), Bystander->HP(), 20.0f);

	return true;
}

// [2026-11-04 T-076] Claw contract: §5.4 values (25 dmg / -4
// stamina / 450 cm / 110° arc), the verb-timing damage window (no
// T-068 montage notify exists), burglar one-shot, adventurer
// frontal block at gear >= 2 (flank lands), and dwarf-carrier
// priority over a nearer non-carrier.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCombatClawTest, "UnderTheMountain.Dragon.CombatClaw",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCombatClawTest::RunTest(const FString& Parameters)
{
	auto MakeKindConfig = [](EThiefKind Kind, int32 GearTier, float MaxHP, int32 Cap)
	{
		UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
		Config->Kind = Kind;
		Config->GearTier = GearTier;
		Config->MaxHP = MaxHP;
		Config->GrabTimeSeconds = 1.5f;
		Config->CarriedCap = Cap;
		Config->bDropsAllLootOnRoar = true;
		return Config;
	};
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward(1.0f, 0.0f, 0.0f);
	const FVector FacingDragon(-1.0f, 0.0f, 0.0f);
	const FVector FacingAway(1.0f, 0.0f, 0.0f);

	UDragonCombatComponent* Probe = NewObject<UDragonCombatComponent>();
	TestEqual(TEXT("spec claw damage is 25"), Probe->ClawDamage, 25.0f);
	TestEqual(TEXT("spec claw cost is 4"), Probe->ClawCost, 4.0f);
	TestEqual(TEXT("spec claw range is 450"), Probe->ClawRange, 450.0f);
	TestEqual(TEXT("spec claw half-angle is 55 (110 arc)"), Probe->ClawHalfAngleDeg, 55.0f);
	TestTrue(TEXT("dwarf carrier outranks any carrier outranks dwarf outranks other"),
			 UDragonCombatComponent::ClawTargetPriority(EThiefKind::Dwarf, true) >
					 UDragonCombatComponent::ClawTargetPriority(EThiefKind::Burglar, true) &&
				 UDragonCombatComponent::ClawTargetPriority(EThiefKind::Burglar, true) >
					 UDragonCombatComponent::ClawTargetPriority(EThiefKind::Dwarf, false) &&
				 UDragonCombatComponent::ClawTargetPriority(EThiefKind::Dwarf, false) >
					 UDragonCombatComponent::ClawTargetPriority(EThiefKind::Burglar, false));

	// One-shot + window timing + stamina/cooldown gates.
	{
		UTreasureRegistry* Registry = MakeSingleTreasureRegistry(FVector(300.0f, 0.0f, 0.0f));
		UThiefBrain* Burglar =
			MakeCombatBrain(MakeKindConfig(EThiefKind::Burglar, 1, 20.0f, 1), Registry, TEXT("ClawBurglar"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		Stats->Stamina = 100.0f;
		UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
		const TArray<UThiefBrain*> Brains = {Burglar};
		const TArray<FVector> Locations = {FVector(300.0f, 0.0f, 0.0f)};
		const TArray<FVector> Forwards = {FacingAway};
		bool bBlocked = false;

		TestTrue(TEXT("claw swing starts"), Combat->TryClaw(10.0f, Stats));
		TestEqual(TEXT("claw costs 4 stamina (100 -> 96)"), Stats->Stamina, 96.0f);
		TestFalse(TEXT("second swing inside the cooldown is refused"),
				  Combat->TryClaw(10.1f, Stats));
		TestEqual(TEXT("a refused swing is free"), Stats->Stamina, 96.0f);
		TestEqual(TEXT("0.1s in, the damage window has not opened"),
				  Combat->TickClaw(0.1f, Origin, Forward, Brains, Locations, Forwards, bBlocked), 0);
		TestFalse(TEXT("burglar unhurt before the window"), Burglar->State() == EThiefState::Dead);
		TestEqual(TEXT("at the window the claw lands once"),
				  Combat->TickClaw(0.2f, Origin, Forward, Brains, Locations, Forwards, bBlocked), 1);
		TestTrue(TEXT("25 damage one-shots a 20 HP burglar"), Burglar->State() == EThiefState::Dead);
		TestFalse(TEXT("a clean hit is not a block"), bBlocked);
		// [2026-11-04 feel/spam] Spam presses inside the cooldown are
		// [2026-11-04 feel/spam] damage-free and re-arm nothing: the pawn
		// [2026-11-04 feel/spam] may replay clip/sound, TickClaw stays shut.
		TestFalse(TEXT("spam press at 10.2s is damage-refused"), Combat->TryClaw(10.2f, Stats));
		TestFalse(TEXT("spam press at 10.4s is damage-refused"), Combat->TryClaw(10.4f, Stats));
		TestEqual(TEXT("spam presses spend no stamina"), Stats->Stamina, 96.0f);
		TestEqual(TEXT("spam presses land no extra claw hit"),
				  Combat->TickClaw(0.3f, Origin, Forward, Brains, Locations, Forwards, bBlocked), 0);
	}
	// Out of range / out of arc whiffs, and the stamina gate.
	{
		UTreasureRegistry* Registry = MakeSingleTreasureRegistry(FVector(300.0f, 0.0f, 0.0f));
		UThiefBrain* Far = MakeCombatBrain(MakeKindConfig(EThiefKind::Burglar, 1, 20.0f, 1), Registry,
										   TEXT("ClawFar"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		Stats->Stamina = 100.0f;
		UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
		bool bBlocked = false;
		TestTrue(TEXT("whiff swing starts"), Combat->TryClaw(20.0f, Stats));
		TestEqual(TEXT("target at 500cm is beyond the 450 reach"),
				  Combat->TickClaw(0.3f, Origin, Forward, {Far}, {FVector(500.0f, 0.0f, 0.0f)},
								   {FacingAway}, bBlocked),
				  0);
		TestEqual(TEXT("whiffed target kept full HP"), Far->HP(), 20.0f);

		UDragonStatsComponent* LowStats = NewObject<UDragonStatsComponent>();
		LowStats->Stamina = 3.0f;
		UDragonCombatComponent* LowCombat = NewObject<UDragonCombatComponent>();
		TestFalse(TEXT("claw refuses below its 4 stamina cost"), LowCombat->TryClaw(20.0f, LowStats));
		TestEqual(TEXT("refused claw spends nothing"), LowStats->Stamina, 3.0f);
	}
	// Adventurer shield: frontal at gear 2 blocks, flank and gear 1 land.
	{
		UTreasureRegistry* Registry = MakeSingleTreasureRegistry(FVector(300.0f, 0.0f, 0.0f));
		const FVector At(300.0f, 0.0f, 0.0f);
		UThiefBrain* Frontal = MakeCombatBrain(MakeKindConfig(EThiefKind::Adventurer, 2, 80.0f, 2),
											   Registry, TEXT("ClawAdvFront"));
		UThiefBrain* Flank = MakeCombatBrain(MakeKindConfig(EThiefKind::Adventurer, 2, 80.0f, 2),
											 Registry, TEXT("ClawAdvFlank"));
		UThiefBrain* LowGear = MakeCombatBrain(MakeKindConfig(EThiefKind::Adventurer, 1, 80.0f, 2),
											   Registry, TEXT("ClawAdvGear1"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		Stats->Stamina = 100.0f;
		bool bBlocked = false;

		UDragonCombatComponent* C1 = NewObject<UDragonCombatComponent>();
		TestTrue(TEXT("frontal swing starts"), C1->TryClaw(30.0f, Stats));
		TestEqual(TEXT("frontal claw vs gear-2 shield hits nothing"),
				  C1->TickClaw(0.3f, Origin, Forward, {Frontal}, {At}, {FacingDragon}, bBlocked), 0);
		TestTrue(TEXT("the block is reported"), bBlocked);
		TestEqual(TEXT("blocked adventurer kept full HP"), Frontal->HP(), 80.0f);

		UDragonCombatComponent* C2 = NewObject<UDragonCombatComponent>();
		TestTrue(TEXT("flank swing starts"), C2->TryClaw(30.0f, Stats));
		TestEqual(TEXT("flank claw lands (tail/flank rule)"),
				  C2->TickClaw(0.3f, Origin, Forward, {Flank}, {At}, {FacingAway}, bBlocked), 1);
		TestEqual(TEXT("flanked adventurer takes 25 (80 -> 55)"), Flank->HP(), 55.0f);

		UDragonCombatComponent* C3 = NewObject<UDragonCombatComponent>();
		TestTrue(TEXT("gear-1 frontal swing starts"), C3->TryClaw(30.0f, Stats));
		TestEqual(TEXT("gear-1 frontal claw lands (block needs tier >= 2)"),
				  C3->TickClaw(0.3f, Origin, Forward, {LowGear}, {At}, {FacingDragon}, bBlocked), 1);
		TestEqual(TEXT("gear-1 adventurer takes 25"), LowGear->HP(), 55.0f);
	}
	// Dwarf carrier priority: the carrier (farther) is hit, the
	// nearer non-carrier is not.
	{
		UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
		FTreasureEntry Loot;
		Loot.Id = FName(TEXT("CarrierLoot"));
		Loot.GoldValue = 100;
		Loot.Location = FVector(320.0f, 0.0f, 0.0f);
		Registry->Register(Loot);
		UThiefBrain* Carrier = MakeCombatBrain(MakeKindConfig(EThiefKind::Dwarf, 1, 45.0f, 3),
											   Registry, TEXT("ClawDwarfCarrier"));
		GrabBestTreasureForCombat(Carrier, FVector(320.0f, 0.0f, 0.0f));
		TestTrue(TEXT("dwarf is carrying before the claw"), Carrier->IsCarryingLoot());
		UThiefBrain* Nearer = MakeCombatBrain(MakeKindConfig(EThiefKind::Burglar, 1, 20.0f, 1),
											  Registry, TEXT("ClawNearBurglar"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		Stats->Stamina = 100.0f;
		UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
		bool bBlocked = false;
		TestTrue(TEXT("priority swing starts"), Combat->TryClaw(40.0f, Stats));
		TestEqual(TEXT("claw hits exactly one target"),
				  Combat->TickClaw(0.3f, Origin, Forward, {Nearer, Carrier},
								   {FVector(200.0f, 0.0f, 0.0f), FVector(320.0f, 0.0f, 0.0f)},
								   {FacingAway, FacingAway}, bBlocked),
				  1);
		TestEqual(TEXT("dwarf carrier took the 25 (45 -> 20)"), Carrier->HP(), 20.0f);
		TestEqual(TEXT("nearer non-carrier is untouched"), Nearer->HP(), 20.0f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
