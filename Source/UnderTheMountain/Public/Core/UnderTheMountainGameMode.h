// Copyright Under the Mountain contributors.
// Purpose: game mode for the walking skeleton. Created 2026-10-27 (T-003).
// [2026-10-28 T-041] The .cpp sets only DefaultPawnClass to the dragon
// pawn; the player controller is the engine default and is not bound
// here. Level content stays in the map, never in code (T-012 builds
// the real lair in the same map slot).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UnderTheMountainGameMode.generated.h"

UCLASS()
class UNDERTHEMOUNTAIN_API AUnderTheMountainGameMode : public AGameModeBase
{
	GENERATED_BODY()

  public:
	AUnderTheMountainGameMode();
};
