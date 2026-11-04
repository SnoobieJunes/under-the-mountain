// Copyright Under the Mountain contributors.
// Purpose: T-033 input-construction contract test — a fresh dragon
// pawn builds its Enhanced Input actions lazily via
// EnsureInputActions (never in the constructor: CDO creation asserts
// on UObject creation), and afterwards every action + the mapping
// context exist so SetupPlayerInputComponent cannot bind a null.
// Created 2026-10-28.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonCameraSaveGame.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

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
		  TEXT("ScentToggleAction"),
		  TEXT("PadLookAction"), TEXT("PadMoveAction"), TEXT("CameraCycleAction"),
		  TEXT("FireAction"), TEXT("RoarAction"), TEXT("EatAction"), TEXT("WingBuffetAction"),
		  // [2026-11-03 T-108] Combat verbs join the GC contract: an
		  // unreflected action is collectable mid-fight (same T-004 lesson).
		  TEXT("TailSlamAction"),
		  // [2026-11-03 T-072] Flow pause joins the GC contract (same lesson).
		  TEXT("PauseAction"),
		  // [2026-11-03 T-073] Help action + onboarding widget join the GC
		  // contract (same lesson).
		  TEXT("HelpAction"),
		  TEXT("OnboardingWidget"),
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

// [2026-11-03 T-063] The RMB free-look / D-pad-Up steer-latch wiring,
// proven headless: the mapping table must bind IA_FreeLook to the right
// mouse button and IA_SteerLatch to gamepad D-pad Up (the adopted T-063
// map). This plus the latch/cancel logic proves everything short of Slate
// delivering a live button press, which rides with the next PIE session.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonFreeLookBindingTest,
								 "UnderTheMountain.Dragon.FreeLookBinding",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonFreeLookBindingTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for binding check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->EnsureInputActions();

	UClass* PawnClass = ADragonCharacter::StaticClass();
	const FObjectProperty* FreeLookProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("FreeLookAction"));
	const FObjectProperty* LatchProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("SteerLatchAction"));
	const FObjectProperty* ContextProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("MappingContext"));
	TestNotNull(TEXT("FreeLookAction member reflects"), FreeLookProp);
	TestNotNull(TEXT("SteerLatchAction member reflects"), LatchProp);
	TestNotNull(TEXT("MappingContext member reflects"), ContextProp);
	if (FreeLookProp == nullptr || LatchProp == nullptr || ContextProp == nullptr)
	{
		return false;
	}

	const UInputAction* FreeLookAction =
		Cast<UInputAction>(FreeLookProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputAction* LatchAction =
		Cast<UInputAction>(LatchProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputMappingContext* Context =
		Cast<UInputMappingContext>(ContextProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns the free-look action"), FreeLookAction);
	TestNotNull(TEXT("ensured pawn owns the steer-latch action"), LatchAction);
	TestNotNull(TEXT("ensured pawn owns the mapping context"), Context);
	if (FreeLookAction == nullptr || LatchAction == nullptr || Context == nullptr)
	{
		return false;
	}

	bool bHasRMB = false;
	bool bHasPadUp = false;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.Action == FreeLookAction)
		{
			bHasRMB = bHasRMB || Mapping.Key == EKeys::RightMouseButton;
		}
		if (Mapping.Action == LatchAction)
		{
			bHasPadUp = bHasPadUp || Mapping.Key == EKeys::Gamepad_DPad_Up;
		}
	}
	TestTrue(TEXT("right mouse button drives free look"), bHasRMB);
	TestTrue(TEXT("gamepad D-pad Up drives the steer latch"), bHasPadUp);
	return true;
}

