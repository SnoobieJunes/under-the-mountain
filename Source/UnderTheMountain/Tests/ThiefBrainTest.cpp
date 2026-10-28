// Copyright Under the Mountain contributors.
// Purpose: T-014 thief contract tests — the UThiefBrain state machine
// (sneak/grab/flee/scare/death against a real UTreasureRegistry) and
// the dwarf trap budget — all exercised headless, no world spawned,
// because the brain is pure logic by design.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
// [2026-10-28 T-014] Fixed hoard geography for every scenario: three
// pieces at distinct values/locations so "greediest first" (Crown,
// then Goblet) is observable, and an exit far from all of them so a
// Fleeing thief never accidentally "arrives" mid-scenario.
const FVector TrinketLocation(0.0f, 0.0f, 0.0f);
const FVector GobletLocation(1000.0f, 0.0f, 0.0f);
const FVector CrownLocation(2000.0f, 0.0f, 0.0f);
const FVector ExitLocation(-5000.0f, 0.0f, 0.0f);

UTreasureRegistry* MakeHoard()
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();

	FTreasureEntry Trinket;
	Trinket.Id = FName(TEXT("Trinket"));
	Trinket.GoldValue = 10;
	Trinket.Location = TrinketLocation;
	Registry->Register(Trinket);

	FTreasureEntry Goblet;
	Goblet.Id = FName(TEXT("Goblet"));
	Goblet.GoldValue = 250;
	Goblet.Location = GobletLocation;
	Registry->Register(Goblet);

	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1200;
	Crown.Location = CrownLocation;
	Registry->Register(Crown);

	return Registry;
}

// [2026-10-28 T-014] Configs are built in code (NewObject + fields) so
// the test needs no authored .uassets and every knob a scenario cares
// about is visible at its call site.
UThiefDataAsset* MakeConfig(int32 CarriedCap, bool bDropsAll, bool bCanTrap)
{
	UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
	Config->MaxHP = 20.0f;
	Config->GrabTimeSeconds = 1.5f;
	Config->CarriedCap = CarriedCap;
	Config->bDropsAllLootOnRoar = bDropsAll;
	Config->bCanPlaceTraps = bCanTrap;
	Config->MaxTraps = 3;
	return Config;
}

UThiefBrain* MakeBrain(UThiefDataAsset* Config, UTreasureRegistry* Registry, const TCHAR* Name)
{
	UThiefBrain* Brain = NewObject<UThiefBrain>();
	Brain->Init(Config, Registry, FName(Name));
	Brain->SetExitLocation(ExitLocation);
	return Brain;
}

// [2026-10-28 T-014] Drives one full grab of whatever the brain thinks
// is richest, standing exactly on it: one tick to arrive (Sneak->Grab)
// and one tick of grab time (Grab->next state).
void GrabBestTreasure(UThiefBrain* Brain, const FVector& TreasureLocation)
{
	Brain->Tick(0.1f, TreasureLocation);
	Brain->Tick(1.5f, TreasureLocation);
}
} // namespace

