// Copyright Under the Mountain contributors.
// [2026-11-01 T-053] AVillagerActor implementation: flat steering
// (the goat's proven pattern), single-node walk/run playback, and
// all decisions read from UVillagerBrain every tick so a dragon
// arriving mid-wander is reacted to the same frame it matters.

#include "NPC/VillagerActor.h"
#include "UnderTheMountain.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/VillagerBrain.h"
#include "UObject/ConstructorHelpers.h"

AVillagerActor::AVillagerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Brain = CreateDefaultSubobject<UVillagerBrain>(TEXT("Brain"));

	// [2026-11-01 T-053] Meshy burglar rig as the civilian visual
	// (see the header for the decision). Guarded like every other
	// ConstructorHelpers load in the project: a missing asset
	// degrades to an invisible villager, never a construction
	// crash, and BeginPlay logs the mesh state so it is visible.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> VillagerMesh(
		TEXT("/Game/Meshy/Thieves/burglar_rigged.burglar_rigged"));
	if (VillagerMesh.Succeeded())
	{
		Mesh->SetSkeletalMeshAsset(VillagerMesh.Object);
	}
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// [2026-11-01 T-053] Locomotion clips load HERE, in the
	// constructor: FObjectFinder is constructor-only, and the
	// first version kept them as statics inside Tick, which is a
	// fatal error the first time a villager ticks in a real game
	// (caught by the T-053 -game run, invisible to headless tests
	// where no villager ever ticks).
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimFinder(
		TEXT("/Game/Meshy/Thieves/burglar_walking_Anim.burglar_walking_Anim"));
	if (WalkAnimFinder.Succeeded())
	{
		WalkAnim = WalkAnimFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAnimFinder(
		TEXT("/Game/Meshy/Thieves/burglar_running_Anim.burglar_running_Anim"));
	if (RunAnimFinder.Succeeded())
	{
		RunAnim = RunAnimFinder.Object;
	}
}

void AVillagerActor::BeginPlay()
{
	Super::BeginPlay();
	if (!bHomeLocationOverridden)
	{
		HomeLocation = GetActorLocation();
	}
	Destination = HomeLocation;
	PickNewBehaviour();
	UE_LOG(LogUnderTheMountain, Log, TEXT("Villager spawned at %s (mesh %s)"),
		   *GetActorLocation().ToString(),
		   Mesh->GetSkeletalMeshAsset() ? TEXT("set") : TEXT("MISSING"));
}

void AVillagerActor::SetHomeLocation(const FVector& NewHomeLocation)
{
	HomeLocation = NewHomeLocation;
	bHomeLocationOverridden = true;
}

FVector AVillagerActor::GetHomeLocation() const
{
	return HomeLocation;
}

EVillagerState AVillagerActor::GetState() const
{
	return State;
}

void AVillagerActor::PickNewBehaviour()
{
	// [2026-11-01 T-053] Coin flip between standing a while and
	// strolling to a new point near home; 3-7 s per decision keeps
	// a street visibly alive without constant motion.
	DecisionTimer = FMath::FRandRange(3.0f, 7.0f);
	if (FMath::FRand() < 0.45f)
	{
		State = EVillagerState::Idle;
		Destination = GetActorLocation();
	}
	else
	{
		State = EVillagerState::Wander;
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::FRandRange(0.2f, 1.0f) * WanderRadius;
		Destination = HomeLocation + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
	}
}

void AVillagerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Brain == nullptr)
	{
		return;
	}

	// [2026-11-01 T-053] Threat first, every tick: the dragon's
	// distance decides Flee before any wander timer gets a say,
	// so a landing dragon empties the street immediately.
	EVillagerState Desired = EVillagerState::Wander;
	if (AActor* Dragon = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const float Dist = FVector::Dist(GetActorLocation(), Dragon->GetActorLocation());
		Desired = UVillagerBrain::StateForThreat(Dist, Brain->FleeRadius);
		if (Desired == EVillagerState::Flee)
		{
			State = EVillagerState::Flee;
			Destination = GetActorLocation() +
						  UVillagerBrain::FleeDirection(GetActorLocation(), Dragon->GetActorLocation()) *
							  500.0f;
		}
	}
	if (State == EVillagerState::Flee && Desired != EVillagerState::Flee)
	{
		// [2026-11-01 T-053] Threat gone: back to daily life with a
		// fresh decision instead of resuming a stale flee target.
		PickNewBehaviour();
	}
	else if (State != EVillagerState::Flee)
	{
		DecisionTimer -= DeltaSeconds;
		if (DecisionTimer <= 0.0f)
		{
			PickNewBehaviour();
		}
	}

	// [2026-11-01 T-053] Flat steer at the brain's speed, yaw onto
	// heading; arrival (or Idle's zero speed) just stands. The
	// walk/run clip follows the state, switched only on change of
	// the desired clip so PlayAnimation is not restarted per tick.
	const float Speed = UVillagerBrain::SpeedForState(State, Brain->WalkSpeed, Brain->FleeSpeed);
	FVector ToDest = Destination - GetActorLocation();
	ToDest.Z = 0.0f;
	if (Speed > 0.0f && ToDest.Size() > 25.0f)
	{
		const FVector Dir = ToDest.GetSafeNormal();
		AddActorWorldOffset(Dir * Speed * DeltaSeconds, false);
		SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
	}
	else if (State == EVillagerState::Wander)
	{
		PickNewBehaviour();
	}

	UAnimSequence* DesiredAnim = State == EVillagerState::Flee ? RunAnim.Get()
								 : State == EVillagerState::Wander
									 ? WalkAnim.Get()
									 : nullptr;
	// [2026-11-01 T-053] Clip follows state, restarted only when
	// the clip actually changes (PlayAnimation on the playing clip
	// would restart it every tick - the thief's LastPlayedAnim
	// lesson, applied via the node's current asset instead).
	if (DesiredAnim != nullptr)
	{
		UAnimSingleNodeInstance* Node = Mesh->GetSingleNodeInstance();
		if (Node == nullptr || Node->GetAnimationAsset() != DesiredAnim)
		{
			Mesh->PlayAnimation(DesiredAnim, true);
		}
	}
	else if (Mesh->GetSingleNodeInstance() != nullptr)
	{
		Mesh->Stop();
	}
}
