// Copyright Under the Mountain contributors.
// Purpose: T-002 toolchain smoke test — proves the module loads and its real
// entry point (GetGameVersion) answers, inside the Unreal Automation framework
// that Scripts/test-fast.sh drives. Not a placeholder assert: it exercises the
// same function the module logs at startup and packaging will stamp.

#include "Misc/AutomationTest.h"
#include "UnderTheMountain.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-27 T-002] Editor-context test: runs headless via UnrealEditor-Cmd
// -ExecCmds="Automation RunTests UnderTheMountain.Smoke" (Scripts/test-fast.sh).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderTheMountainSmokeTest, "UnderTheMountain.Smoke.Version",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnderTheMountainSmokeTest::RunTest(const FString& Parameters)
{
	const FString Version = UnderTheMountain::GetGameVersion();
	TestFalse(TEXT("game version must not be empty"), Version.IsEmpty());
	TestTrue(TEXT("game version carries the hackathon suffix"), Version.EndsWith(TEXT("-hackathon")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
