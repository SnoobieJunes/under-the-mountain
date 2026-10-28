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

	// [2026-10-28 T-035] The GC must be able to see the pawn's
	// UObject members: every action/context/widget member must be a
	// reflected UPROPERTY, or the GC can collect objects the pawn
	// still points at (T-004 sweep finding).
	UClass* PawnClass = ADragonCharacter::StaticClass();
	for (const TCHAR* Member :
		 {TEXT("MoveAction"), TEXT("LookAction"), TEXT("FlapAction"), TEXT("ToggleFlightAction"),
		  TEXT("DiveAction"), TEXT("TrueSightAction"), TEXT("TintAction"), TEXT("TintWidget"),
		  TEXT("MappingContext")})
	{
		TestNotNull(FString::Printf(TEXT("%s is a reflected UPROPERTY"), Member),
					PawnClass->FindPropertyByName(FName(Member)));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
