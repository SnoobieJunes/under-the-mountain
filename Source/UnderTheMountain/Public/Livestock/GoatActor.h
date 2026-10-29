// Copyright Under the Mountain contributors.
// Purpose: a greybox goat for the exterior flight zone. It wanders a
// small pasture around its home so the outside world reads as alive
// (and stocked) before real livestock art and behaviour land.
// Created 2026-10-28 (T-017).
// Invariants: HomeLocation is the actor's location captured in
// BeginPlay (placement sets the location before BeginPlay runs, so
// the captured location IS the authored home) unless SetHomeLocation
// overrode it first; the actor never leaves WanderRadius of home and
// never leaves home's Z plane; Body and Head are NoCollision — the
// dragon capsule and world geometry own collision, and the pawn
// finds goats by range, not by touching them.
// [2026-10-28 T-015] HP is clamped at 0 from below by
// ApplyGoatDamage; once dead the goat never wanders again, and once
// eaten it is hidden, collision-free, and stays eaten — neither
// transition reverses.
// [2026-10-28 T-017] Honest scope: the sphere/cube body below is a
// greybox stand-in until the Meshy goat (docs/WORLD-ASSETS.md LIV-01,
// approval-gated) replaces it. Wander uses no navmesh: the pasture
// is open and flat, so steering straight at a random in-radius
// target is enough.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoatActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API AGoatActor : public AActor
{
	GENERATED_BODY()

  public:
	AGoatActor();

	// [2026-10-28 T-017] Scene root so Body/Head carry only their own
	// greybox offsets and the actor transform stays the goat's ground
	// position (home capture and wander math read it directly).
	UPROPERTY(VisibleAnywhere, Category = "Livestock")
	TObjectPtr<USceneComponent> SceneRoot;

	// [2026-10-28 T-017] Greybox body: engine sphere scaled into a
	// goat-ish ellipsoid in the constructor. Replaced wholesale when
	// the Meshy goat (LIV-01) is approved; wander logic never reads it.
	UPROPERTY(VisibleAnywhere, Category = "Livestock")
	TObjectPtr<UStaticMeshComponent> Body;

	// [2026-10-28 T-017] Greybox head: engine cube attached to Body so
	// heading reads at a glance while flying over. Like Body, it is a
	// stand-in for the LIV-01 mesh, not authored art.
	UPROPERTY(VisibleAnywhere, Category = "Livestock")
	TObjectPtr<UStaticMeshComponent> Head;

	// [2026-10-28 T-017] Overrides the wander home. Placement normally
	// just sets the actor location before BeginPlay captures it as
	// home; this setter exists for callers that must pin home
	// explicitly (before or after BeginPlay) — a pre-BeginPlay call is
	// honoured, not overwritten by the capture.
	void SetHomeLocation(const FVector& NewHomeLocation);

	// [2026-10-28 T-017] The current wander home (see header
	// invariants for how it is established).
	FVector GetHomeLocation() const;

	// [2026-10-28 T-015] Hit points; the authored default IS the full
	// pool (a goat has no separate MaxHP — nothing heals a goat).
	// 30 is under two seconds of held fire breath (18 DPS in
	// docs/GAMEPLAY.md section 2), so killing lunch costs a moment,
	// not a fight. Public EditAnywhere like the dragon's pools: the
	// editor and tests may set it, production writes go through
	// ApplyGoatDamage.
	UPROPERTY(EditAnywhere, Category = "Livestock|Food")
	float HP = 30.0f;

	// [2026-10-28 T-015] What the dragon gains for eating this goat
	// (docs/GAMEPLAY.md section 2: livestock +50 stamina/+10 HP).
	// The values live on the goat, not on the combat component, so a
	// future fat sheep is a data edit per placement; the eat channel
	// reads them at completion and applies them through
	// UDragonStatsComponent's verbs.
	UPROPERTY(EditAnywhere, Category = "Livestock|Food")
	float FoodStamina = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Livestock|Food")
	float FoodHP = 10.0f;

	// [2026-10-28 T-015] Subtracts Damage from HP (negative damage is
	// ignored; nothing heals a goat), clamped at 0. The hit that
	// reaches 0 marks the goat dead — wander stops from the next
	// Tick — logs "Goat down", and returns true; every other hit
	// (including any against a corpse) returns false, mirroring
	// UThiefBrain::ApplyDamage's death-report contract.
	bool ApplyGoatDamage(float Damage);

	// [2026-10-28 T-015] True once HP reached 0 (or the goat was
	// eaten, which implies dead). A dead goat stays visible where it
	// fell until the dragon eats the body.
	bool IsDead() const;

	// [2026-10-28 T-015] True once Consume() ran: the body is gone
	// and the goat can never be eaten (or wander) again.
	bool IsEaten() const;

	// [2026-10-28 T-015] Removes the goat from the world politely:
	// eaten (and therefore dead), hidden in game, actor collision
	// off, one "Goat eaten" log line. Idempotent — a second call is
	// a no-op — so the eat channel cannot double-count a meal by
	// completing twice.
	void Consume();

  protected:
	// [2026-10-28 T-017] Captures HomeLocation from the placed actor
	// location (unless SetHomeLocation already pinned it), picks the
	// first wander target, and logs one spawn line so a PIE log proves
	// the goat exists where placement meant it to.
	virtual void BeginPlay() override;

	// [2026-10-28 T-017] Steers straight at CurrentTarget at
	// WanderSpeed on home's Z plane, yawing the actor onto the flat
	// movement direction. Repicks the target on arrival (within 50cm)
	// or when RepickTimer exceeds 6s, whichever comes first.
	// [2026-10-28 T-015] Early-outs before any steering once the goat
	// is dead or eaten: a corpse holds still for the eat channel.
	virtual void Tick(float DeltaSeconds) override;

  private:
	// [2026-10-28 T-017] Picks CurrentTarget at a random point within
	// WanderRadius of HomeLocation (Z pinned to home) and resets
	// RepickTimer. The only writer of CurrentTarget besides
	// SetHomeLocation's reset-to-home.
	void PickNewTarget();

	// [2026-10-28 T-017] Wander centre; see header invariants.
	FVector HomeLocation = FVector::ZeroVector;

	// [2026-10-28 T-017] True once SetHomeLocation pinned the home, so
	// BeginPlay's location capture does not clobber an explicit pin.
	bool bHomeLocationOverridden = false;

	// [2026-10-28 T-017] How far from home this goat may roam, in cm.
	// EditAnywhere so a pasture can be sized per placement.
	UPROPERTY(EditAnywhere, Category = "Livestock|Wander")
	float WanderRadius = 600.0f;

	// [2026-10-28 T-017] Ground speed in cm/s — a calm graze-wander,
	// deliberately far below anything that could outrun the dragon.
	UPROPERTY(EditAnywhere, Category = "Livestock|Wander")
	float WanderSpeed = 60.0f;

	// [2026-10-28 T-017] Current wander destination, always on home's
	// Z plane and within WanderRadius of it.
	FVector CurrentTarget = FVector::ZeroVector;

	// [2026-10-28 T-017] Seconds since CurrentTarget was picked; the
	// 6s cap in Tick bounds how long a goat walks one heading.
	float RepickTimer = 0.0f;

	// [2026-10-28 T-015] One-way state flags behind IsDead/IsEaten:
	// set only by ApplyGoatDamage (lethal) and Consume, never
	// cleared — there is no resurrection verb for livestock.
	bool bDead = false;
	bool bEaten = false;
};
