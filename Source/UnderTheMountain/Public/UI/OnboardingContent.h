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
		Row(TEXT("IA_SteerHeld"), TEXT("Right mouse (hold) — sail: mouse steers the nose"), TEXT("—")),
		Row(TEXT("IA_Sink"), TEXT("Left Ctrl"), TEXT("—")),
		Row(TEXT("IA_SteerLatch"), TEXT("—"), TEXT("D-pad Up")),
		Row(TEXT("IA_TrueSight"), TEXT("T"), TEXT("X (left face)")),
		Row(TEXT("IA_ScentToggle"), TEXT("N"), TEXT("D-pad Down")),
		Row(TEXT("IA_TintPicker"), TEXT("C"), TEXT("B tap (no claw target)")),
		// [2026-11-04 T-102] Adopted pad map: fire RB, eat on the
		// right-face hold multiplex; IA_Dive has no binding (posture).
		Row(TEXT("IA_Fire"), TEXT("Left mouse (hold)"), TEXT("Right shoulder (hold)")),
		Row(TEXT("IA_Roar"), TEXT("R"), TEXT("Left shoulder")),
		Row(TEXT("IA_Eat"), TEXT("E"), TEXT("B hold (ground, via IA_RightFace)")),
		// [2026-11-04 T-076/T-070/T-069/T-102] New bound actions.
		Row(TEXT("IA_Claw"), TEXT("Q"), TEXT("B tap (claw target in arc, via IA_RightFace)")),
		Row(TEXT("IA_PickupCarry"), TEXT("P"), TEXT("B hold (ground, via IA_RightFace)")),
		Row(TEXT("IA_Surge"), TEXT("W tap (momentum)"), TEXT("Right trigger")),
		Row(TEXT("IA_Brake"), TEXT("Left Shift (hold)"), TEXT("Left trigger (hold)")),
		Row(TEXT("IA_RightFace"), TEXT("—"), TEXT("B (right face): tap/hold multiplex")),
		Row(TEXT("IA_SmartLock"), TEXT("Tab"), TEXT("L3 (left-stick press)")),
		Row(TEXT("IA_WingBuffet"), TEXT("X"), TEXT("D-pad Left")),
		Row(TEXT("IA_TailSlam"), TEXT("Z"), TEXT("D-pad Right")),
		Row(TEXT("IA_Flap"), TEXT("Space"), TEXT("Cross (bottom face)")),
		Row(TEXT("IA_LandingAssist"), TEXT("F"), TEXT("Triangle (top face)")),
		Row(TEXT("IA_FreeLook"), TEXT("Right mouse (hold)"), TEXT("—")),
		Row(TEXT("IA_Sink"), TEXT("Left Ctrl"), TEXT("Circle hold (air, via IA_RightFace)")),
		Row(TEXT("IA_SteerLatch"), TEXT("Double-click right mouse"), TEXT("L3 (left-stick click)")),
		Row(TEXT("IA_TrueSight"), TEXT("T"), TEXT("D-pad Up")),
		Row(TEXT("IA_ScentToggle"), TEXT("N"), TEXT("D-pad Down tap (via IA_DPadDown)")),
		Row(TEXT("IA_TintPicker"), TEXT("C"), TEXT("D-pad Down hold 0.35s (via IA_DPadDown)")),
		// [2026-11-04] DualSense pad map v2 (T-102 kept: R2 Surge /
		// L2 Brake analog, R1 Fire): Square claws directly, Circle is
		// hold-interact only, WingBuffet is the L1+R1 chord, and
		// targeting cycles on D-pad Left (prev) / Right (next).
		// PlayStation names throughout (Cross/Circle/Square/Triangle,
		// Options/Create, L1/R1/L2/R2). IA_Dive is deleted (posture).
		Row(TEXT("IA_Fire"), TEXT("Left mouse (hold)"), TEXT("R1 (hold)")),
		Row(TEXT("IA_Roar"), TEXT("R"), TEXT("L1")),
		Row(TEXT("IA_Eat"), TEXT("E"), TEXT("Circle hold (ground, via IA_RightFace)")),
		Row(TEXT("IA_Claw"), TEXT("Q"), TEXT("Square (left face)")),
		Row(TEXT("IA_PickupCarry"), TEXT("P"), TEXT("Circle hold (ground, via IA_RightFace)")),
		Row(TEXT("IA_Surge"), TEXT("W tap (momentum)"), TEXT("R2 (analog pull)")),
		Row(TEXT("IA_Brake"), TEXT("Left Shift (hold)"), TEXT("L2 (analog hold)")),
		Row(TEXT("IA_RightFace"), TEXT("—"), TEXT("Circle: hold interact only")),
		Row(TEXT("IA_SmartLock"), TEXT("Tab (next)"), TEXT("D-pad Left (prev) / Right (next)")),
		Row(TEXT("IA_WingBuffet"), TEXT("X"), TEXT("L1+R1 chord")),
		Row(TEXT("IA_TailSlam"), TEXT("Z"), TEXT("Triangle (ground, via IA_LandingAssist)")),
		Row(TEXT("IA_PadLook"), TEXT("—"), TEXT("Right stick")),
		Row(TEXT("IA_PadMove"), TEXT("—"), TEXT("Left stick")),
		Row(TEXT("IA_DPadDown"), TEXT("—"), TEXT("D-pad Down: tap scent / hold tint")),
		Row(TEXT("IA_CameraCycle"), TEXT("V"), TEXT("R3 (right-stick click)")),
		Row(TEXT("IA_Pause"), TEXT("Esc"), TEXT("Options")),
		Row(TEXT("IA_Help"), TEXT("H"), TEXT("Create")),
		// [2026-11-04 T-084] Sleep has no gamepad binding (see pawn).
		Row(TEXT("IA_Sleep"), TEXT("Y (at the hoard mound)"), TEXT("—")),
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
			   TEXT("CROSS — flap to climb, then glide")),
		Prompt(EOnboardingVerb::Dive, TEXT("Push the nose down past level to DIVE (S flares)"),
			   TEXT("Push the nose down past level to DIVE")),
		Prompt(EOnboardingVerb::Fire, TEXT("Hold LEFT MOUSE — breathe fire at the red target"),
			   TEXT("Hold R1 — breathe fire at the red target")),
		Prompt(EOnboardingVerb::Roar, TEXT("R — ROAR: scares thieves, drops loot"),
			   TEXT("L1 — ROAR: scares thieves, drops loot")),
		Prompt(EOnboardingVerb::Eat, TEXT("E — EAT goats and the fallen to heal"),
			   TEXT("CIRCLE HOLD — EAT goats and the fallen to heal")),
		Prompt(EOnboardingVerb::Scent, TEXT("N — scent ribbons point at treasure"),
			   TEXT("D-PAD DOWN — scent ribbons point at treasure")),
		Prompt(EOnboardingVerb::Tint, TEXT("C — scale tint picker"),
			   TEXT("D-PAD DOWN HOLD — scale tint picker")),
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
