// Copyright Under the Mountain contributors.
// Purpose: T-013 treasure + scent contract tests — the registry's
// register/total/carried/unregister behaviour, the docs/GAMEPLAY.md
// section 3 width brackets, and the no-information rule (equal-value
// treasures look identical at any distance) — all exercised against
// the pure registry/math with no world spawned.
// [2026-10-28 T-038] Two tests added below pin the failure-honesty
// and location-freshness seams: an uninitialised subsystem must fail
// explicitly, and a moved ATreasureActor must refresh its registry
// location instead of leaving it stale at registration time.

#include "Misc/AutomationTest.h"
#include "Scent/ScentMath.h"
#include "Scent/ScentRibbonActor.h"
#include "Treasure/TreasureActor.h"
#include "Treasure/TreasureDataAsset.h"
#include "Treasure/TreasureRegistry.h"
#include "Treasure/TreasureSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-28 T-013] Registry is a plain UObject by design (see its
// header), so the whole lifecycle runs headless here.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreasureRegistryTest, "UnderTheMountain.Treasure.Registry",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTreasureRegistryTest::RunTest(const FString& Parameters)
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	TestEqual(TEXT("new registry is empty"), Registry->Count(), 0);

	FTreasureEntry Trinket;
	Trinket.Id = FName(TEXT("Trinket"));
	Trinket.GoldValue = 10;
	Trinket.Location = FVector(100.0f, 0.0f, 0.0f);

	FTreasureEntry Goblet;
	Goblet.Id = FName(TEXT("Goblet"));
	Goblet.GoldValue = 100;
	Goblet.Location = FVector(0.0f, 200.0f, 0.0f);

	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1000;
	Crown.bIsSeeingStone = false;

	TestEqual(TEXT("register returns caller id"), Registry->Register(Trinket), Trinket.Id);
	Registry->Register(Goblet);
	Registry->Register(Crown);
	TestEqual(TEXT("three entries registered"), Registry->Count(), 3);
	TestEqual(TEXT("total gold sums every entry"), Registry->TotalGold(), int64(1110));

	FTreasureEntry Anonymous;
	Anonymous.GoldValue = 5;
	const FName GeneratedId = Registry->Register(Anonymous);
	TestFalse(TEXT("anonymous entry gets a generated id"), GeneratedId == NAME_None);
	TestEqual(TEXT("generated entry counted"), Registry->Count(), 4);
	TestTrue(TEXT("generated entry unregisters"), Registry->Unregister(GeneratedId));
	TestEqual(TEXT("back to three entries"), Registry->Count(), 3);

	TestTrue(TEXT("set carried on known entry"),
			 Registry->SetCarried(Goblet.Id, true, FName(TEXT("Thief1"))));
	const FTreasureEntry* FoundGoblet = Registry->Find(Goblet.Id);
	TestNotNull(TEXT("goblet still findable"), FoundGoblet);
	if (FoundGoblet != nullptr)
	{
		TestTrue(TEXT("goblet marked carried"), FoundGoblet->bCarried);
		TestEqual(TEXT("carrier recorded"), FoundGoblet->CarrierId, FName(TEXT("Thief1")));
	}

	TestTrue(TEXT("drop clears carried"), Registry->SetCarried(Goblet.Id, false, NAME_None));
	FoundGoblet = Registry->Find(Goblet.Id);
	TestNotNull(TEXT("goblet findable after drop"), FoundGoblet);
	if (FoundGoblet != nullptr)
	{
		TestFalse(TEXT("goblet no longer carried"), FoundGoblet->bCarried);
		TestEqual(TEXT("carrier cleared on drop"), FoundGoblet->CarrierId, NAME_None);
	}

	TestTrue(TEXT("location updates on known entry"),
			 Registry->UpdateLocation(Trinket.Id, FVector(7.0f, 8.0f, 9.0f)));
	const FTreasureEntry* FoundTrinket = Registry->Find(Trinket.Id);
	TestNotNull(TEXT("trinket findable"), FoundTrinket);
	if (FoundTrinket != nullptr)
	{
		TestEqual(TEXT("location stored"), FoundTrinket->Location, FVector(7.0f, 8.0f, 9.0f));
	}

	const FName Missing = FName(TEXT("NoSuchTreasure"));
	TestNull(TEXT("find of unknown id is null"), Registry->Find(Missing));
	TestFalse(TEXT("set carried refuses unknown id"), Registry->SetCarried(Missing, true, NAME_None));
	TestFalse(TEXT("update location refuses unknown id"),
			  Registry->UpdateLocation(Missing, FVector::ZeroVector));
	TestFalse(TEXT("unregister refuses unknown id"), Registry->Unregister(Missing));

	TestTrue(TEXT("unregister known entry"), Registry->Unregister(Crown.Id));
	TestEqual(TEXT("count drops after unregister"), Registry->Count(), 2);
	TestEqual(TEXT("total drops after unregister"), Registry->TotalGold(), int64(110));
	return true;
}

