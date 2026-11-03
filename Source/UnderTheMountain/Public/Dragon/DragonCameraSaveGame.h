// Copyright Under the Mountain contributors.
// Purpose: T-065 camera-choice save slot (tint-style persistence): which of
// the three dragon cameras is active. Data only — the pawn reads/writes it
// through its CameraSaveSlotName member (production slot by default, test
// slot in tests). Created 2026-11-03 (T-065).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DragonCameraSaveGame.generated.h"

UCLASS()
class UNDERTHEMOUNTAIN_API UDragonCameraSaveGame : public USaveGame
{
	GENERATED_BODY()

  public:
	// Active camera as an int (EDragonCameraMode); validated on load, so a
	// corrupt slot falls back to the default chase instead of a bad mode.
	UPROPERTY()
	int32 CameraModeIndex = 0;
};
