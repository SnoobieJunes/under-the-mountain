// Copyright Under the Mountain contributors.
// [2026-11-01 T-049] Editor module entry point. The module exists to
// host editor-only tooling (today: the terrain commandlet); it has
// no runtime behaviour of its own, so startup/shutdown are empty.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUnderTheMountainEditorModule : public IModuleInterface
{
  public:
	virtual void StartupModule() override
	{
	}
	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FUnderTheMountainEditorModule, UnderTheMountainEditor)
