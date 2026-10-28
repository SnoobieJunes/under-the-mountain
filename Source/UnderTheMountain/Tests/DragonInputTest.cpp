// Copyright Under the Mountain contributors.
// Purpose: T-033 input-construction contract test — a fresh dragon
// pawn builds its Enhanced Input actions lazily via
// EnsureInputActions (never in the constructor: CDO creation asserts
// on UObject creation), and afterwards every action + the mapping
// context exist so SetupPlayerInputComponent cannot bind a null.
// Created 2026-10-28.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-28 T-033] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonInputTest, "UnderTheMountain.Dragon.Input",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonInputTest::RunTest(const FString& Parameters)
{
	// [2026-10-28 T-033] A fresh pawn must NOT have actions yet —
	// they cannot be built in the constructor (CDO creation asserts
	// on UObject creation, proven at module load). The contract is
	// that EnsureInputActions builds them before any binding, which
	// SetupPlayerInputComponent and BeginPlay both call first.
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("fresh dragon pawn constructs"), Pawn);
	if (Pawn != nullptr)
	{
		TestFalse(TEXT("fresh pawn has no actions before ensure"), Pawn->HasInputActions());
		Pawn->EnsureInputActions();
		TestTrue(TEXT("ensured pawn has all input actions and mapping context"),
				 Pawn->HasInputActions());
		Pawn->EnsureInputActions();
		TestTrue(TEXT("ensure is idempotent"), Pawn->HasInputActions());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