// [2026-11-03 T-065] The split stick map + camera cycle, proven headless:
// mouse stays on LookAction, the sticks move to their own actions, and V +
// R3 step the adopted set. Same reflection pattern as the scent binding
// test (the T-035 precedent — no test-only accessors).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCameraBindingTest,
								 "UnderTheMountain.Dragon.CameraBinding",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCameraBindingTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for binding check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->EnsureInputActions();

	UClass* PawnClass = ADragonCharacter::StaticClass();
	const FObjectProperty* LookProp = FindFProperty<FObjectProperty>(PawnClass, TEXT("LookAction"));
	const FObjectProperty* PadLookProp = FindFProperty<FObjectProperty>(PawnClass, TEXT("PadLookAction"));
	const FObjectProperty* MoveProp = FindFProperty<FObjectProperty>(PawnClass, TEXT("MoveAction"));
	const FObjectProperty* PadMoveProp = FindFProperty<FObjectProperty>(PawnClass, TEXT("PadMoveAction"));
	const FObjectProperty* CycleProp = FindFProperty<FObjectProperty>(PawnClass, TEXT("CameraCycleAction"));
	const FObjectProperty* ContextProp = FindFProperty<FObjectProperty>(PawnClass, TEXT("MappingContext"));
	TestNotNull(TEXT("LookAction member reflects"), LookProp);
	TestNotNull(TEXT("PadLookAction member reflects"), PadLookProp);
	TestNotNull(TEXT("MoveAction member reflects"), MoveProp);
	TestNotNull(TEXT("PadMoveAction member reflects"), PadMoveProp);
	TestNotNull(TEXT("CameraCycleAction member reflects"), CycleProp);
	TestNotNull(TEXT("MappingContext member reflects"), ContextProp);
	if (!LookProp || !PadLookProp || !MoveProp || !PadMoveProp || !CycleProp || !ContextProp)
	{
		return false;
	}

	const UInputAction* Look = Cast<UInputAction>(LookProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputAction* PadLook = Cast<UInputAction>(PadLookProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputAction* Move = Cast<UInputAction>(MoveProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputAction* PadMove = Cast<UInputAction>(PadMoveProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputAction* Cycle = Cast<UInputAction>(CycleProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputMappingContext* Context =
		Cast<UInputMappingContext>(ContextProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns every split action and the context"), Look);
	TestNotNull(TEXT("ensured pawn owns the pad look action"), PadLook);
	TestNotNull(TEXT("ensured pawn owns the pad move action"), PadMove);
	TestNotNull(TEXT("ensured pawn owns the camera cycle action"), Cycle);
	TestNotNull(TEXT("ensured pawn owns the mapping context"), Context);
	if (!Look || !PadLook || !Move || !PadMove || !Cycle || !Context)
	{
		return false;
	}

	bool bMouseLook = false;
	bool bPadOnLook = false;
	bool bPadLook = false;
	bool bWASD = false;
	bool bStickOnMove = false;
	bool bPadMove = false;
	bool bCycleV = false;
	bool bCycleR3 = false;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.Action == Look)
		{
			bMouseLook = bMouseLook || Mapping.Key == EKeys::Mouse2D;
			bPadOnLook = bPadOnLook || Mapping.Key == EKeys::Gamepad_Right2D;
		}
		if (Mapping.Action == PadLook)
		{
			bPadLook = bPadLook || Mapping.Key == EKeys::Gamepad_Right2D;
		}
		if (Mapping.Action == Move)
		{
			bWASD = bWASD || Mapping.Key == EKeys::W || Mapping.Key == EKeys::A;
			bStickOnMove = bStickOnMove || Mapping.Key == EKeys::Gamepad_Left2D;
		}
		if (Mapping.Action == PadMove)
		{
			bPadMove = bPadMove || Mapping.Key == EKeys::Gamepad_Left2D;
		}
		if (Mapping.Action == Cycle)
		{
			bCycleV = bCycleV || Mapping.Key == EKeys::V;
			bCycleR3 = bCycleR3 || Mapping.Key == EKeys::Gamepad_RightThumbstick;
		}
	}
	TestTrue(TEXT("mouse look stays on the mouse action"), bMouseLook);
	TestFalse(TEXT("right stick leaves the mouse action"), bPadOnLook);
	TestTrue(TEXT("right stick drives the pad-look action"), bPadLook);
	TestTrue(TEXT("WASD stays on the move action"), bWASD);
	TestFalse(TEXT("left stick leaves the move action"), bStickOnMove);
	TestTrue(TEXT("left stick drives the pad-move action"), bPadMove);
	TestTrue(TEXT("V cycles the camera"), bCycleV);
	TestTrue(TEXT("gamepad R3 cycles the camera"), bCycleR3);
	return true;
}

// [2026-11-04 T-111] Camera order + persistence, headless: Near (default)
// → Far → Eyes → Near, saved to the slot on every step, reloaded on a
// fresh pawn, missing/corrupt slot means Near. Uses a test slot like the
// tint test so the player's DragonCameraChoice.sav is never touched.
// [2026-11-04 T-111] Plus the pure rig decisions per mode (arm, mesh
// [2026-11-04 T-111] visibility, boom collision) via CameraRigForMode — the
// [2026-11-04 T-111] same values ApplyCameraRig runs, so the -game capture
// [2026-11-04 T-111] set proves the pivots, not these numbers twice.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonCameraCycleTest,
								 "UnderTheMountain.Dragon.CameraCycle",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonCameraCycleTest::RunTest(const FString& Parameters)
{
	const FString TestSlot = TEXT("DragonCameraChoiceTest");
	UGameplayStatics::DeleteGameInSlot(TestSlot, 0);

	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for camera check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->SetCameraSaveSlotForTesting(TestSlot);
	Pawn->LoadCameraChoice();
	TestEqual(TEXT("missing slot means near"), static_cast<int32>(Pawn->GetCameraMode()), 0);

	Pawn->CycleCamera();
	TestEqual(TEXT("first cycle steps to far"), static_cast<int32>(Pawn->GetCameraMode()), 1);
	Pawn->CycleCamera();
	TestEqual(TEXT("second cycle steps to eyes"), static_cast<int32>(Pawn->GetCameraMode()), 2);
	Pawn->CycleCamera();
	TestEqual(TEXT("third cycle wraps to near"), static_cast<int32>(Pawn->GetCameraMode()), 0);

	Pawn->CycleCamera();
	TestEqual(TEXT("far persists"), static_cast<int32>(Pawn->GetCameraMode()), 1);
	ADragonCharacter* Fresh = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("fresh pawn constructs for reload check"), Fresh);
	if (Fresh == nullptr)
	{
		return false;
	}
	Fresh->SetCameraSaveSlotForTesting(TestSlot);
	Fresh->LoadCameraChoice();
	TestEqual(TEXT("fresh pawn reloads far"), static_cast<int32>(Fresh->GetCameraMode()), 1);

	UDragonCameraSaveGame* Corrupt = Cast<UDragonCameraSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UDragonCameraSaveGame::StaticClass()));
	TestNotNull(TEXT("corrupt save object creates"), Corrupt);
	if (Corrupt == nullptr)
	{
		return false;
	}
	Corrupt->CameraModeIndex = 99;
	TestTrue(TEXT("corrupt slot writes"), UGameplayStatics::SaveGameToSlot(Corrupt, TestSlot, 0));
	Fresh->LoadCameraChoice();
	TestEqual(TEXT("corrupt index falls back to near"), static_cast<int32>(Fresh->GetCameraMode()), 0);

	// [2026-11-04 T-111] Rig decisions per mode: the exact arm / visibility /
	// [2026-11-04 T-111] collision contract ApplyCameraRig runs.
	for (const TTuple<EDragonCameraMode, float, bool, bool>& Rig :
		 {MakeTuple(EDragonCameraMode::Near, 750.0f, false, true),
		  MakeTuple(EDragonCameraMode::Far, 1200.0f, false, true),
		  MakeTuple(EDragonCameraMode::Eyes, 0.0f, true, false)})
	{
		float Arm = -1.0f;
		bool bHide = false;
		bool bCollide = false;
		ADragonCharacter::CameraRigForMode(Rig.Get<0>(), Arm, bHide, bCollide);
		TestEqual(
			FString::Printf(TEXT("mode %d arm"), static_cast<int32>(Rig.Get<0>())), Arm, Rig.Get<1>());
		TestEqual(FString::Printf(TEXT("mode %d hides mesh"), static_cast<int32>(Rig.Get<0>())), bHide,
				  Rig.Get<2>());
		TestEqual(FString::Printf(TEXT("mode %d collides"), static_cast<int32>(Rig.Get<0>())), bCollide,
				  Rig.Get<3>());
	}

	// [2026-11-04 T-111] Applied rig per mode on a fresh pawn (starts Near):
	// [2026-11-04 T-111] arm converges, mesh visibility flips only in Eyes,
	// [2026-11-04 T-111] boom collision off only in Eyes. Pivots need live
	// [2026-11-04 T-111] bones — proven by the -game capture set, not here.
	for (const TTuple<EDragonCameraMode, float, bool, bool>& Rig :
		 {MakeTuple(EDragonCameraMode::Near, 750.0f, false, true),
		  MakeTuple(EDragonCameraMode::Far, 1200.0f, false, true),
		  MakeTuple(EDragonCameraMode::Eyes, 0.0f, true, false)})
	{
		while (Pawn->GetCameraMode() != Rig.Get<0>())
		{
			Pawn->CycleCamera();
		}
		Pawn->ApplyCameraRigForTesting(100.0f);
		TestEqual(FString::Printf(TEXT("applied mode %d arm"), static_cast<int32>(Rig.Get<0>())),
				  Pawn->CameraBoom->TargetArmLength, Rig.Get<1>());
		TestEqual(FString::Printf(TEXT("applied mode %d mesh visible"), static_cast<int32>(Rig.Get<0>())),
				  Pawn->GetMesh()->IsVisible(), !Rig.Get<2>());
		TestEqual(FString::Printf(TEXT("applied mode %d boom collides"), static_cast<int32>(Rig.Get<0>())),
				  Pawn->CameraBoom->bDoCollisionTest, Rig.Get<3>());
	}

	// Pitch-invert defaults: nose-up on every device until T-088 surfaces
	// the Settings choice (adopted FLIGHT-SPIKE.md default).
	TestFalse(TEXT("mouse pitch defaults to non-inverted"), ADragonCharacter::InvertMousePitch());
	TestFalse(TEXT("pad flight pitch defaults to non-inverted"), ADragonCharacter::InvertPadFlightPitch());
	TestFalse(TEXT("pad camera pitch defaults to non-inverted"), ADragonCharacter::InvertPadCameraPitch());

	UGameplayStatics::DeleteGameInSlot(TestSlot, 0);
	return true;
}

