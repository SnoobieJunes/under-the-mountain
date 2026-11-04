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
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
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

	// [2026-11-04 T-133] Per-member party role override (dwarf spawns
	// set Carrier/Trapper/Guard per member; None keeps ThiefData->Role).
	UPROPERTY(EditAnywhere, Category = "Thief")
	EThiefRole PartyRole = EThiefRole::None;

	// [2026-11-04 T-089] Authored waypoint route injected per spawn /
	// per placement (level or spawner owns the list; this actor holds
	// no map knowledge). Empty = steer straight at the brain's
	// destination. NavMesh volumes are editor work on top of this seam.
	UPROPERTY(EditAnywhere, Category = "Thief")
	TArray<FVector> RouteWaypoints;

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

#if WITH_DEV_AUTOMATION_TESTS
	// [2026-11-03 T-104] Test seam (dev builds only): attach a
	// headless-built brain (NewObject + Init, driven to whatever state
	// the scenario needs) so wave-resolution tests can cover live
	// thieves without spawning a world. Production always builds the
	// brain in BeginPlay; nothing outside tests calls this.
	void SetBrainForTesting(UThiefBrain* InBrain)
	{
		Brain = InBrain;
	}
#endif

	// [2026-10-28 T-014] Dragon-roar hook for combat code: forwards to
	// the brain, which owns the drop/scare rules. No-op (with the
	// BeginPlay warnings as evidence) before the brain exists.
	void OnRoarHeard();

	// [2026-10-28 T-014] Dragon-attack hook: forwards to the brain and
	// reports whether the blow killed the thief.
	bool ApplyThiefDamage(float Damage);

	// [2026-11-01 T-054] Death presentation rule: a Dead brain means
	// the ragdoll (physics settle), every other state means the
	// single-node locomotion. Pure so the test proves the wiring
	// decision without a world.
	static bool ShouldEnterRagdoll(EThiefState State);

	// [2026-11-01 T-054] Hit-flinch curve: pitch kick in degrees at
	// TimeRemaining of a Duration-second flinch - a sine bump (0 at
	// both ends, peak mid-flinch), clamped outside the window.
	// Authored in code because no hit-reaction clip exists in the
	// Meshy sets; if one is authored later it replaces this curve.
	static float FlinchPitchDegrees(float TimeRemaining, float Duration);

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

	// [2026-11-01 T-054] Hit-reaction bookkeeping: last brain HP
	// seen (a drop starts a flinch), seconds left on the flinch,
	// and the one-shot ragdoll latch for the death transition.
	float LastBrainHP = -1.0f;
	float FlinchTimeRemaining = 0.0f;
	bool bRagdollEntered = false;

	// [2026-11-01 T-054] The mesh's rest relative rotation,
	// captured at BeginPlay so the flinch can add pitch over it
	// instead of overwriting the engine's -90 yaw on GetMesh().
	FRotator BaseMeshRotation = FRotator::ZeroRotator;

	// [2026-11-04 T-089] Route progress: index into RouteWaypoints and
	// the brain destination the route was being followed toward (a new
	// destination — treasure grabbed, now fleeing — restarts the route).
	int32 RouteWaypointIndex = 0;
	FVector LastRouteDestination = FVector::ZeroVector;
	bool bHasRouteDestination = false;
};