// [2026-10-28 T-014] The state-machine contract: greed order, grab
// timing, carried-cap flee, scent-follow location refresh, roar drops
// (all vs exactly one), scare expiry, and death dropping everything.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefStateTest, "UnderTheMountain.Thief.States",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefStateTest::RunTest(const FString& Parameters)
{
	// --- Burglar (cap 1, drops all on roar) ---
	{
		UTreasureRegistry* Registry = MakeHoard();
		UThiefBrain* Brain = MakeBrain(MakeConfig(1, true, false), Registry, TEXT("Burglar"));

		TestTrue(TEXT("burglar starts sneaking"), Brain->State() == EThiefState::Sneak);
		TestEqual(TEXT("burglar starts at config HP"), Brain->HP(), 20.0f);

		// Tick from far away: destination must be the 1200 crown, not
		// the nearer-or-first pieces — the greed rule.
		Brain->Tick(0.1f, FVector(9000.0f, 0.0f, 0.0f));
		TestTrue(TEXT("still sneaking from afar"), Brain->State() == EThiefState::Sneak);
		TestEqual(TEXT("sneak destination is the richest treasure"), Brain->CurrentDestination(),
				  CrownLocation);

		Brain->Tick(0.1f, CrownLocation);
		TestTrue(TEXT("arriving at the crown starts the grab"), Brain->State() == EThiefState::Grab);

		Brain->Tick(1.5f, CrownLocation);
		TestTrue(TEXT("grabbing to cap flees"), Brain->State() == EThiefState::Flee);
		TestEqual(TEXT("flee destination is the exit"), Brain->CurrentDestination(), ExitLocation);
		TestEqual(TEXT("carrying exactly the crown"), Brain->CarriedItems().Num(), 1);
		TestTrue(TEXT("crown is the carried item"),
				 Brain->CarriedItems().Contains(FName(TEXT("Crown"))));

		const FTreasureEntry* Crown = Registry->Find(FName(TEXT("Crown")));
		TestNotNull(TEXT("crown still registered"), Crown);
		if (Crown != nullptr)
		{
			TestTrue(TEXT("registry marks crown carried"), Crown->bCarried);
			TestEqual(TEXT("carrier is the burglar"), Crown->CarrierId, FName(TEXT("Burglar")));
		}

		// Carried loot rides at the thief's location (scent follows).
		const FVector MovedTo(1500.0f, 300.0f, 0.0f);
		Brain->Tick(0.1f, MovedTo);
		Crown = Registry->Find(FName(TEXT("Crown")));
		TestNotNull(TEXT("crown findable after move"), Crown);
		if (Crown != nullptr)
		{
			TestEqual(TEXT("carried location follows the thief"), Crown->Location, MovedTo);
		}

		// Non-lethal damage wounds but does not kill or drop.
		TestFalse(TEXT("non-lethal damage does not kill"), Brain->ApplyDamage(5.0f, MovedTo));
		TestEqual(TEXT("hp reduced by damage"), Brain->HP(), 15.0f);
		TestTrue(TEXT("still fleeing after a wound"), Brain->State() == EThiefState::Flee);

		// Roar: drop-all config drops the crown where the thief stands.
		const FVector RoarLocation(1400.0f, 250.0f, 0.0f);
		Brain->OnRoar(RoarLocation);
		TestTrue(TEXT("roar scares the burglar"), Brain->State() == EThiefState::Scared);
		TestEqual(TEXT("scared destination is the exit"), Brain->CurrentDestination(),
				  ExitLocation);
		TestEqual(TEXT("roar dropped everything"), Brain->CarriedItems().Num(), 0);
		Crown = Registry->Find(FName(TEXT("Crown")));
		TestNotNull(TEXT("crown findable after roar"), Crown);
		if (Crown != nullptr)
		{
			TestFalse(TEXT("crown uncarried after roar"), Crown->bCarried);
			TestEqual(TEXT("crown rests at the roar location"), Crown->Location, RoarLocation);
		}

		// ScaredSeconds is 6; a 7 s tick must settle into plain Flee.
		Brain->Tick(7.0f, RoarLocation);
		TestTrue(TEXT("scare expiry returns to flee"), Brain->State() == EThiefState::Flee);
	}

	// --- Adventurer (cap 2, drops all): two grabs, then death drops both ---
	{
		UTreasureRegistry* Registry = MakeHoard();
		UThiefBrain* Brain = MakeBrain(MakeConfig(2, true, false), Registry, TEXT("Adventurer"));

		GrabBestTreasure(Brain, CrownLocation);
		TestTrue(TEXT("under cap goes back for more"), Brain->State() == EThiefState::Sneak);
		TestEqual(TEXT("carrying one after first grab"), Brain->CarriedItems().Num(), 1);

		// Second-richest uncarried piece is the 250 goblet.
		Brain->Tick(0.1f, FVector(9000.0f, 0.0f, 0.0f));
		TestEqual(TEXT("next destination is the goblet"), Brain->CurrentDestination(),
				  GobletLocation);

		GrabBestTreasure(Brain, GobletLocation);
		TestTrue(TEXT("second grab reaches cap and flees"), Brain->State() == EThiefState::Flee);
		TestEqual(TEXT("carrying two"), Brain->CarriedItems().Num(), 2);
		TestTrue(TEXT("crown carried"), Brain->CarriedItems().Contains(FName(TEXT("Crown"))));
		TestTrue(TEXT("goblet carried"), Brain->CarriedItems().Contains(FName(TEXT("Goblet"))));

		const FVector DeathLocation(500.0f, 500.0f, 0.0f);
		TestTrue(TEXT("lethal damage reports death"), Brain->ApplyDamage(999.0f, DeathLocation));
		TestTrue(TEXT("state is dead"), Brain->State() == EThiefState::Dead);
		TestEqual(TEXT("hp clamped at zero"), Brain->HP(), 0.0f);
		TestEqual(TEXT("death empties the hands"), Brain->CarriedItems().Num(), 0);

		const FTreasureEntry* Crown = Registry->Find(FName(TEXT("Crown")));
		const FTreasureEntry* Goblet = Registry->Find(FName(TEXT("Goblet")));
		TestNotNull(TEXT("crown findable after death"), Crown);
		TestNotNull(TEXT("goblet findable after death"), Goblet);
		if (Crown != nullptr && Goblet != nullptr)
		{
			TestFalse(TEXT("crown dropped on death"), Crown->bCarried);
			TestFalse(TEXT("goblet dropped on death"), Goblet->bCarried);
			TestEqual(TEXT("crown dropped at death location"), Crown->Location, DeathLocation);
			TestEqual(TEXT("goblet dropped at death location"), Goblet->Location, DeathLocation);
		}
	}

	// --- Dwarf (cap 3, drops exactly one on roar) ---
	{
		UTreasureRegistry* Registry = MakeHoard();
		UThiefBrain* Brain = MakeBrain(MakeConfig(3, false, true), Registry, TEXT("Dwarf"));

		GrabBestTreasure(Brain, CrownLocation);
		GrabBestTreasure(Brain, GobletLocation);
		TestEqual(TEXT("dwarf carrying two"), Brain->CarriedItems().Num(), 2);

		const FVector RoarLocation(800.0f, -400.0f, 0.0f);
		Brain->OnRoar(RoarLocation);
		TestTrue(TEXT("roar scares the dwarf too"), Brain->State() == EThiefState::Scared);
		TestEqual(TEXT("dwarf drops exactly one item"), Brain->CarriedItems().Num(), 1);
		// The fumbled item is the first grabbed (the crown).
		TestTrue(TEXT("goblet is kept"), Brain->CarriedItems().Contains(FName(TEXT("Goblet"))));

		const FTreasureEntry* Crown = Registry->Find(FName(TEXT("Crown")));
		const FTreasureEntry* Goblet = Registry->Find(FName(TEXT("Goblet")));
		TestNotNull(TEXT("crown findable after dwarf roar"), Crown);
		TestNotNull(TEXT("goblet findable after dwarf roar"), Goblet);
		if (Crown != nullptr && Goblet != nullptr)
		{
			TestFalse(TEXT("crown was the dropped one"), Crown->bCarried);
			TestEqual(TEXT("crown rests at roar location"), Crown->Location, RoarLocation);
			TestTrue(TEXT("goblet still carried"), Goblet->bCarried);
		}
	}

	return true;
}

