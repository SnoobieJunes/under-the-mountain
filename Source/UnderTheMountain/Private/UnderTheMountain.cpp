// Copyright Under the Mountain contributors.
// [2026-10-27 T-002] Module implementation: startup/shutdown logging only.
// Gameplay arrives per-task from T-003; nothing here may grow into a god object.

#include "UnderTheMountain.h"

DEFINE_LOG_CATEGORY(LogUnderTheMountain);

void FUnderTheMountainModule::StartupModule()
{
	UE_LOG(LogUnderTheMountain, Log, TEXT("UnderTheMountain module started, version %s"), *UnderTheMountain::GetGameVersion());
}

void FUnderTheMountainModule::ShutdownModule()
{
	UE_LOG(LogUnderTheMountain, Log, TEXT("UnderTheMountain module shut down"));
}

namespace UnderTheMountain
{
FString GetGameVersion()
{
	// [2026-10-27 T-002] Pre-alpha: bumped by the packaging task, not by hand
	// in feature commits, so the string means "which submission build".
	return TEXT("0.1.0-hackathon");
}
} // namespace UnderTheMountain

IMPLEMENT_PRIMARY_GAME_MODULE(FUnderTheMountainModule, UnderTheMountain, "UnderTheMountain");
