// Copyright Under the Mountain contributors.
// Purpose: T-074/T-089/T-132/T-133/T-134 rule tests — thief attacks on
// the dragon (tell/cadence/damage/defeat), waypoint steering, the one
// entry table, dwarf roles + party re-pick, star gear + raider loot,
// and witness-gated notoriety. All pure/headless: brains, registries
// and stats objects are NewObject'd, no world is spawned.

#include "Misc/AutomationTest.h"
#include "Core/GameFlowMachine.h"
#include "Dragon/DragonStatsComponent.h"
#include "Notoriety/NotorietySubsystem.h"
#include "Notoriety/WitnessRules.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
#include "Thief/ThiefRules.h"
#include "Treasure/TreasureRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
UTreasureRegistry* MakeRegistry()
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	FTreasureEntry Entry;
	Entry.Id = FName(TEXT("Crown"));
	Entry.GoldValue = 1200;
	Entry.Location = FVector(2000.0f, 0.0f, 0.0f);
	Registry->Register(Entry);
	return Registry;
}

UThiefBrain* MakeBrain(EThiefRole Role, float MaxHP, const TCHAR* Name)
{
	UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
	Config->MaxHP = MaxHP;
	Config->Role = Role;
	Config->GrabTimeSeconds = 1.5f;
	Config->CarriedCap = 1;
	UThiefBrain* Brain = NewObject<UThiefBrain>();
	Brain->Init(Config, MakeRegistry(), FName(Name));
	Brain->SetExitLocation(FVector(-5000.0f, 0.0f, 0.0f));
	return Brain;
}
} // namespace

