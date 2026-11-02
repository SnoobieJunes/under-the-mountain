// Copyright Under the Mountain contributors.
// Purpose: T-054 presentation-rule tests - the thief ragdoll
// decision and flinch curve, and the goat side-fall curve. Pure
// statics, no world: the in-world half (physics settle, mesh roll)
// is proven by the -game run named in T-054's closing note.

#include "Misc/AutomationTest.h"
#include "Livestock/GoatActor.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-01 T-054] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThiefReactionTest, "UnderTheMountain.Thief.Reactions",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThiefReactionTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-054] Only death ragdolls.
	TestTrue(TEXT("dead ragdolls"), AThiefCharacter::ShouldEnterRagdoll(EThiefState::Dead));
	TestFalse(TEXT("sneaking does not ragdoll"),
			  AThiefCharacter::ShouldEnterRagdoll(EThiefState::Sneak));
	TestFalse(TEXT("fleeing does not ragdoll"),
			  AThiefCharacter::ShouldEnterRagdoll(EThiefState::Flee));
	TestFalse(TEXT("scared does not ragdoll"),
			  AThiefCharacter::ShouldEnterRagdoll(EThiefState::Scared));
	TestFalse(TEXT("grabbing does not ragdoll"),
			  AThiefCharacter::ShouldEnterRagdoll(EThiefState::Grab));

	// [2026-11-01 T-054] Flinch: a 0.28 s sine bump - zero at both
	// ends (no snap), peak 14 degrees mid-flinch, nothing outside.
	TestEqual(TEXT("flinch starts at zero"), AThiefCharacter::FlinchPitchDegrees(0.28f, 0.28f), 0.0f);
	TestEqual(TEXT("flinch ends at zero"), AThiefCharacter::FlinchPitchDegrees(0.0f, 0.28f), 0.0f);
	TestTrue(TEXT("flinch peaks mid-window"),
			 FMath::IsNearlyEqual(AThiefCharacter::FlinchPitchDegrees(0.14f, 0.28f), 14.0f, 0.01f));
	TestEqual(TEXT("no flinch after the window"),
			  AThiefCharacter::FlinchPitchDegrees(-0.1f, 0.28f), 0.0f);
	TestEqual(TEXT("broken duration cannot flinch"),
			  AThiefCharacter::FlinchPitchDegrees(0.1f, 0.0f), 0.0f);

	// [2026-11-01 T-054] Goat side-fall: standing at 0, fully over
	// (-88) at 1, monotonic between, clamped outside.
	TestEqual(TEXT("goat standing at alpha 0"), AGoatActor::DeathFallRollDegrees(0.0f), 0.0f);
	TestEqual(TEXT("goat fully over at alpha 1"), AGoatActor::DeathFallRollDegrees(1.0f), -88.0f);
	TestEqual(TEXT("goat fall clamps below 0"), AGoatActor::DeathFallRollDegrees(-0.5f), 0.0f);
	TestEqual(TEXT("goat fall clamps above 1"), AGoatActor::DeathFallRollDegrees(1.5f), -88.0f);
	float Previous = 0.0f;
	for (float Alpha = 0.1f; Alpha <= 1.0f; Alpha += 0.1f)
	{
		const float Roll = AGoatActor::DeathFallRollDegrees(Alpha);
		TestTrue(FString::Printf(TEXT("goat fall monotonic at %.1f"), Alpha), Roll <= Previous);
		Previous = Roll;
	}

	// [2026-11-01 T-054] A goat actor carries the skeletal mesh
	// component the fall and walk clip drive (a greybox regression
	// would fail this before any visual check).
	AGoatActor* Goat = NewObject<AGoatActor>();
	TestNotNull(TEXT("goat has a skeletal mesh component"),
				Goat->FindComponentByClass<USkeletalMeshComponent>());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
