// Copyright Under the Mountain contributors.
// Purpose: the in-world thief — an ACharacter whose UThiefBrain
// decides where to go (treasure, then exit) while this actor only
// walks, mirrors the brain's speeds, forwards roar/damage events, and
// shows the per-type body. [2026-10-28 T-044] The visible body is the
// Meshy skeletal mesh from ThiefData when one is assigned, with the
// T-014 greybox sphere kept hidden as the no-mesh fallback. Created
// 2026-10-28 (T-014).
// Invariants: all steal/flee decisions live in UThiefBrain; this actor
// never reads the treasure registry directly, and both bodies are
// visual-only so capsule collision stays the single movement truth.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ThiefCharacter.generated.h"

class UAnimSequence;
class UStaticMeshComponent;
class UThiefBrain;
class UThiefDataAsset;

UCLASS()
class UNDERTHEMOUNTAIN_API AThiefCharacter : public ACharacter
{
	GENERATED_BODY()

  public:
	AThiefCharacter();

	// [2026-10-28 T-014] Authored thief type (burglar/adventurer/dwarf
	// variants are data, not subclasses). Read once at BeginPlay.
	UPROPERTY(EditAnywhere, Category = "Thief")
	TObjectPtr<UThiefDataAsset> ThiefData;

	// [2026-10-28 T-014] World location the thief runs to in
	// Flee/Scared — authored per spawn so cave mouths stay level data.
	UPROPERTY(EditAnywhere, Category = "Thief")
	FVector ExitLocation = FVector::ZeroVector;

	// [2026-10-28 T-014] The decision-maker, created in BeginPlay once
	// the world's treasure registry can be injected into it.
	UPROPERTY(VisibleAnywhere, Category = "Thief")
	TObjectPtr<UThiefBrain> Brain;

	// [2026-10-28 T-044] Greybox body (engine sphere), now the fallback
	// shown only when ThiefData has no MeshAsset; BeginPlay hides it in
	// favour of the skeletal mesh. Kept as a component (never removed)
	// so a type whose mesh is not assigned yet still reads in-world.
	// Scaled from ThiefData.BodyScale at BeginPlay when it is shown.
	UPROPERTY(VisibleAnywhere, Category = "Thief")
	TObjectPtr<UStaticMeshComponent> Body;

	UThiefBrain* GetBrain() const;

	// [2026-10-28 T-014] Dragon-roar hook for combat code: forwards to
	// the brain, which owns the drop/scare rules. No-op (with the
	// BeginPlay warnings as evidence) before the brain exists.
	void OnRoarHeard();

	// [2026-10-28 T-014] Dragon-attack hook: forwards to the brain and
	// reports whether the blow killed the thief.
	bool ApplyThiefDamage(float Damage);

  protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

  private:
	// [2026-10-28 T-014] Latches after the escape is logged so the
	// "reached exit" line prints exactly once.
	bool bLoggedExit = false;

	// [2026-10-28 T-014 resume] Accumulates toward the 10s
	// position/state debug log in Tick. This field was missing when
	// the task was parked (the stash would not compile without it).
	float DebugLogTimer = 0.0f;

	// [2026-10-28 T-014] Last chamber the steering latched onto; a
	// change resets the two-stage waypoint so progress is monotonic.
	int32 LastChamber = -1;

	// [2026-10-28 T-014] Stage latch: false = steering to own centre,
	// true = steering to the next centre. Once true it stays true
	// until the chamber changes, so passing the centre cannot
	// flip the target back (the pre-fix oscillation).
	bool bHeadingToNextCenter = false;

	// [2026-10-28 T-044] Last single-node anim handed to PlayAnimation,
	// so Tick only (re)starts playback on a walk/run/still change and
	// never restarts the same clip every tick. A raw pointer is safe
	// here: the anim is owned by ThiefData, which this actor holds via
	// its UPROPERTY for its whole lifetime. Null means bind pose.
	UAnimSequence* LastPlayedAnim = nullptr;

	// [2026-10-28 T-044] True while LastPlayedAnim is meant to be
	// playing; pairs with it so the still case calls Stop() once on
	// the transition into stillness, not every tick.
	bool bAnimPlaying = false;

	// [2026-10-28 T-014] Trap-placement cadence accumulator: Tick
	// adds DeltaSeconds each frame and attempts a placement every
	// 5 s (docs/GAMEPLAY.md 5.2 — the trapper "pauses to place
	// traps"). The brain owns budget/state rules; this actor owns
	// the cadence and the ATrapActor spawn.
	float TrapPlaceTimer = 0.0f;
};
