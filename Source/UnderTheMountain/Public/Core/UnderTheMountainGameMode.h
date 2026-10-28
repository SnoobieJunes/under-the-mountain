// Copyright Under the Mountain contributors.
// Purpose: game mode for the walking skeleton — binds the dragon pawn and
// player controller. Level content stays in the map, never in code (T-012
// builds the real lair in the same map slot). Created 2026-10-27 (T-003).

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
