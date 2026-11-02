// Copyright Under the Mountain contributors.
// [2026-10-28 T-017] AGoatActor implementation: greybox construction
// plus the flat-pasture wander. Everything visual here (engine sphere
// body, cube head) is a stand-in for the approval-gated Meshy goat in
// docs/WORLD-ASSETS.md (LIV-01).
// [2026-10-28 T-015] HP/death/eaten added below the wander accessors:
// damage can stop the wander for good, and the dragon's eat channel
// finishes the goat off via Consume().

#include "Livestock/GoatActor.h"
#include "UnderTheMountain.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// [2026-11-01 T-054] How long the authored side-fall takes, in
// seconds: fast enough to read as a fall, slow enough to see.
constexpr float DeathFallSeconds = 0.6f;
} // namespace

AGoatActor::AGoatActor()
{
	// [2026-10-28 T-017] Ticking is ON: Tick owns the wander (steering,
	// yaw, target repicks); nothing else moves this actor.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// [2026-11-01 T-054] The Meshy goat (liv-goat_rigged + its walk
	// clip) replaces the T-017 sphere/cube greybox. NoCollision as
	// before: the dragon finds goats by eat range, never by touch.
	// Loads are guarded so a missing asset degrades to an invisible
	// wandering goat, never a construction crash.
	GoatMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GoatMesh"));
	GoatMesh->SetupAttachment(SceneRoot);
	GoatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GoatMeshAsset(
		TEXT("/Game/Meshy/World/liv-goat_rigged.liv-goat_rigged"));
	if (GoatMeshAsset.Succeeded())
	{
		GoatMesh->SetSkeletalMeshAsset(GoatMeshAsset.Object);
	}
	GoatMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GoatWalkAnim(
		TEXT("/Game/Meshy/World/liv-goat_rigged_Anim.liv-goat_rigged_Anim"));
	if (GoatWalkAnim.Succeeded())
	{
		WalkAnim = GoatWalkAnim.Object;
	}
}

float AGoatActor::DeathFallRollDegrees(float Alpha)
{
	// [2026-11-01 T-054] Ease-out cubic: fast tip, soft settle.
	const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float Eased = 1.0f - FMath::Pow(1.0f - T, 3.0f);
	return -88.0f * Eased;
}

void AGoatActor::SetHomeLocation(const FVector& NewHomeLocation)
{
	// [2026-10-28 T-017] Pin home explicitly and restart the wander
	// from it: CurrentTarget reset means the next Tick repicks within
	// the new pasture instead of walking back to the old one. The
	// override flag stops a later BeginPlay capture clobbering this.
	HomeLocation = NewHomeLocation;
	bHomeLocationOverridden = true;
	CurrentTarget = HomeLocation;
	RepickTimer = 0.0f;
}

FVector AGoatActor::GetHomeLocation() const
{
	return HomeLocation;
}

bool AGoatActor::ApplyGoatDamage(float Damage)
{
	// [2026-10-28 T-015] A corpse (or an eaten goat) takes no more
	// damage and reports no new death: the true return is reserved
	// for the single killing blow, mirroring UThiefBrain.
	if (bDead || bEaten)
	{
		return false;
	}

	// [2026-10-28 T-015] Negative damage is not healing (nothing
	// heals a goat) and HP never goes below 0, so callers can pass
	// any DPS-scaled value without pre-clamping.
	HP = FMath::Max(0.0f, HP - FMath::Max(0.0f, Damage));
	if (HP > 0.0f)
	{
		return false;
	}

	// [2026-10-28 T-015] Death is a flag, not a teardown: Tick's
	// early-out stops the wander from the next frame, and the body
	// stays visible where it fell so the dragon can eat it. The log
	// line is the in-world proof a kill happened where expected.
	bDead = true;
	UE_LOG(LogUnderTheMountain, Log, TEXT("Goat down at %s"), *GetActorLocation().ToString());
	return true;
}

bool AGoatActor::IsDead() const
{
	return bDead;
}

bool AGoatActor::IsEaten() const
{
	return bEaten;
}

void AGoatActor::Consume()
{
	// [2026-10-28 T-015] Idempotent on purpose: the eat channel
	// clears its own pending pointer at completion, but a defensive
	// second call must not re-log or re-hide an already-gone goat.
	if (bEaten)
	{
		return;
	}

	// [2026-10-28 T-015] Eaten implies dead (the eat channel accepts
	// a live goat, so Consume cannot assume the killing blow landed
	// first). The actor is hidden rather than destroyed: ownership
	// and lifespan stay with whoever placed/spawned it, and the eat
	// channel holds no opinion about actor teardown.
	bEaten = true;
	bDead = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	UE_LOG(LogUnderTheMountain, Log, TEXT("Goat eaten at %s"), *GetActorLocation().ToString());
}

