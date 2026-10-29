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

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGoatActor::AGoatActor()
{
	// [2026-10-28 T-017] Ticking is ON: Tick owns the wander (steering,
	// yaw, target repicks); nothing else moves this actor.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(SceneRoot);
	// [2026-10-28 T-017] Engine sphere stretched into a goat-ish
	// ellipsoid: long in X (the heading axis Tick yaws onto), narrower
	// and shorter across. Guarded on Succeeded() like the other
	// greybox actors so a missing engine asset degrades to an
	// invisible-but-wandering goat instead of a construction crash.
	Body->SetRelativeScale3D(FVector(1.4f, 0.9f, 0.9f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Body->SetStaticMesh(SphereMesh.Object);
	}

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Body);
	// [2026-10-28 T-017] Cube head perched at the body's front-top so
	// the goat's heading is readable from dragon altitude. The offset
	// and scale are relative to Body (its attachment parent).
	Head->SetRelativeLocation(FVector(55.0f, 0.0f, 35.0f));
	Head->SetRelativeScale3D(FVector(0.45f, 0.4f, 0.45f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Head->SetStaticMesh(CubeMesh.Object);
	}

	// [2026-10-28 T-017] NoCollision on both meshes: the dragon capsule
	// and world geometry own collision, and a collidable greybox
	// would only snag flight. [2026-10-28 T-015] The eat verb finds
	// goats by range from the pawn (UDragonCombatComponent::EatRange),
	// never by touching them, so NoCollision stays correct now that
	// goats are edible.
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		return;
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