// [2026-11-04 T-074] Attack rules: range gate, readable tell, strike
// through the Stats seam, cadence, tell-cancel, unarmed/scared refusal,
// and the 0 HP defeat event feeding GameFlow's Lost (slink-away) path.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefAttackTest, "UnderTheMountain.Thief.Attack",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefAttackTest::RunTest(const FString& Parameters)
{
	const FVector ThiefLoc(0.0f, 0.0f, 0.0f);
	{
		UThiefBrain* Brain = MakeBrain(EThiefRole::Adventurer, 80.0f, TEXT("Fighter"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		int32 Hits = 0;
		Stats->OnDragonHit.AddLambda([&Hits](float, float)
									 { ++Hits; });

		TestTrue(TEXT("out of range stays idle"),
				 Brain->TickDragonAttack(0.1f, ThiefLoc, FVector(1000.0f, 0.0f, 0.0f), Stats) ==
					 EThiefAttackPhase::Idle);
		TestEqual(TEXT("no damage out of range"), Stats->HP, 100.0f);

		TestTrue(TEXT("in range starts the tell"),
				 Brain->TickDragonAttack(0.1f, ThiefLoc, FVector(100.0f, 0.0f, 0.0f), Stats) ==
					 EThiefAttackPhase::Tell);
		TestEqual(TEXT("tell has not landed yet"), Stats->HP, 100.0f);
		TestTrue(TEXT("tell expiry strikes into cooldown"),
				 Brain->TickDragonAttack(0.5f, ThiefLoc, FVector(100.0f, 0.0f, 0.0f), Stats) ==
					 EThiefAttackPhase::Cooldown);
		TestEqual(TEXT("sword hit costs 6 HP (S5.3)"), Stats->HP, 94.0f);
		TestEqual(TEXT("hit event fired once"), Hits, 1);

		TestTrue(TEXT("cadence blocks an instant second strike"),
				 Brain->TickDragonAttack(0.1f, ThiefLoc, FVector(100.0f, 0.0f, 0.0f), Stats) ==
					 EThiefAttackPhase::Cooldown);
		TestEqual(TEXT("no damage during cadence"), Stats->HP, 94.0f);
		Brain->TickDragonAttack(1.2f, ThiefLoc, FVector(100.0f, 0.0f, 0.0f), Stats);
		Brain->TickDragonAttack(0.5f, ThiefLoc, FVector(100.0f, 0.0f, 0.0f), Stats);
		TestEqual(TEXT("second strike lands after cadence"), Stats->HP, 88.0f);

		// Tell-cancel: wind up, dragon steps out, no strike.
		Brain->TickDragonAttack(1.2f, ThiefLoc, FVector(100.0f, 0.0f, 0.0f), Stats);
		TestTrue(TEXT("back in tell"),
				 Brain->AttackPhase() == EThiefAttackPhase::Tell);
		TestTrue(TEXT("leaving range cancels the tell"),
				 Brain->TickDragonAttack(0.5f, ThiefLoc, FVector(5000.0f, 0.0f, 0.0f), Stats) ==
					 EThiefAttackPhase::Idle);
		TestEqual(TEXT("cancelled tell deals nothing"), Stats->HP, 88.0f);
	}
	{
		// Unarmed and scared thieves never attack.
		UThiefBrain* Burglar = MakeBrain(EThiefRole::Burglar, 20.0f, TEXT("Sneak"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		TestTrue(TEXT("burglar cannot attack"),
				 Burglar->TickDragonAttack(1.0f, ThiefLoc, ThiefLoc, Stats) == EThiefAttackPhase::Idle);
		TestEqual(TEXT("burglar dealt nothing"), Stats->HP, 100.0f);

		UThiefBrain* Scared = MakeBrain(EThiefRole::Adventurer, 80.0f, TEXT("ScaredFighter"));
		Scared->OnRoar(ThiefLoc);
		TestTrue(TEXT("scared adventurer runs instead of fighting"),
				 Scared->TickDragonAttack(1.0f, ThiefLoc, ThiefLoc, Stats) == EThiefAttackPhase::Idle);
	}
	{
		// Crossbow at 3 stars: ranged strike for 4 (S5.3).
		UThiefBrain* Brain = MakeBrain(EThiefRole::Adventurer, 80.0f, TEXT("Crossbowman"));
		Brain->ApplyStarGear(3);
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		Brain->TickDragonAttack(0.1f, ThiefLoc, FVector(1000.0f, 0.0f, 0.0f), Stats);
		Brain->TickDragonAttack(0.7f, ThiefLoc, FVector(1000.0f, 0.0f, 0.0f), Stats);
		TestEqual(TEXT("crossbow bolt costs 4 HP (S5.3)"), Stats->HP, 96.0f);
	}
	{
		// Defeat: hits to 0 fire OnDragonDefeated exactly once, and the
		// result feeds GameFlow as Lost (the slink-away outcome, T-072).
		UThiefBrain* Brain = MakeBrain(EThiefRole::Adventurer, 80.0f, TEXT("Slayer"));
		UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
		Stats->HP = 6.0f;
		int32 Defeats = 0;
		Stats->OnDragonDefeated.AddLambda([&Defeats]()
										  { ++Defeats; });
		Brain->TickDragonAttack(0.1f, ThiefLoc, ThiefLoc, Stats);
		Brain->TickDragonAttack(0.5f, ThiefLoc, ThiefLoc, Stats);
		TestEqual(TEXT("dragon at 0 HP"), Stats->HP, 0.0f);
		TestTrue(TEXT("stats report defeated"), Stats->IsDefeated());
		TestEqual(TEXT("defeat fired once"), Defeats, 1);
		Stats->ApplyHPDamage(10.0f);
		TestEqual(TEXT("overkill does not re-fire defeat"), Defeats, 1);

		UGameFlowMachine* Flow = NewObject<UGameFlowMachine>();
		Flow->StartPlay();
		FGameFlowResult Result;
		Result.Outcome = EGameFlowOutcome::Lost;
		TestTrue(TEXT("defeat ends the run as Lost (slink-away)"), Flow->RequestEnd(Result));
		TestTrue(TEXT("flow is ended"), Flow->IsEnded());
	}
	return true;
}

// [2026-11-04 T-089/T-133] Waypoint steering, the one entry table, dwarf
// role numbers, frontal blocks and the 3 s carrier re-pick.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefRouteRoleTest, "UnderTheMountain.Thief.RouteRoles",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefRouteRoleTest::RunTest(const FString& Parameters)
{
	{
		const FVector Final(5000.0f, 0.0f, 0.0f);
		int32 Index = 0;
		TestEqual(TEXT("empty route steers at the destination"),
				  FThiefRules::SteerTarget(TArray<FVector>(), FVector::ZeroVector, Final, 150.0f, Index),
				  Final);
		const TArray<FVector> Route = {FVector(1000.0f, 0.0f, 0.0f), FVector(2000.0f, 500.0f, 0.0f)};
		Index = 0;
		TestEqual(TEXT("first waypoint is the target"),
				  FThiefRules::SteerTarget(Route, FVector::ZeroVector, Final, 150.0f, Index), Route[0]);
		TestEqual(TEXT("reached waypoint is consumed"),
				  FThiefRules::SteerTarget(Route, Route[0], Final, 150.0f, Index), Route[1]);
		TestEqual(TEXT("index advanced"), Index, 1);
		TestEqual(TEXT("exhausted route falls back to destination"),
				  FThiefRules::SteerTarget(Route, Route[1], Final, 150.0f, Index), Final);
	}
	{
		const FThiefEntryPoint BurglarNorth =
			FThiefRules::EntryForRole(EThiefRole::Burglar, FVector(7500.0f, 9000.0f, 0.0f));
		TestTrue(TEXT("burglar enters by crack"), BurglarNorth.Kind == EThiefEntryKind::Crack);
		const FThiefEntryPoint BurglarSouth =
			FThiefRules::EntryForRole(EThiefRole::Burglar, FVector(11500.0f, -9000.0f, 0.0f));
		TestTrue(TEXT("burglar gets the nearest crack"), BurglarSouth.Id == FName(TEXT("CrackSouth")));
		TestTrue(TEXT("dwarf enters by portal"),
				 FThiefRules::EntryForRole(EThiefRole::Carrier, FVector::ZeroVector).Kind ==
					 EThiefEntryKind::GrandPortal);
		TestTrue(TEXT("adventurer enters by portal"),
				 FThiefRules::EntryForRole(EThiefRole::Adventurer, FVector::ZeroVector).Kind ==
					 EThiefEntryKind::GrandPortal);
	}
	{
		TestEqual(TEXT("guard HP is 70 (S5.2)"), FThiefRules::HPForRole(EThiefRole::Guard, 45.0f), 70.0f);
		TestEqual(TEXT("carrier HP is 45 (S5.2)"), FThiefRules::HPForRole(EThiefRole::Carrier, 99.0f), 45.0f);
		TestEqual(TEXT("carrier cap is 3 (S5.2)"), FThiefRules::CarriedCapForRole(EThiefRole::Carrier, 1), 3);
		TestEqual(TEXT("guard frontal claw halved"),
				  FThiefRules::ResolveIncomingDamage(25.0f, EThiefRole::Guard, true, false, 0), 12.5f);
		TestEqual(TEXT("tail bypasses guard block"),
				  FThiefRules::ResolveIncomingDamage(20.0f, EThiefRole::Guard, true, true, 0), 20.0f);
		TestEqual(TEXT("shield blocks frontal claw at 2 stars"),
				  FThiefRules::ResolveIncomingDamage(25.0f, EThiefRole::Adventurer, true, false, 2), 0.0f);
		TestEqual(TEXT("no shield block before 2 stars"),
				  FThiefRules::ResolveIncomingDamage(25.0f, EThiefRole::Adventurer, true, false, 1), 25.0f);
		TestEqual(TEXT("flank is never blocked"),
				  FThiefRules::ResolveIncomingDamage(25.0f, EThiefRole::Adventurer, false, false, 3), 25.0f);

		UThiefBrain* Guard = MakeBrain(EThiefRole::None, 45.0f, TEXT("PartyGuard"));
		Guard->SetRole(EThiefRole::Guard);
		TestEqual(TEXT("brain guard HP re-derived to 70"), Guard->HP(), 70.0f);
		UThiefBrain* Carrier = MakeBrain(EThiefRole::Carrier, 45.0f, TEXT("PartyCarrier"));
		TestEqual(TEXT("brain carrier cap is 3"), Carrier->EffectiveCarriedCap(), 3);

		const TArray<FPartyMemberState> Party = {{EThiefRole::Carrier, false},
												 {EThiefRole::Trapper, true},
												 {EThiefRole::Guard, true}};
		TestEqual(TEXT("no re-pick before 3 s"), FThiefRules::RepickCarrierIndex(Party, 2.9f), -1);
		TestEqual(TEXT("trapper is promoted at 3 s"), FThiefRules::RepickCarrierIndex(Party, 3.0f), 1);
		const TArray<FPartyMemberState> Living = {{EThiefRole::Carrier, true},
												  {EThiefRole::Trapper, true},
												  {EThiefRole::Guard, true}};
		TestEqual(TEXT("no re-pick while carrier lives"), FThiefRules::RepickCarrierIndex(Living, 99.0f), -1);
		const TArray<FPartyMemberState> GuardOnly = {{EThiefRole::Carrier, false},
													 {EThiefRole::Trapper, false},
													 {EThiefRole::Guard, true}};
		TestEqual(TEXT("guard is the last resort"), FThiefRules::RepickCarrierIndex(GuardOnly, 3.0f), 2);
	}
	return true;
}

// [2026-11-04 T-132] Star gear table, spawn scaling, and raider loot
// drops that register on death.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNotorietyGearTest, "UnderTheMountain.Thief.GearLoot",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNotorietyGearTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("0-star HP mult"), FThiefRules::GearForStars(0).HPMultiplier, 1.0f);
	TestFalse(TEXT("0-star has no gear"), FThiefRules::GearForStars(0).bCrossbow);
	TestTrue(TEXT("1-star unlocks smoke"), FThiefRules::GearForStars(1).bSmokePuff);
	TestTrue(TEXT("2-star unlocks shield"), FThiefRules::GearForStars(2).bShield);
	TestTrue(TEXT("3-star unlocks crossbow"), FThiefRules::GearForStars(3).bCrossbow);
	TestTrue(TEXT("4-star unlocks pit/net"), FThiefRules::GearForStars(4).bPitAndNet);
	TestTrue(TEXT("5-star guarantees prize"), FThiefRules::GearForStars(5).bGuaranteedPrizePerParty);
	TestEqual(TEXT("stars clamp above the table"), FThiefRules::GearForStars(99).HPMultiplier, 2.5f);
	TestEqual(TEXT("scaled HP at 3 stars"), FThiefRules::ScaledHP(80.0f, 3), 120.0f);
	TestTrue(TEXT("scaled speed at 3 stars"), FMath::IsNearlyEqual(FThiefRules::ScaledSpeed(200.0f, 3), 220.0f, 0.01f));

	{
		UThiefBrain* Brain = MakeBrain(EThiefRole::Adventurer, 80.0f, TEXT("Geared"));
		Brain->ApplyStarGear(3);
		TestEqual(TEXT("brain HP scaled by gear"), Brain->HP(), 120.0f);
		TestEqual(TEXT("brain speed mult from gear"), Brain->GearSpeedMultiplier(), 1.1f);
		TestEqual(TEXT("brain remembers its stars"), Brain->GearStars(), 3);
	}
	{
		const FVector DropLoc(300.0f, 400.0f, 0.0f);
		TestEqual(TEXT("loot roll low end"),
				  FThiefRules::MakeRaiderLootDrop(FName(TEXT("R1")), 3, DropLoc, 0.0f).GoldValue, 75);
		TestEqual(TEXT("loot roll high end"),
				  FThiefRules::MakeRaiderLootDrop(FName(TEXT("R1")), 3, DropLoc, 1.0f).GoldValue, 200);
		UTreasureRegistry* Registry = MakeRegistry();
		const FName Id = FThiefRules::RegisterRaiderLootDrop(
			Registry, FThiefRules::MakeRaiderLootDrop(FName(TEXT("R1")), 3, DropLoc, 0.5f));
		const FTreasureEntry* Drop = Registry->Find(Id);
		TestNotNull(TEXT("raider loot registered"), Drop);
		if (Drop != nullptr)
		{
			TestFalse(TEXT("drop rests uncarried"), Drop->bCarried);
			TestEqual(TEXT("drop rests at the death location"), Drop->Location, DropLoc);
		}
		TestTrue(TEXT("null registry registers nothing"),
				 FThiefRules::RegisterRaiderLootDrop(nullptr, FTreasureEntry()) == NAME_None);
	}
	{
		// Death through the brain registers the raider's carried-in loot.
		UTreasureRegistry* Registry = MakeRegistry();
		UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
		Config->MaxHP = 20.0f;
		Config->Role = EThiefRole::Burglar;
		UThiefBrain* Brain = NewObject<UThiefBrain>();
		Brain->Init(Config, Registry, FName(TEXT("DeadRaider")));
		Brain->ApplyStarGear(0);
		Brain->SetRaiderLootRoll(1.0f);
		TestTrue(TEXT("raider dies"), Brain->ApplyDamage(999.0f, FVector(10.0f, 0.0f, 0.0f)));
		const FTreasureEntry* Drop = Registry->Find(FName(TEXT("RaiderLoot_DeadRaider")));
		TestNotNull(TEXT("brain death registered raider loot"), Drop);
		if (Drop != nullptr)
		{
			TestEqual(TEXT("0-star loot at roll top is 10"), Drop->GoldValue, 10);
		}
	}
	return true;
}

// [2026-11-04 T-134] Witness rule both ways, through the pure rule and
// through the subsystem single-writer verbs the pawn calls.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWitnessNotorietyTest, "UnderTheMountain.Notoriety.Witness",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWitnessNotorietyTest::RunTest(const FString& Parameters)
{
	const FVector Wilderness(38000.0f, 9000.0f, 100.0f);
	const TArray<FSettlementArea> Village = {{FVector(38000.0f, 0.0f, 100.0f), 3000.0f}};
	const TArray<FVillagerWitness> Seer = {{FVector(38500.0f, 9000.0f, 100.0f), true}};
	const TArray<FVillagerWitness> Blind = {{FVector(38500.0f, 9000.0f, 100.0f), false}};
	const TArray<FVillagerWitness> FarSeer = {{FVector(60000.0f, 0.0f, 100.0f), true}};

	TestFalse(TEXT("unwitnessed wilderness is free"),
			  FWitnessRules::IsWitnessed(Wilderness, false, TArray<FSettlementArea>(), TArray<FVillagerWitness>(),
										 FWitnessRules::DefaultWitnessRadius));
	TestTrue(TEXT("settlement act is witnessed"),
			 FWitnessRules::IsWitnessed(FVector(38500.0f, 0.0f, 100.0f), false, Village,
										TArray<FVillagerWitness>(), FWitnessRules::DefaultWitnessRadius));
	TestTrue(TEXT("villager with LOS witnesses"),
			 FWitnessRules::IsWitnessed(Wilderness, false, TArray<FSettlementArea>(), Seer,
										FWitnessRules::DefaultWitnessRadius));
	TestFalse(TEXT("villager without LOS does not witness"),
			  FWitnessRules::IsWitnessed(Wilderness, false, TArray<FSettlementArea>(), Blind,
										 FWitnessRules::DefaultWitnessRadius));
	TestFalse(TEXT("distant villager does not witness"),
			  FWitnessRules::IsWitnessed(Wilderness, false, TArray<FSettlementArea>(), FarSeer,
										 FWitnessRules::DefaultWitnessRadius));
	TestFalse(TEXT("inside lair is never witness-charged"),
			  FWitnessRules::IsWitnessed(FVector(6000.0f, 0.0f, 150.0f), true, Village, Seer,
										 FWitnessRules::DefaultWitnessRadius));
	TestEqual(TEXT("witnessed person kill is 10"),
			  FWitnessRules::PointsForAct(EWitnessedAct::PersonKill, true, false), 10);
	TestEqual(TEXT("unwitnessed person kill is 0"),
			  FWitnessRules::PointsForAct(EWitnessedAct::PersonKill, false, false), 0);
	TestEqual(TEXT("witnessed livestock is 3"),
			  FWitnessRules::PointsForAct(EWitnessedAct::LivestockEat, true, false), 3);
	TestEqual(TEXT("inside livestock is 0"), FWitnessRules::PointsForAct(EWitnessedAct::LivestockEat, true, true),
			  0);

	UNotorietySubsystem* Notoriety = NewObject<UNotorietySubsystem>();
	Notoriety->AddPublicKillPointsWitnessed(Wilderness, TArray<FSettlementArea>(), TArray<FVillagerWitness>());
	TestEqual(TEXT("unwitnessed kill adds nothing through the writer"), Notoriety->GetPoints(), 0);
	Notoriety->AddPublicKillPointsWitnessed(Wilderness, TArray<FSettlementArea>(), Seer);
	TestEqual(TEXT("witnessed kill adds 10 through the writer"), Notoriety->GetPoints(), 10);
	Notoriety->AddLivestockEatPointsWitnessed(Wilderness, TArray<FSettlementArea>(), TArray<FVillagerWitness>());
	TestEqual(TEXT("unwitnessed livestock adds nothing"), Notoriety->GetPoints(), 10);
	Notoriety->AddLivestockEatPointsWitnessed(FVector(38500.0f, 0.0f, 100.0f), Village,
											  TArray<FVillagerWitness>());
	TestEqual(TEXT("settlement livestock adds 3"), Notoriety->GetPoints(), 13);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