// [2026-10-28 T-013] Width brackets from docs/GAMEPLAY.md section 3,
// checked at bracket interiors and edges. Strict monotonicity across
// brackets is what lets a player rank treasures by ribbon alone.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScentWidthTest, "UnderTheMountain.Scent.Width",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScentWidthTest::RunTest(const FString& Parameters)
{
	const float Width0 = FScentMath::RibbonWidthForValue(0);
	const float Width10 = FScentMath::RibbonWidthForValue(10);
	const float Width24 = FScentMath::RibbonWidthForValue(24);
	const float Width25 = FScentMath::RibbonWidthForValue(25);
	const float Width50 = FScentMath::RibbonWidthForValue(50);
	const float Width200 = FScentMath::RibbonWidthForValue(200);
	const float Width500 = FScentMath::RibbonWidthForValue(500);
	const float Width999 = FScentMath::RibbonWidthForValue(999);
	const float Width1000 = FScentMath::RibbonWidthForValue(1000);
	const float Width2000 = FScentMath::RibbonWidthForValue(2000);

	TestTrue(TEXT("value 0 uses trinket base width"), Width0 >= 3.5f && Width0 <= 4.5f);
	TestTrue(TEXT("trinket interior near 4 cm"), Width10 >= 3.5f && Width10 <= 4.5f);
	TestTrue(TEXT("next bracket starts wider than trinket ends"), Width25 > Width24);
	TestTrue(TEXT("25-99 bracket wider than trinket"), Width50 > Width10);
	TestTrue(TEXT("100-399 bracket wider than 25-99"), Width200 > Width50);
	TestTrue(TEXT("400-999 bracket wider than 100-399"), Width500 > Width200);
	TestTrue(TEXT("crown bracket starts wider than 400-999 ends"), Width1000 > Width999);
	TestTrue(TEXT("crown interior wider than 400-999"), Width2000 > Width500);
	TestTrue(TEXT("width grows within crown bracket up to its cap"), Width2000 > Width1000);
	return true;
}

// [2026-10-28 T-013] The no-information rule: ribbon appearance must
// not encode distance. Two equal-value entries at wildly different
// locations must produce identical params; opacity and the carried
// pulse are the only other sanctioned variations, pinned here too.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScentNoDistanceTest, "UnderTheMountain.Scent.NoDistanceAndTrueSight",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScentNoDistanceTest::RunTest(const FString& Parameters)
{
	FTreasureEntry Near;
	Near.Id = FName(TEXT("Near"));
	Near.GoldValue = 500;
	Near.Location = FVector::ZeroVector;

	FTreasureEntry Far;
	Far.Id = FName(TEXT("Far"));
	Far.GoldValue = 500;
	Far.Location = FVector(100000.0f, 0.0f, 0.0f);

	const FScentRibbonParams NearParams = FScentMath::ParamsForEntry(Near, 1.25f, false);
	const FScentRibbonParams FarParams = FScentMath::ParamsForEntry(Far, 1.25f, false);
	TestEqual(TEXT("width ignores distance"), NearParams.Width, FarParams.Width);
	TestEqual(TEXT("opacity ignores distance"), NearParams.Opacity, FarParams.Opacity);
	TestTrue(TEXT("color ignores distance"), NearParams.Color == FarParams.Color);
	TestTrue(TEXT("carried flag carried through"), NearParams.bCarried == FarParams.bCarried);

	TestEqual(TEXT("true-sight dims ribbons to 0.3"), FScentMath::RibbonOpacity(true), 0.3f);
	TestEqual(TEXT("normal smelling is fully opaque"), FScentMath::RibbonOpacity(false), 1.0f);
	TestEqual(TEXT("params reflect true-sight opacity"),
			  FScentMath::ParamsForEntry(Near, 0.0f, true).Opacity, 0.3f);

	const float BaseWidth = 20.0f;
	const float CarriedAt0 = FScentMath::PulsedWidth(BaseWidth, 0.0f, true);
	const float CarriedAtHalf = FScentMath::PulsedWidth(BaseWidth, 0.5f, true);
	TestFalse(TEXT("carried width pulses over time"),
			  FMath::IsNearlyEqual(CarriedAt0, CarriedAtHalf));
	TestEqual(TEXT("resting width never pulses"), FScentMath::PulsedWidth(BaseWidth, 0.5f, false),
			  BaseWidth);
	TestEqual(TEXT("resting width static at other times too"),
			  FScentMath::PulsedWidth(BaseWidth, 3.75f, false), BaseWidth);
	return true;
}

