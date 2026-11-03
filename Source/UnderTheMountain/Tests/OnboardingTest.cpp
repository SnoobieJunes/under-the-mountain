// Copyright Under the Mountain contributors.
// Purpose: T-073 onboarding contract — the prompt arbiter's two-lane
// logic (combat prompts preempt on context, basics run in teaching
// sequence, nothing deadlocks) plus the help table's completeness:
// every action bound in the shipped mapping must have a row, so the
// overlay can never silently drop a binding. Pure logic + mapping
// reflection, headless, no world spawned.
// Created 2026-11-03 (T-073).

#include "Misc/AutomationTest.h"
#include "Dragon/DragonCharacter.h"
#include "InputMappingContext.h"
#include "UI/OnboardingContent.h"
#include "UnderTheMountain.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-03 T-073] The arbiter contract: combat preempts on context,
// basics advance in order, completion shows nothing. Each case pins
// one row of the decision table.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnboardingArbiterTest, "UnderTheMountain.Onboarding.Arbiter",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOnboardingArbiterTest::RunTest(const FString& Parameters)
{
	const int32 N = static_cast<int32>(EOnboardingVerb::Count);
	auto Fresh = [N]()
	{
		TArray<bool> Done;
		Done.Init(false, N);
		return Done;
	};
	// Untouched run: flap teaches first.
	{
		TArray<bool> Done = Fresh();
		TestEqual(TEXT("fresh run prompts flap"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), false, false, false)),
				  static_cast<int32>(EOnboardingVerb::Flap));
	}
	// Basics advance in order once performed.
	{
		TArray<bool> Done = Fresh();
		Done[static_cast<int32>(EOnboardingVerb::Flap)] = true;
		TestEqual(TEXT("dive follows flap"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), false, false, false)),
				  static_cast<int32>(EOnboardingVerb::Dive));
		Done[static_cast<int32>(EOnboardingVerb::Dive)] = true;
		TestEqual(TEXT("scent follows dive"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), false, false, false)),
				  static_cast<int32>(EOnboardingVerb::Scent));
	}
	// Combat preempts on context, even with basics pending.
	{
		TArray<bool> Done = Fresh();
		TestEqual(TEXT("fire preempts on a burnable in range"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), true, false, false)),
				  static_cast<int32>(EOnboardingVerb::Fire));
		Done[static_cast<int32>(EOnboardingVerb::Fire)] = true;
		TestEqual(TEXT("roar preempts on a thief in range"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), false, true, false)),
				  static_cast<int32>(EOnboardingVerb::Roar));
		Done[static_cast<int32>(EOnboardingVerb::Roar)] = true;
		TestEqual(TEXT("eat preempts on a goat in range"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), false, false, true)),
				  static_cast<int32>(EOnboardingVerb::Eat));
	}
	// Context without need stays silent: an unperformed combat verb
	// with no context shows nothing (it must not block, and there is
	// nothing else left to teach).
	{
		TArray<bool> Done = Fresh();
		for (int32 I = 0; I < N; ++I)
		{
			Done[I] = true;
		}
		Done[static_cast<int32>(EOnboardingVerb::Fire)] = false;
		TestEqual(TEXT("unperformed fire without a target shows nothing"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), false, false, false)),
				  static_cast<int32>(EOnboardingVerb::Count));
	}
	// Everything done: show nothing.
	{
		TArray<bool> Done = Fresh();
		for (int32 I = 0; I < N; ++I)
		{
			Done[I] = true;
		}
		TestEqual(TEXT("completed onboarding shows nothing"),
				  static_cast<int32>(NextOnboardingPrompt(Done.GetData(), true, true, true)),
				  static_cast<int32>(EOnboardingVerb::Count));
	}
	return true;
}

// [2026-11-03 T-073] The overlay IS the shipped scheme: every action
// bound in the mapping context must have a help-table row, or the
// overlay would silently drop a binding the game actually uses.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnboardingHelpCoverageTest,
								 "UnderTheMountain.Onboarding.HelpCoverage",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOnboardingHelpCoverageTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for help coverage check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	Pawn->EnsureInputActions();

	UClass* PawnClass = ADragonCharacter::StaticClass();
	const FObjectProperty* ContextProp =
		FindFProperty<FObjectProperty>(PawnClass, TEXT("MappingContext"));
	TestNotNull(TEXT("MappingContext member reflects"), ContextProp);
	if (ContextProp == nullptr)
	{
		return false;
	}
	const UInputMappingContext* MappingContext = Cast<UInputMappingContext>(
		ContextProp->GetObjectPropertyValue_InContainer(Pawn));
	TestNotNull(TEXT("ensured pawn owns the mapping context"), MappingContext);
	if (MappingContext == nullptr)
	{
		return false;
	}

	TSet<FString> TableActions;
	for (const FOnboardingBindingDef& Row : GetOnboardingBindings())
	{
		TableActions.Add(Row.ActionName);
	}
	TestTrue(TEXT("help table is not empty"), TableActions.Num() > 0);

	bool bAllCovered = true;
	for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
	{
		if (Mapping.Action == nullptr)
		{
			continue;
		}
		const FString ActionName = Mapping.Action->GetFName().ToString();
		if (!TableActions.Contains(ActionName))
		{
			UE_LOG(LogUnderTheMountain, Error,
				   TEXT("Help overlay missing row for bound action %s"), *ActionName);
			bAllCovered = false;
		}
	}
	TestTrue(TEXT("every bound action has a help row"), bAllCovered);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
