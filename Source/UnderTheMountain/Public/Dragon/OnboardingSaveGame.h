// Copyright Under the Mountain contributors.
// Purpose: T-073 onboarding persistence — which verbs the player has
// performed, so prompts never reappear after first success, across
// sessions. Slot "OnboardingProgressV1" (fresh slot per format, like
// the camera/tint saves). Data only; the pawn reads/writes it.
// Created 2026-11-03 (T-073).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "OnboardingSaveGame.generated.h"

UCLASS()
class UNDERTHEMOUNTAIN_API UOnboardingSaveGame : public USaveGame
{
	GENERATED_BODY()

  public:
	// [2026-11-03 T-073] One flag per EOnboardingVerb, indexed by the
	// enum (Count-sized). True = performed at least once, prompt done
	// forever.
	UPROPERTY()
	TArray<bool> VerbDone;
};
