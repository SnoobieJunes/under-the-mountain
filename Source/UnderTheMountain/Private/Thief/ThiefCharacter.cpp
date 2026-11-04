// Copyright Under the Mountain contributors.
// [2026-10-28 T-014] AThiefCharacter implementation: movement plumbing
// only. The brain picks destinations; this actor feeds
// AddMovementInput toward them and keeps the movement component's
// speed in step with the brain's state.
// [2026-10-28 T-044] BeginPlay also puts the type's Meshy mesh on the
// inherited skeletal-mesh component (greybox sphere as fallback) and
// Tick drives its walk/run single-node anims from brain state + speed.

#include "Thief/ThiefCharacter.h"

#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
#include "Thief/ThiefRules.h"
#include "Dragon/DragonStatsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Thief/TrapActor.h"
#include "Treasure/TreasureSubsystem.h"
#include "UnderTheMountain.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AThiefCharacter::AThiefCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// [2026-11-03 T-066] Burglar-built capsule (r=30/hh=88, 60 cm wide):
	// thieves squeeze through the 80 cm mountain cracks that block the
	// dragon (capsule r=42). Dwarf/adventurer visuals fit inside
	// approximately; collision, not the mesh, is what the cracks test.
	GetCapsuleComponent()->InitCapsuleSize(30.0f, 88.0f);

	// [2026-10-28 T-044] Skeletal mesh starts hidden and the engine
	// sphere is the constructed body; BeginPlay swaps which one is
	// visible once ThiefData says whether this type has a Meshy mesh.
	// Both are visual only (NoCollision on the sphere) — the inherited
	// capsule keeps its default character collision for movement.
	GetMesh()->SetVisibility(false);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Body->SetStaticMesh(SphereMesh.Object);
	}

	// [2026-10-28 T-014] Walk toward the brain's destination facing
	// travel direction; BeginPlay sets the actual speeds from data.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

UThiefBrain* AThiefCharacter::GetBrain() const
{
	return Brain.Get();
}

void AThiefCharacter::BeginPlay()
{
	Super::BeginPlay();

	// [2026-11-01 T-018] Runtime-spawned thieves (intro gnome,
	// wave spawns) arrived with no controller, and CMC does not
	// simulate movement for a controller-less pawn - they stood
	// frozen at spawn while editor-placed thieves walked (proven
	// by the T-018/T-016 in-game logs: runtime spawns never left
	// their spawn point). Spawn the default AI controller when
	// missing; placed thieves already have one and skip this.
	if (Controller == nullptr)
	{
		SpawnDefaultController();
	}

	if (ThiefData == nullptr)
	{
		// [2026-10-28 T-014] Surfaced, not swallowed: without data the
		// thief has no speeds/HP/loot rules — say so once and stand
		// still (the brain no-ops on a null config too).
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Thief %s: no ThiefData assigned — brain will no-op"), *GetName());
	}

	UTreasureRegistry* Registry = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UTreasureSubsystem* TreasureSubsystem = World->GetSubsystem<UTreasureSubsystem>())
		{
			Registry = TreasureSubsystem->Registry();
		}
	}
	if (Registry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Thief %s: no treasure registry in world — brain will no-op"), *GetName());
	}

	Brain = NewObject<UThiefBrain>(this);
	Brain->Init(ThiefData.Get(), Registry, FName(GetName()));
	if (PartyRole != EThiefRole::None)
	{
		// [2026-11-04 T-133] Per-member dwarf role (shared party asset).
		Brain->SetRole(PartyRole);
	}
	Brain->SetExitLocation(ExitLocation);

	// [2026-10-28 T-044] Visible body: the type's Meshy mesh replaces
	// the greybox sphere as what the player sees. GetMesh() keeps
	// ACharacter's default attachment to the capsule; only its
	// relative location is set, to (0,0,-90), so the feet sit near
	// the capsule floor (capsule is r42/hh96, floor at pawn z -96).
	// Per-type height differences (dwarf ~125cm, burglar ~165cm,
	// adventurer ~180cm) ride on this one offset because the Meshy
	// origins are at the feet, so no per-type z is needed. BodyScale
	// moves to the mesh with it (the dwarf's data already carries
	// 0.9); the sphere keeps its component and is only hidden.
	if (ThiefData != nullptr && ThiefData->MeshAsset != nullptr)
	{
		GetMesh()->SetSkeletalMeshAsset(ThiefData->MeshAsset.Get());
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->SetVisibility(true);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeScale3D(ThiefData->BodyScale);
		Body->SetVisibility(false);
	}
	else
	{
		// [2026-10-28 T-044] Fallback, the pre-T-044 behaviour: the
		// sphere stays visible and BodyScale still applies to it. One
		// warning names the missing asset; a null ThiefData already
		// warned above, so this one is only for data without a mesh.
		if (ThiefData != nullptr)
		{
			Body->SetRelativeScale3D(ThiefData->BodyScale);
			UE_LOG(LogUnderTheMountain, Warning,
				   TEXT("Thief %s: no mesh asset on ThiefData — keeping greybox sphere"),
				   *GetName());
		}
	}

	if (ThiefData != nullptr)
	{
		GetCharacterMovement()->MaxWalkSpeed = ThiefData->SneakSpeed;
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s spawned at %s, exit %s"), *GetName(),
		   *GetActorLocation().ToString(), *ExitLocation.ToString());
	// [2026-11-01 T-054] Capture after the mesh setup above: the
	// flinch adds pitch over exactly this rotation.
	BaseMeshRotation = GetMesh()->GetRelativeRotation();
}

void AThiefCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Brain == nullptr)
	{
		return;
	}

	Brain->Tick(DeltaSeconds, GetActorLocation());

	// [2026-11-01 T-054] Death + hit presentation. Death is a
	// one-shot transition: stop locomotion and movement, then hand
	// the body to its PhysicsAsset (the Meshy *_rigged_PhysicsAsset
	// imported beside every thief mesh) for the ragdoll settle. A
	// corpse with no physics asset keeps its final pose instead -
	// logged once, never retried per tick.
	if (ShouldEnterRagdoll(Brain->State()) && !bRagdollEntered)
	{
		bRagdollEntered = true;
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->Stop();
		if (GetMesh()->GetPhysicsAsset() != nullptr)
		{
			GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			GetMesh()->SetSimulatePhysics(true);
			UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s died: ragdoll settle"), *GetName());
		}
		else
		{
			UE_LOG(LogUnderTheMountain, Warning,
				   TEXT("Thief %s died: no PhysicsAsset on mesh, holding death pose"), *GetName());
		}
		return;
	}
	if (bRagdollEntered)
	{
		return;
	}

	// [2026-11-01 T-054] Hit flinch: any HP drop starts a 0.28 s
	// pitch kick on the mesh (relative pitch added over the -90
	// rest offset set in BeginPlay). The brain owns HP; this actor
	// only mirrors the change it observes.
	const float BrainHP = Brain->HP();
	if (LastBrainHP >= 0.0f && BrainHP < LastBrainHP)
	{
		FlinchTimeRemaining = 0.28f;
	}
	LastBrainHP = BrainHP;
	if (FlinchTimeRemaining > 0.0f)
	{
		FlinchTimeRemaining = FMath::Max(0.0f, FlinchTimeRemaining - DeltaSeconds);
		const float Kick = FlinchPitchDegrees(FlinchTimeRemaining, 0.28f);
		FRotator FlinchRotation = BaseMeshRotation;
		FlinchRotation.Pitch += Kick;
		GetMesh()->SetRelativeRotation(FlinchRotation);
		if (FlinchTimeRemaining <= 0.0f)
		{
			GetMesh()->SetRelativeRotation(BaseMeshRotation);
		}
	}

	// [2026-10-28 T-014] Trap placement, timer-gated: docs/GAMEPLAY.md
	// 5.2 says the trapper "pauses to place traps", so the actor
	// attempts a placement every 5 s rather than every tick. The
	// brain owns the budget/state rules (trap-capable configs only,
	// Sneak state only, under MaxTraps); this actor owns the cadence
	// and spawning the ATrapActor the brain's success authorises.
	TrapPlaceTimer += DeltaSeconds;
	if (TrapPlaceTimer >= 5.0f)
	{
		TrapPlaceTimer = 0.0f;
		if (Brain->TryPlaceTrap())
		{
			if (UWorld* World = GetWorld())
			{
				const FVector TrapLocation = GetActorLocation() - FVector(0.0f, 0.0f, 80.0f);
				if (ATrapActor* Trap = World->SpawnActor<ATrapActor>(
						ATrapActor::StaticClass(), TrapLocation, FRotator::ZeroRotator))
				{
					Trap->SlowFactor = Brain->TrapSlowFactor();
					Trap->SlowSeconds = Brain->TrapSlowSeconds();
				}
			}
		}
	}

	// [2026-10-28 T-014] Debug cadence for PT-THIEF runs: one line
	// per thief per 10 s so a -game log shows the scenario playing
	// out (state, position, destination) without per-tick spam.
	DebugLogTimer += DeltaSeconds;
	if (DebugLogTimer >= 10.0f)
	{
		DebugLogTimer = 0.0f;
		UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s state=%d at %s dest=%s carrying=%d"),
			   *GetName(), static_cast<int32>(Brain->State()), *GetActorLocation().ToString(),
			   *Brain->CurrentDestination().ToString(), Brain->CarriedItems().Num());
	}

	if (Brain->ReachedExit())
	{
		if (!bLoggedExit)
		{
			bLoggedExit = true;
			UE_LOG(LogUnderTheMountain, Log, TEXT("Thief %s reached exit carrying %d items"),
				   *GetName(), Brain->CarriedItems().Num());
		}
		// [2026-10-28 T-014] Escaped: stop spending ticks. Removal /
		// notoriety bookkeeping for a successful theft is a later
		// system's job; the actor stands at the exit until then.
		SetActorTickEnabled(false);
		return;
	}

	// [2026-10-28 T-014] Speed follows state: fleeing and scared are
	// runs, everything else is the sneak pace.
	if (ThiefData != nullptr)
	{
		const bool bRunning =
			Brain->State() == EThiefState::Flee || Brain->State() == EThiefState::Scared;
		// [2026-11-04 T-132] Star-gear speed multiplier from the brain's spawn snapshot.
		const float GearMult = Brain->GearSpeedMultiplier();
		GetCharacterMovement()->MaxWalkSpeed =
			(bRunning ? ThiefData->FleeSpeed : ThiefData->SneakSpeed) * GearMult;
	}

	// [2026-10-28 T-044] Single-node locomotion follows brain state +
	// actual speed: still (2D speed <= 20 cm/s, e.g. mid-Grab) holds
	// bind pose via Stop(), Flee/Scared run, everything else walks.
	// PlayAnimation/Stop fire only on a change (LastPlayedAnim +
	// bAnimPlaying latch the current choice) so a looping clip is
	// never restarted every tick. Null anims skip silently to bind
	// pose — the anims are data and may be unset for a type. The
	// Meshy clips live on their own Skeleton assets, so playback
	// depends on skeleton compatibility being set on the assets; if
	// the engine rejects an anim the mesh simply holds bind pose,
	// and the latch still records the attempt so Tick does not retry
	// (and re-log) it every frame.
	if (ThiefData != nullptr && ThiefData->MeshAsset != nullptr)
	{
		UAnimSequence* DesiredAnim = nullptr;
		if (GetVelocity().Size2D() > 20.0f)
		{
			const bool bRunningAnim =
				Brain->State() == EThiefState::Flee || Brain->State() == EThiefState::Scared;
			DesiredAnim = bRunningAnim ? ThiefData->RunAnim.Get() : ThiefData->WalkAnim.Get();
		}
		if (DesiredAnim == nullptr)
		{
			if (bAnimPlaying)
			{
				GetMesh()->Stop();
				bAnimPlaying = false;
				LastPlayedAnim = nullptr;
			}
		}
		else if (DesiredAnim != LastPlayedAnim || !bAnimPlaying)
		{
			GetMesh()->PlayAnimation(DesiredAnim, true);
			LastPlayedAnim = DesiredAnim;
			bAnimPlaying = true;
		}
	}

	// [2026-11-04 T-089] Waypoint-route steering (replaces the T-066
	// portal/mouth constants and the T-012 chamber centres before them):
	// the route is injected (RouteWaypoints, authored per spawn/level),
	// FThiefRules::SteerTarget consumes reached waypoints in order, and
	// an empty route steers straight at the brain destination. No map
	// coordinates live in this actor any more. Editor-side NavMesh
	// volumes can generate these lists later; the seam does not change.
	const FVector MyLocation = GetActorLocation();
	const FVector BrainDestination = Brain->CurrentDestination();
	if (!bHasRouteDestination || !BrainDestination.Equals(LastRouteDestination, 1.0f))
	{
		LastRouteDestination = BrainDestination;
		bHasRouteDestination = true;
		RouteWaypointIndex = 0;
	}
	// [2026-11-04 T-074] Armed thieves fight the dragon they can reach:
	// the brain owns tell/cadence/damage, this actor only supplies the
	// two locations and the dragon's Stats seam (found on the player pawn).
	if (UWorld* World = GetWorld())
	{
		if (ACharacter* DragonPawn = UGameplayStatics::GetPlayerCharacter(World, 0))
		{
			if (UDragonStatsComponent* DragonStats =
					DragonPawn->FindComponentByClass<UDragonStatsComponent>())
			{
				Brain->TickDragonAttack(DeltaSeconds, MyLocation, DragonPawn->GetActorLocation(),
										DragonStats);
			}
		}
	}
	const FVector SteerTarget = FThiefRules::SteerTarget(RouteWaypoints, MyLocation, BrainDestination,
														 Brain->ArriveRadius, RouteWaypointIndex);
	const FVector ToDestination = SteerTarget - MyLocation;
	if (ToDestination.Size() > 10.0f)
	{
		FVector Direction = ToDestination;
		Direction.Z = 0.0f;
		if (!Direction.IsNearlyZero())
		{
			AddMovementInput(Direction.GetSafeNormal(), 1.0f);
		}
	}
}

void AThiefCharacter::OnRoarHeard()
{
	if (Brain != nullptr)
	{
		Brain->OnRoar(GetActorLocation());
	}
}

bool AThiefCharacter::ApplyThiefDamage(float Damage)
{
	return Brain != nullptr && Brain->ApplyDamage(Damage, GetActorLocation());
}

bool AThiefCharacter::ShouldEnterRagdoll(EThiefState State)
{
	// [2026-11-01 T-054] Only death ragdolls; every living state
	// (including Scared) keeps steering and single-node locomotion.
	return State == EThiefState::Dead;
}

float AThiefCharacter::FlinchPitchDegrees(float TimeRemaining, float Duration)
{
	// [2026-11-01 T-054] Sine bump over the flinch window: zero at
	// both ends (no pose snap), 14 degrees at the middle. Outside
	// the window - or with a broken duration - no kick at all.
	if (Duration <= 0.0f || TimeRemaining <= 0.0f || TimeRemaining >= Duration)
	{
		return 0.0f;
	}
	const float Phase = 1.0f - (TimeRemaining / Duration);
	return 14.0f * FMath::Sin(Phase * PI);
}