// [2026-11-03 T-072] The flow-pause wiring, proven headless: the GC
// sees the pause action (T-108 contract keeps every HasInputActions
// member reflected) and the mapping table binds it to exactly Esc +
// gamepad menu (Start). Same pattern as the scent/camera binding
// tests; a live keypress rides with the next PIE session.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonPauseBindingTest,
								 "UnderTheMountain.Dragon.PauseBinding",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonPauseBindingTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for binding check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->EnsureInputActions();

	UClass* PawnClass = ADragonCharacter::StaticClass();
	const FObjectProperty* PauseProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("PauseAction"));
	const FObjectProperty* ContextProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("MappingContext"));
	TestNotNull(TEXT("PauseAction member reflects"), PauseProp);
	TestNotNull(TEXT("MappingContext member reflects"), ContextProp);
	if (PauseProp == nullptr || ContextProp == nullptr)
	{
		return false;
	}
	const UInputAction* Pause =
		Cast<UInputAction>(PauseProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns the pause action"), Pause);
	const UInputMappingContext* Context =
		Cast<UInputMappingContext>(ContextProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns the mapping context"), Context);
	if (Pause == nullptr || Context == nullptr)
	{
		return false;
	}

	TestTrue(TEXT("pause action joins the input contract"), Pawn->HasInputActions());

	bool bEsc = false;
	bool bPadMenu = false;
	if (Context != nullptr)
	{
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action == Pause)
			{
				bEsc = bEsc || Mapping.Key == EKeys::Escape;
				bPadMenu = bPadMenu || Mapping.Key == EKeys::Gamepad_Special_Right;
			}
		}
	}
	TestTrue(TEXT("Esc pauses the flow"), bEsc);
	TestTrue(TEXT("gamepad menu pauses the flow"), bPadMenu);
	return true;
}

