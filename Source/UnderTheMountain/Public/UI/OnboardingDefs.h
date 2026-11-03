// Copyright Under the Mountain contributors.
// Purpose: T-073 onboarding content — the prompt chain and the help
// overlay's binding table, generated from the shipped scheme. ONE
// source: the overlay widget, the prompt text, and the
// completeness test (every MappingContext action has a row) all read
// this table, so T-019's parity checklist stays true by construction.
// Created 2026-11-03 (T-073).

#pragma once

#include "CoreMinimal.h"
#include "OnboardingDefs.generated.h"

// [2026-11-03 T-073] Prompt chain order = teaching order: locomotion,
// then combat, then senses, cosmetics, camera. Scent/tint/cameras are
// sequenced (not proximity-gated): each appears when the previous
// verb is first performed.
UENUM(BlueprintType)
enum class EOnboardingVerb : uint8
{
	Flap UMETA(DisplayName = "Flap"),
	Dive UMETA(DisplayName = "Dive"),
	Fire UMETA(DisplayName = "Fire"),
	Roar UMETA(DisplayName = "Roar"),
	Eat UMETA(DisplayName = "Eat"),
	Scent UMETA(DisplayName = "Scent"),
	Tint UMETA(DisplayName = "Tint"),
	Camera UMETA(DisplayName = "Camera"),
	Count UMETA(Hidden),
};

// [2026-11-03 T-073] One prompt row: keyboard-first text plus the pad
// equivalent. Both are shown (keyboard line, pad line) so neither
// device reads the other's bindings.
USTRUCT(BlueprintType)
struct FOnboardingPromptDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Onboarding")
	EOnboardingVerb Verb = EOnboardingVerb::Flap;

	UPROPERTY(EditAnywhere, Category = "Onboarding")
	FString KeyboardText;

	UPROPERTY(EditAnywhere, Category = "Onboarding")
	FString PadText;
};

// [2026-11-03 T-073] One help-overlay row per input action, keyed by
// the action object's name (IA_*): the completeness test matches
// MappingContext entries against these names, so an unlisted binding
// fails the gate instead of shipping undocumented.
USTRUCT(BlueprintType)
struct FOnboardingBindingDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Onboarding")
	FString ActionName;

	UPROPERTY(EditAnywhere, Category = "Onboarding")
	FString KeyboardKeys;

	UPROPERTY(EditAnywhere, Category = "Onboarding")
	FString PadKeys;
};