// [2026-10-28 T-031] The Niagara actor must carry exactly the width
// FScentMath computed — this is the seam where a value-scaled ribbon
// could silently become a fixed-width one.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScentRibbonWidthPassThroughTest,
								 "UnderTheMountain.Scent.RibbonWidthPassThrough",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScentRibbonWidthPassThroughTest::RunTest(const FString& Parameters)
{
	AScentRibbonActor* Ribbon = NewObject<AScentRibbonActor>();
	const float CrownWidth = FScentMath::RibbonWidthForValue(1200);
	Ribbon->SetEndpoints(FVector::ZeroVector, FVector(5000.0f, 0.0f, 0.0f), CrownWidth);
	TestEqual(TEXT("ribbon keeps the FScentMath crown width"), Ribbon->CurrentWidth(), CrownWidth);

	const float TrinketWidth = FScentMath::RibbonWidthForValue(10);
	Ribbon->SetEndpoints(FVector::ZeroVector, FVector(100.0f, 0.0f, 0.0f), TrinketWidth);
	TestEqual(TEXT("ribbon keeps the FScentMath trinket width"), Ribbon->CurrentWidth(),
			  TrinketWidth);
	TestTrue(TEXT("crown ribbon is wider than trinket ribbon"), CrownWidth > TrinketWidth);
	Ribbon->SetOpacityFactor(0.3f);
	TestEqual(TEXT("ribbon keeps the true-sight opacity factor"), Ribbon->VisibleOpacity, 0.3f);
	return true;
}

// [2026-10-28 T-038] Failure honesty at the subsystem seam: a
// UTreasureSubsystem that was never Initialize()d owns no registry,
// so every forwarding verb must fail explicitly (NAME_None/false)
// rather than crash or pretend success — ATreasureActor::BeginPlay's
// "Treasure registration FAILED" warning depends on exactly this.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreasureSubsystemFailureTest,
								 "UnderTheMountain.Treasure.SubsystemFailure",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTreasureSubsystemFailureTest::RunTest(const FString& Parameters)
{
	UTreasureSubsystem* Subsystem = NewObject<UTreasureSubsystem>();
	TestNull(TEXT("uninitialised subsystem has no registry"), Subsystem->Registry());

	FTreasureEntry Entry;
	Entry.GoldValue = 100;
	TestEqual(TEXT("register on uninitialised subsystem returns NAME_None"),
			  Subsystem->Register(Entry), NAME_None);
	TestFalse(TEXT("unregister on uninitialised subsystem returns false"),
			  Subsystem->Unregister(FName(TEXT("Anything"))));
	return true;
}

// [2026-10-28 T-038] Location freshness: an actor that registers at
// one location and then moves (carried/repositioned) must be able to
// push its new location into the registry — this is the seam
// ATreasureActor::Tick calls every frame, so the scent endpoint
// follows the treasure instead of its spawn point. SetActorLocation
// on this NewObject (unspawned) actor moves its root component
// transform directly, which GetActorLocation then reports.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTreasureActorLocationRefreshTest,
								 "UnderTheMountain.Treasure.ActorLocationRefresh",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTreasureActorLocationRefreshTest::RunTest(const FString& Parameters)
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	ATreasureActor* Actor = NewObject<ATreasureActor>();

	UTreasureDataAsset* Data = NewObject<UTreasureDataAsset>();
	Data->GoldValue = 100;
	Data->DisplayName = TEXT("Refresh Test Goblet");
	Actor->TreasureData = Data;

	Actor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	const FName Id = Actor->RegisterWithRegistry(Registry);
	TestFalse(TEXT("register with registry returns a real id"), Id == NAME_None);
	const FTreasureEntry* Found = Registry->Find(Id);
	TestNotNull(TEXT("registered entry is findable"), Found);
	if (Found != nullptr)
	{
		TestEqual(TEXT("registered at the spawn location"), Found->Location,
				  FVector(100.0f, 0.0f, 0.0f));
	}

	Actor->SetActorLocation(FVector(5000.0f, 200.0f, 0.0f));
	TestTrue(TEXT("refresh after a move reports current"),
			 Actor->RefreshRegisteredLocation(Registry));
	Found = Registry->Find(Id);
	TestNotNull(TEXT("entry still findable after refresh"), Found);
	if (Found != nullptr)
	{
		TestEqual(TEXT("registry location follows the actor"), Found->Location,
				  FVector(5000.0f, 200.0f, 0.0f));
	}

	TestTrue(TEXT("refresh with no move still reports current"),
			 Actor->RefreshRegisteredLocation(Registry));
	TestFalse(TEXT("refresh against a null registry fails explicitly"),
			  Actor->RefreshRegisteredLocation(nullptr));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
