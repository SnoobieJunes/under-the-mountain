// Copyright Under the Mountain contributors.
// Purpose: T-033 input-construction contract test — a fresh dragon
// pawn builds its Enhanced Input actions lazily via
// EnsureInputActions (never in the constructor: CDO creation asserts
// on UObject creation), and afterwards every action + the mapping
// context exist so SetupPlayerInputComponent cannot bind a null.
// Created 2026-10-28.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonCharacter.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

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
	// [2026-11-03 T-075] Prototype actions join the GC contract with the rest:
	// an unreflected action is collectable mid-flight (same T-004 lesson).
	for (const TCHAR* Member :
		 {TEXT("MoveAction"), TEXT("LookAction"), TEXT("FlapAction"), TEXT("ToggleFlightAction"),
		  TEXT("DiveAction"), TEXT("TrueSightAction"), TEXT("TintAction"), TEXT("TintWidget"),
		  TEXT("FreeLookAction"), TEXT("SinkAction"), TEXT("SteerLatchAction"),
		  TEXT("ScentToggleAction"), // [2026-11-03 T-061] N-key scent toggle joins the GC contract.
		  TEXT("MappingContext")})
	{
		TestNotNull(FString::Printf(TEXT("%s is a reflected UPROPERTY"), Member),
					PawnClass->FindPropertyByName(FName(Member)));
	}
	return true;
}

// [2026-11-03 T-061] The N / D-pad-Down wiring, proven headless: the
// private action and mapping-context members are read through
// reflection (the T-035 precedent — no test-only accessors), and the
// mapping table must bind IA_ScentToggle to exactly the decided keys.
// This plus Scent.ToggleVisibility proves everything short of Slate
// delivering a live keypress, which rides with the next PIE session.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonScentToggleBindingTest,
								 "UnderTheMountain.Dragon.ScentToggleBinding",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonScentToggleBindingTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for binding check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->EnsureInputActions();

	UClass* PawnClass = ADragonCharacter::StaticClass();
	const FObjectProperty* ActionProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("ScentToggleAction"));
	const FObjectProperty* ContextProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("MappingContext"));
	TestNotNull(TEXT("ScentToggleAction member reflects"), ActionProp);
	TestNotNull(TEXT("MappingContext member reflects"), ContextProp);
	if (ActionProp == nullptr || ContextProp == nullptr)
	{
		return false;
	}

	const UInputAction* ScentAction =
		Cast<UInputAction>(ActionProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputMappingContext* Context =
		Cast<UInputMappingContext>(ContextProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns the scent-toggle action"), ScentAction);
	TestNotNull(TEXT("ensured pawn owns the mapping context"), Context);
	if (ScentAction == nullptr || Context == nullptr)
	{
		return false;
	}

	bool bHasN = false;
	bool bHasPadDown = false;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.Action == ScentAction)
		{
			bHasN = bHasN || Mapping.Key == EKeys::N;
			bHasPadDown = bHasPadDown || Mapping.Key == EKeys::Gamepad_DPad_Down;
		}
	}
	TestTrue(TEXT("N key drives the scent toggle"), bHasN);
	TestTrue(TEXT("gamepad D-pad Down drives the scent toggle"), bHasPadDown);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
