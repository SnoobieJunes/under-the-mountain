// Copyright Under the Mountain contributors.
// Purpose: T-073 onboarding content — the shipped-scheme binding table
// and the prompt chain text. ONE source: the help overlay renders this
// table, prompts read these rows, and the completeness test asserts
// every MappingContext action has a row, so the overlay IS the
// T-019 parity list and can never drift from the shipped map.
// Created 2026-11-03 (T-073).

#pragma once

#include "CoreMinimal.h"
#include "UI/OnboardingDefs.h"

// [2026-11-03 T-073] Key names below are display strings matching the
// adopted map in BuildInputActions (keyboard first, pad second). If a
// binding moves, this table moves with it — the completeness test
// fails otherwise.
inline TArray<FOnboardingBindingDef> GetOnboardingBindings()
{
	auto Row = [](const TCHAR* Action, const TCHAR* Keys, const TCHAR* Pad)
	{
		FOnboardingBindingDef Def;
		Def.ActionName = Action;
		Def.KeyboardKeys = Keys;
		Def.PadKeys = Pad;
		return Def;
	};
	return {
		Row(TEXT("IA_Move"), TEXT("W A S D"), TEXT("Left stick")),
		Row(TEXT("IA_Look"), TEXT("Mouse"), TEXT("Right stick")),
		Row(TEXT("IA_Flap"), TEXT("Space"), TEXT("A (bottom face)")),
		Row(TEXT("IA_ToggleFlight"), TEXT("F"), TEXT("Y (top face)")),
		Row(TEXT("IA_Dive"), TEXT("Left Shift"), TEXT("Right trigger")),
		Row(TEXT("IA_FreeLook"), TEXT("Right mouse (hold)"), TEXT("—")),
		Row(TEXT("IA_Sink"), TEXT("Left Ctrl"), TEXT("—")),
		Row(TEXT("IA_SteerLatch"), TEXT("—"), TEXT("D-pad Up")),
		Row(TEXT("IA_TrueSight"), TEXT("T"), TEXT("X (left face)")),
		Row(TEXT("IA_ScentToggle"), TEXT("N"), TEXT("D-pad Down")),
		Row(TEXT("IA_TintPicker"), TEXT("C"), TEXT("B (right face)")),
		Row(TEXT("IA_Fire"), TEXT("Left mouse (hold)"), TEXT("Left trigger (hold)")),
		Row(TEXT("IA_Roar"), TEXT("R"), TEXT("Left shoulder")),
		Row(TEXT("IA_Eat"), TEXT("E"), TEXT("Right shoulder")),
		Row(TEXT("IA_WingBuffet"), TEXT("X"), TEXT("D-pad Left")),
		Row(TEXT("IA_TailSlam"), TEXT("Z"), TEXT("D-pad Right")),
		Row(TEXT("IA_PadLook"), TEXT("—"), TEXT("Right stick")),
		Row(TEXT("IA_PadMove"), TEXT("—"), TEXT("Left stick")),
		Row(TEXT("IA_CameraCycle"), TEXT("V"), TEXT("R3 (right-stick press)")),
		Row(TEXT("IA_Pause"), TEXT("Esc"), TEXT("Menu (Start)")),
		Row(TEXT("IA_Help"), TEXT("H"), TEXT("Back (Select)")),
	};
}

// [2026-11-03 T-073] Prompt chain in teaching order (locomotion, then
// combat, then senses, cosmetics, camera). Shown one at a time by the
// pawn's onboarding tick; each dismisses permanently on first success.
inline TArray<FOnboardingPromptDef> GetOnboardingPrompts()
{
	auto Prompt = [](EOnboardingVerb Verb, const TCHAR* Keys, const TCHAR* Pad)
	{
		FOnboardingPromptDef Def;
		Def.Verb = Verb;
		Def.KeyboardText = Keys;
		Def.PadText = Pad;
		return Def;
	};
	return {
		Prompt(EOnboardingVerb::Flap, TEXT("SPACE — flap to climb, then glide"),
			   TEXT("A — flap to climb, then glide")),
		Prompt(EOnboardingVerb::Dive, TEXT("Push the nose down past level to DIVE (S flares)"),
			   TEXT("Push the nose down past level to DIVE")),
		Prompt(EOnboardingVerb::Fire, TEXT("Hold LEFT MOUSE — breathe fire at the red target"),
			   TEXT("Hold LEFT TRIGGER — breathe fire at the red target")),
		Prompt(EOnboardingVerb::Roar, TEXT("R — ROAR: scares thieves, drops loot"),
			   TEXT("LEFT SHOULDER — ROAR: scares thieves, drops loot")),
		Prompt(EOnboardingVerb::Eat, TEXT("E — EAT goats and the fallen to heal"),
			   TEXT("RIGHT SHOULDER — EAT goats and the fallen to heal")),
		Prompt(EOnboardingVerb::Scent, TEXT("N — scent ribbons point at treasure"),
			   TEXT("D-PAD DOWN — scent ribbons point at treasure")),
		Prompt(EOnboardingVerb::Tint, TEXT("C — scale tint picker"),
			   TEXT("B — scale tint picker")),
		Prompt(EOnboardingVerb::Camera, TEXT("V — cycle camera (head / wide / first-person)"),
			   TEXT("R3 — cycle camera (head / wide / first-person)")),
	};
}

// [2026-11-03 T-073] The prompt arbiter, pure and headless-tested.
// Two lanes: combat prompts preempt whenever their context holds
// (rare moments must not wait), basics run in teaching sequence.
// Nothing here can deadlock: combat never blocks basics, and the
// basics always advance on performance. Count = show nothing.
inline EOnboardingVerb NextOnboardingPrompt(const bool* Performed, bool HasFireTarget,
											bool ThiefNear, bool GoatNear)
{
	const int32 F = static_cast<int32>(EOnboardingVerb::Flap);
	const int32 D = static_cast<int32>(EOnboardingVerb::Dive);
	const int32 Fi = static_cast<int32>(EOnboardingVerb::Fire);
	const int32 R = static_cast<int32>(EOnboardingVerb::Roar);
	const int32 E = static_cast<int32>(EOnboardingVerb::Eat);
	const int32 S = static_cast<int32>(EOnboardingVerb::Scent);
	const int32 T = static_cast<int32>(EOnboardingVerb::Tint);
	const int32 C = static_cast<int32>(EOnboardingVerb::Camera);
	if (!Performed[Fi] && HasFireTarget)
	{
		return EOnboardingVerb::Fire;
	}
	if (!Performed[R] && ThiefNear)
	{
		return EOnboardingVerb::Roar;
	}
	if (!Performed[E] && GoatNear)
	{
		return EOnboardingVerb::Eat;
	}
	if (!Performed[F])
	{
		return EOnboardingVerb::Flap;
	}
	if (!Performed[D])
	{
		return EOnboardingVerb::Dive;
	}
	if (!Performed[S])
	{
		return EOnboardingVerb::Scent;
	}
	if (!Performed[T])
	{
		return EOnboardingVerb::Tint;
	}
	if (!Performed[C])
	{
		return EOnboardingVerb::Camera;
	}
	return EOnboardingVerb::Count;
}
