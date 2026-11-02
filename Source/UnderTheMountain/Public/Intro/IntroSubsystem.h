// Copyright Under the Mountain contributors.
// Purpose: the intro sequence (T-018, docs/GAMEPLAY.md intro
// branches) - wake on the hoard, a gnome-thief steals, and the
// player's first real choice (let go / chase) with its Notoriety
// consequence. The sequence is a state machine the pawn drives;
// this subsystem owns states, branch points and the audit
// suppression that keeps intro accounting exact (the generic
// escape audit must not double-charge an intro escape).
// Created 2026-11-01 (T-018).
// Invariants: the choice can never soft-lock - the gnome freezes
// only while the choice is on screen, every branch resumes the
// world, and a 20 s choice timeout resolves as Let Go. Branch
// points are the spec table's, as pure statics the test reads.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IntroSubsystem.generated.h"

enum class EIntroState : uint8
{
	Sleeping,
	WakeUp,
	GnomeSteals,
	Choice,
	FreePlay
};

enum class EIntroBranch : uint8
{
	None,
	LetGo,
	Chase
};

UCLASS()
class UNDERTHEMOUNTAIN_API UIntroSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-018] Spec branch points (intro table): letting
	// the gnome go costs +5 (it talks); a failed chase also lands
	// +5 when it gets out; chase outcomes that resolve in the
	// world (kill outside +10, roar-scare +4) come from the
	// generic notoriety verbs, not from here.
	static int32 LetGoPoints();
	static int32 FailedChasePoints();

	EIntroState GetState() const;
	EIntroBranch GetBranch() const;
	float GetWakeProgress() const;

	// [2026-11-01 T-018] Advance the timed states. GnomeSteals is
	// event-driven (the pawn reports the steal); Choice waits for
	// Choose() or the timeout.
	void TickIntro(float DeltaSeconds);
	void ReportGnomeStole();
	void Choose(EIntroBranch Branch);

	// [2026-11-01 T-018] The gnome actor's name, set by the pawn
	// at spawn. The loop audit asks ShouldSuppressEscapeGain for
	// it so the intro escape is charged once, here, at the spec's
	// flat intro rate instead of the generic +8-with-loot.
	void SetGnomeName(FName InName);
	bool ShouldSuppressEscapeGain(FName ThiefName, int32& OutReplacementPoints) const;

	// [2026-11-01 T-018] utm.intro.enabled=0 path: no sequence, no
	// gnome, straight to free play (captures, tests, PIE checks
	// that are not about the intro).
	void SkipToFreePlay();

  private:
	EIntroState State = EIntroState::Sleeping;
	EIntroBranch Branch = EIntroBranch::None;
	float StateTime = 0.0f;
	FName GnomeName = NAME_None;
	bool bEscapeCharged = false;
};
