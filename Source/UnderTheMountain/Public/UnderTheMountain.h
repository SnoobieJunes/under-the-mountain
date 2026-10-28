// Copyright Under the Mountain contributors.
// Purpose: module entry point + the project's version seam.
// Invariants: this header owns the module declaration; version comes only from
// GetGameVersion() so tests, HUD and packaging never hard-code divergent strings.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// [2026-10-27 T-002] Declare module log category here so every gameplay file
// logs to one category instead of inventing per-file categories.
DECLARE_LOG_CATEGORY_EXTERN(LogUnderTheMountain, Log, All);

class FUnderTheMountainModule : public IModuleInterface
{
  public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

// [2026-10-27 T-002] Single source of truth for the game version string.
// Exercised by the T-002 smoke automation test; packaging (T-020) reads it too.
namespace UnderTheMountain
{
FString GetGameVersion();
}