// [2026-10-28 T-014] Trap rules: only trap-configured thieves, only
// while Sneaking, only up to the config budget; the slow tuning is
// read through from the config.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefTrapTest, "UnderTheMountain.Thief.Traps",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefTrapTest::RunTest(const FString& Parameters)
{
	UTreasureRegistry* Registry = MakeHoard();

	UThiefDataAsset* DwarfConfig = MakeConfig(3, false, true);
	DwarfConfig->TrapSlowFactor = 0.25f;
	DwarfConfig->TrapSlowSeconds = 2.5f;
	UThiefBrain* Dwarf = MakeBrain(DwarfConfig, Registry, TEXT("TrapDwarf"));

	TestEqual(TEXT("no traps placed yet"), Dwarf->TrapsPlaced(), 0);
	TestEqual(TEXT("slow factor reads from config"), Dwarf->TrapSlowFactor(), 0.25f);
	TestEqual(TEXT("slow seconds read from config"), Dwarf->TrapSlowSeconds(), 2.5f);

	// A fresh brain is Sneaking, so the budget of 3 is available now.
	TestTrue(TEXT("first trap placed"), Dwarf->TryPlaceTrap());
	TestTrue(TEXT("second trap placed"), Dwarf->TryPlaceTrap());
	TestTrue(TEXT("third trap placed"), Dwarf->TryPlaceTrap());
	TestEqual(TEXT("three traps placed"), Dwarf->TrapsPlaced(), 3);
	TestFalse(TEXT("trap budget is exhausted at MaxTraps"), Dwarf->TryPlaceTrap());
	TestEqual(TEXT("count stays at the budget"), Dwarf->TrapsPlaced(), 3);

	// A non-trap config can never place, whatever its state.
	UThiefBrain* Burglar = MakeBrain(MakeConfig(1, true, false), Registry, TEXT("TrapBurglar"));
	TestFalse(TEXT("burglar config cannot place traps"), Burglar->TryPlaceTrap());
	TestEqual(TEXT("burglar placed nothing"), Burglar->TrapsPlaced(), 0);

	// State gate: a roared (Scared) dwarf may not place mid-panic.
	UTreasureRegistry* ScaredRegistry = MakeHoard();
	UThiefBrain* ScaredDwarf = MakeBrain(MakeConfig(3, false, true), ScaredRegistry, TEXT("ScaredDwarf"));
	ScaredDwarf->OnRoar(FVector(100.0f, 0.0f, 0.0f));
	TestTrue(TEXT("dwarf is scared after roar"), ScaredDwarf->State() == EThiefState::Scared);
	TestFalse(TEXT("scared thief cannot place traps"), ScaredDwarf->TryPlaceTrap());

	return true;
}