void AGoatActor::BeginPlay()
{
	Super::BeginPlay();

	// [2026-10-28 T-017] Home is where placement put the actor:
	// placement sets the location before BeginPlay runs, so capturing
	// here needs no extra wiring. An explicit SetHomeLocation pin
	// wins over the capture.
	if (!bHomeLocationOverridden)
	{
		HomeLocation = GetActorLocation();
	}
	PickNewTarget();

	// [2026-10-28 T-017] One line per goat so a PIE log proves the
	// pasture is stocked where placement meant it to be.
	UE_LOG(LogUnderTheMountain, Log, TEXT("Goat spawned at %s"),
		   *GetActorLocation().ToString());
}

void AGoatActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// [2026-10-28 T-015] Dead goats do not wander, and eaten goats
	// do not exist as far as the pasture is concerned (Consume also
	// sets bDead, so either flag alone would do; both are checked so
	// the intent survives if that pairing ever changes). Returning
	// before the steering math keeps a corpse exactly where it fell
	// for the dragon's eat channel.
	if (bDead || bEaten)
	{
		// [2026-11-01 T-054] The authored side-fall plays here, in
		// the dead branch: roll the mesh over DeathFallSeconds and
		// hold. The corpse does not translate - where it fell is
		// where it is eaten from (the T-015 contract above).
		if (!bEaten && DeathFallAlpha < 1.0f && GoatMesh != nullptr)
		{
			DeathFallAlpha = FMath::Min(1.0f, DeathFallAlpha + DeltaSeconds / DeathFallSeconds);
			FRotator FallRotation = GoatMesh->GetRelativeRotation();
			FallRotation.Roll = DeathFallRollDegrees(DeathFallAlpha);
			GoatMesh->SetRelativeRotation(FallRotation);
			if (DeathFallAlpha >= 1.0f)
			{
				GoatMesh->Stop();
			}
		}
		return;
	}

	// [2026-11-01 T-054] Walk clip while alive (the wander below
	// always moves or repicks, so a living goat is a walking goat);
	// started once via the node's current-asset check.
	if (WalkAnim != nullptr && GoatMesh != nullptr)
	{
		UAnimSingleNodeInstance* Node = GoatMesh->GetSingleNodeInstance();
		if (Node == nullptr || Node->GetAnimationAsset() != WalkAnim)
		{
			GoatMesh->PlayAnimation(WalkAnim, true);
		}
	}

	RepickTimer += DeltaSeconds;

	// [2026-10-28 T-017] All wander math is flat: positions and target
	// are projected onto home's Z plane so the goat grazes level
	// ground and never drifts up or down a slope it cannot see.
	FVector Location = GetActorLocation();
	Location.Z = HomeLocation.Z;
	FVector ToTarget = CurrentTarget - Location;
	ToTarget.Z = 0.0f;

	// [2026-10-28 T-017] Repick on arrival (within 50cm — close enough
	// that exact-arrival jitter never shows) or after 6s on one
	// heading, then re-derive the steering vector for this same tick
	// so the goat never stands idle for a frame.
	if (ToTarget.Size() <= 50.0f || RepickTimer > 6.0f)
	{
		PickNewTarget();
		ToTarget = CurrentTarget - Location;
		ToTarget.Z = 0.0f;
	}

	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// [2026-10-28 T-017] Move at WanderSpeed, clamped to the remaining
	// distance so the goat lands on the target instead of orbiting
	// past it at low frame rates. Direct SetActorRotation onto the
	// flat direction: at graze speeds a hard yaw reads fine and keeps
	// the greybox honest (no smoothing state to go stale).
	const FVector Direction = ToTarget / Distance;
	const float Step = FMath::Min(WanderSpeed * DeltaSeconds, Distance);
	FVector NewLocation = Location + Direction * Step;
	NewLocation.Z = HomeLocation.Z;
	SetActorLocation(NewLocation);
	SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
}

void AGoatActor::PickNewTarget()
{
	// [2026-10-28 T-017] Uniform-ish random point in the home disc via
	// polar coordinates (angle + distance from FMath::RandRange), so
	// the target is guaranteed within WanderRadius — a square offset
	// would overshoot the radius at its corners. No navmesh: the
	// pasture is open, so any in-disc point is reachable by the
	// straight steering in Tick. Z is pinned to home's plane.
	const float AngleRadians = FMath::DegreesToRadians(FMath::RandRange(0.0f, 360.0f));
	const float Distance = FMath::RandRange(0.0f, WanderRadius);
	CurrentTarget = HomeLocation + FVector(FMath::Cos(AngleRadians) * Distance,
										   FMath::Sin(AngleRadians) * Distance, 0.0f);
	CurrentTarget.Z = HomeLocation.Z;
	RepickTimer = 0.0f;
}
