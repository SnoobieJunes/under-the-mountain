// Copyright Under the Mountain contributors.
// Purpose: T-018 intro rule tests - branch points, the timed
// state walk, the choice timeout (anti-soft-lock), and escape-gain
// suppression. The subsystem is driven with fake time; the world
// work (gnome, widget, camera) is the pawn's, proven in-game.

#include "Misc/AutomationTest.h"
#include "Intro/IntroSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-01 T-018] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroSequenceTest, "UnderTheMountain.Intro.Sequence",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIntroSequenceTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-018] Spec branch points, read from the
	// subsystem's statics (AT-IntroBranches' accounting core).
	TestEqual(TEXT("let go costs +5"), UIntroSubsystem::LetGoPoints(), 5);
	TestEqual(TEXT("failed chase costs +5"), UIntroSubsystem::FailedChasePoints(), 5);

	UIntroSubsystem* Intro = NewObject<UIntroSubsystem>();
	TestTrue(TEXT("starts asleep"), Intro->GetState() == EIntroState::Sleeping);
	TestEqual(TEXT("wake progress starts at 0"), Intro->GetWakeProgress(), 0.0f);
	Intro->TickIntro(2.1f);
	TestTrue(TEXT("sleep ends into wake"), Intro->GetState() == EIntroState::WakeUp);
	Intro->TickIntro(1.25f);
	TestTrue(TEXT("wake progress is half-way"),
			 FMath::IsNearlyEqual(Intro->GetWakeProgress(), 0.5f, 0.01f));
	Intro->TickIntro(1.3f);
	TestTrue(TEXT("wake ends into the steal"), Intro->GetState() == EIntroState::GnomeSteals);
	TestEqual(TEXT("wake progress completes"), Intro->GetWakeProgress(), 1.0f);

	// The steal report only counts in its own state.
	Intro->ReportGnomeStole();
	TestTrue(TEXT("steal reported -> choice"), Intro->GetState() == EIntroState::Choice);

	// Anti-soft-lock: 20 s unanswered resolves as Let Go.
	Intro->TickIntro(20.1f);
	TestTrue(TEXT("choice timeout reaches free play"), Intro->GetState() == EIntroState::FreePlay);
	TestTrue(TEXT("timeout resolves as let go"), Intro->GetBranch() == EIntroBranch::LetGo);

	// Suppression: the gnome's escape is intro-charged, others not.
	Intro->SetGnomeName(FName(TEXT("Gnome")));
	int32 Replacement = -1;
	TestTrue(TEXT("gnome escape suppressed after let go"),
			 Intro->ShouldSuppressEscapeGain(FName(TEXT("Gnome")), Replacement));
	TestEqual(TEXT("let go escape has no replacement charge"), Replacement, 0);
	TestFalse(TEXT("other thieves are not suppressed"),
			  Intro->ShouldSuppressEscapeGain(FName(TEXT("SomeWaveThief")), Replacement));

	// Skip path: straight to free play.
	UIntroSubsystem* Skipped = NewObject<UIntroSubsystem>();
	Skipped->SkipToFreePlay();
	TestTrue(TEXT("skip lands in free play"), Skipped->GetState() == EIntroState::FreePlay);

	// Choose only counts in Choice state.
	UIntroSubsystem* Early = NewObject<UIntroSubsystem>();
	Early->Choose(EIntroBranch::Chase);
	TestTrue(TEXT("early choose is ignored"), Early->GetState() == EIntroState::Sleeping);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