// [2026-10-28 T-014] The dragon half of the trap contract: one
// application halves both speed caps and starts the clock; the
// countdown in Tick restores the exact original speeds at expiry
// and never stacks a second application into a quarter-speed trap.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefTrapSlowTest, "UnderTheMountain.Thief.TrapSlow",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefTrapSlowTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Dragon = NewObject<ADragonCharacter>();
	UCharacterMovementComponent* Move = Dragon->GetCharacterMovement();
	const float WalkBefore = Move->MaxWalkSpeed;
	const float FlyBefore = Move->MaxFlySpeed;

	Dragon->ApplyTrapSlow(0.5f, 1.0f);
	TestTrue(TEXT("trap slow is active after application"), Dragon->IsTrapSlowed());
	TestEqual(TEXT("walk speed halved"), Move->MaxWalkSpeed, WalkBefore * 0.5f);
	TestEqual(TEXT("fly speed halved"), Move->MaxFlySpeed, FlyBefore * 0.5f);

	Dragon->ApplyTrapSlow(0.5f, 1.0f);
	TestEqual(TEXT("second trap refreshes, never stacks"), Move->MaxWalkSpeed, WalkBefore * 0.5f);

	// AActor::Tick is the public entry; ADragonCharacter's override
	// narrows it to protected.
	static_cast<AActor*>(Dragon)->Tick(2.0f);
	TestFalse(TEXT("trap slow expires on its countdown"), Dragon->IsTrapSlowed());
	TestEqual(TEXT("walk speed restored exactly"), Move->MaxWalkSpeed, WalkBefore);
	TestEqual(TEXT("fly speed restored exactly"), Move->MaxFlySpeed, FlyBefore);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
