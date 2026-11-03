// Copyright Under the Mountain contributors.
// Purpose: T-104 wave-resolution contract — a missing thief (null slot
// where a spawn was recorded, or a live thief with no brain) resolves
// LOUDLY: an Error naming the wave and the slot/actor, once per slot,
// while the deliberate empty-wave mercy stays silent. All headless,
// no world spawned: brains are NewObject + Init, thieves NewObject
// with the SetBrainForTesting seam, following ThiefBrainTest's pattern.
// Created 2026-11-03 (T-104).

#include "Misc/AutomationTest.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"
#include "Waves/WaveSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
// [2026-11-03 T-104] One treasure is enough: the brain only needs a
// non-null registry at Init (a null registry logs a warning and the
// test stays clean without one).
UTreasureRegistry* MakeWaveRegistry()
{
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1200;
	Crown.Location = FVector(2000.0f, 0.0f, 0.0f);
	Registry->Register(Crown);
	return Registry;
}

// [2026-11-03 T-104] Fresh brains start Sneaking (unresolved); lethal
// damage drives Dead (resolved) — the same two states ThiefBrainTest
// proves, reused here as resolution inputs.
UThiefBrain* MakeWaveBrain(UTreasureRegistry* Registry, bool bDead)
{
	UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
	Config->MaxHP = 20.0f;
	UThiefBrain* Brain = NewObject<UThiefBrain>();
	Brain->Init(Config, Registry, FName(TEXT("WaveThief")));
	if (bDead)
	{
		Brain->ApplyDamage(999.0f, FVector::ZeroVector);
	}
	return Brain;
}

AThiefCharacter* MakeWaveThief(UThiefBrain* BrainOrNull)
{
	AThiefCharacter* Thief = NewObject<AThiefCharacter>();
	if (BrainOrNull != nullptr)
	{
		Thief->SetBrainForTesting(BrainOrNull);
	}
	return Thief;
}
} // namespace

// [2026-11-03 T-104] Null slots resolve loudly, live states resolve by
// the book, and the empty-wave mercy survives untouched.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveMissingThiefTest, "UnderTheMountain.Wave.MissingThief",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveMissingThiefTest::RunTest(const FString& Parameters)
{
	// --- Mercy: an empty wave resolves with no report ---
	{
		UWaveSubsystem* Waves = NewObject<UWaveSubsystem>();
		TestTrue(TEXT("empty wave resolves"), Waves->TestAreWaveThievesResolved());
	}

	// --- Null slot: the spawn was recorded, the actor is gone ---
	{
		UWaveSubsystem* Waves = NewObject<UWaveSubsystem>();
		Waves->TestRecordWaveThief(nullptr);
		AddExpectedError(TEXT("thief slot 0 missing"), EAutomationExpectedErrorFlags::Contains, 1);
		TestTrue(TEXT("missing slot resolves"), Waves->TestAreWaveThievesResolved());
		TestTrue(TEXT("missing slot stays resolved without a second report"),
				 Waves->TestAreWaveThievesResolved());
	}

	// --- Live thief, no brain: the actor is there, the driver is not ---
	{
		UWaveSubsystem* Waves = NewObject<UWaveSubsystem>();
		Waves->TestRecordWaveThief(MakeWaveThief(nullptr));
		AddExpectedError(TEXT("has no brain"), EAutomationExpectedErrorFlags::Contains, 1);
		TestTrue(TEXT("brainless thief resolves"), Waves->TestAreWaveThievesResolved());
	}

	// --- All resolved: dead brains end the wave with no report ---
	{
		UTreasureRegistry* Registry = MakeWaveRegistry();
		UWaveSubsystem* Waves = NewObject<UWaveSubsystem>();
		Waves->TestRecordWaveThief(MakeWaveThief(MakeWaveBrain(Registry, true)));
		Waves->TestRecordWaveThief(MakeWaveThief(MakeWaveBrain(Registry, true)));
		TestTrue(TEXT("all-dead wave resolves"), Waves->TestAreWaveThievesResolved());
	}

	// --- Still active: a Sneaking brain holds the wave open ---
	{
		UTreasureRegistry* Registry = MakeWaveRegistry();
		UWaveSubsystem* Waves = NewObject<UWaveSubsystem>();
		Waves->TestRecordWaveThief(MakeWaveThief(MakeWaveBrain(Registry, true)));
		Waves->TestRecordWaveThief(MakeWaveThief(MakeWaveBrain(Registry, false)));
		TestFalse(TEXT("live thief holds the wave"), Waves->TestAreWaveThievesResolved());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