// [2026-11-03 T-073] The help binding, proven headless like pause: the
// action reflects, the pawn owns it, and the mapping table binds it to
// exactly H + gamepad Back (Select). Same pattern rides the next PIE
// session for the live keypress.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonHelpBindingTest,
								 "UnderTheMountain.Dragon.HelpBinding",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonHelpBindingTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for binding check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->EnsureInputActions();

	UClass* PawnClass = ADragonCharacter::StaticClass();
	const FObjectProperty* HelpProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("HelpAction"));
	const FObjectProperty* ContextProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("MappingContext"));
	TestNotNull(TEXT("HelpAction member reflects"), HelpProp);
	TestNotNull(TEXT("MappingContext member reflects"), ContextProp);
	if (HelpProp == nullptr || ContextProp == nullptr)
	{
		return false;
	}
	const UInputAction* Help =
		Cast<UInputAction>(HelpProp->GetObjectPropertyValue_InContainer(Pawn));
	const UInputMappingContext* Context =
		Cast<UInputMappingContext>(ContextProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns the help action"), Help);
	TestNotNull(TEXT("ensured pawn owns the mapping context"), Context);
	if (Help == nullptr || Context == nullptr)
	{
		return false;
	}

	TestTrue(TEXT("help action joins the input contract"), Pawn->HasInputActions());

	bool bH = false;
	bool bPadBack = false;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.Action == Help)
		{
			bH = bH || Mapping.Key == EKeys::H;
			bPadBack = bPadBack || Mapping.Key == EKeys::Gamepad_Special_Left;
		}
	}
	TestTrue(TEXT("H toggles the help overlay"), bH);
	TestTrue(TEXT("gamepad Back toggles the help overlay"), bPadBack);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
