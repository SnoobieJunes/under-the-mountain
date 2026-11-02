// Copyright Under the Mountain contributors.
// [2026-11-01 T-055] ABurnablePropActor implementation: exposure
// accumulation, the burn clock, and the mesh/FX swaps. The combat
// seam feeds exposure; this actor owns what burning means.

#include "World/BurnablePropActor.h"
#include "UnderTheMountain.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "World/ScorchMarkActor.h"

ABurnablePropActor::ABurnablePropActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// [2026-11-01 T-055] Burning presentation: the breath's own
	// Niagara system at small scale on the prop (pooled component,
	// activated on ignition only). No light per prop - a market
	// row of burning lights would cost more than the fire does.
	FireFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireFX"));
	FireFX->SetupAttachment(Mesh);
	FireFX->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	FireFX->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
	FireFX->SetAutoActivate(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FireSystem(
		TEXT("/Game/Dragon/NS_FireBreath.NS_FireBreath"));
	if (FireSystem.Succeeded())
	{
		FireFX->SetAsset(FireSystem.Object);
	}
}

EBurnState ABurnablePropActor::StateAfterExposure(EBurnState Current, float Exposure, float Threshold)
{
	// [2026-11-01 T-055] Forward-only: Intact ignites at the
	// threshold; Burning/Burnt are terminal for this rule (the
	// burn clock, not exposure, ends Burning).
	if (Current == EBurnState::Intact && Exposure >= Threshold)
	{
		return EBurnState::Burning;
	}
	return Current;
}

void ABurnablePropActor::BeginPlay()
{
	Super::BeginPlay();
	if (IntactMesh != nullptr)
	{
		Mesh->SetStaticMesh(IntactMesh);
	}
}

EBurnState ABurnablePropActor::AddFireExposure(float InExposureSeconds)
{
	if (BurnState != EBurnState::Intact)
	{
		return BurnState;
	}
	ExposureSeconds += FMath::Max(0.0f, InExposureSeconds);
	const EBurnState Next = StateAfterExposure(BurnState, ExposureSeconds, IgniteExposureSeconds);
	if (Next == EBurnState::Burning)
	{
		EnterBurning();
	}
	return BurnState;
}

void ABurnablePropActor::IgniteNow()
{
	if (BurnState == EBurnState::Intact)
	{
		EnterBurning();
	}
}

EBurnState ABurnablePropActor::GetBurnState() const
{
	return BurnState;
}

float ABurnablePropActor::GetExposure() const
{
	return ExposureSeconds;
}

void ABurnablePropActor::EnterBurning()
{
	BurnState = EBurnState::Burning;
	BurnTimeRemaining = BurnSeconds;
	if (FireFX != nullptr)
	{
		FireFX->Activate(true);
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Burnable %s ignited"), *GetName());
}

void ABurnablePropActor::EnterBurnt()
{
	BurnState = EBurnState::Burnt;
	if (FireFX != nullptr)
	{
		FireFX->Deactivate();
	}
	if (RubbleMesh != nullptr)
	{
		Mesh->SetStaticMesh(RubbleMesh);
	}
	// [2026-11-01 T-055] Every burnt prop leaves its mark: a
	// scorch disc a little wider than the prop, 4 cm over the
	// ground it stood on (the actor's own Z is its ground -
	// placement grounds these actors).
	if (UWorld* World = GetWorld())
	{
		const FVector Loc = GetActorLocation();
		if (AScorchMarkActor* Scorch = World->SpawnActor<AScorchMarkActor>(
				FVector(Loc.X, Loc.Y, Loc.Z + 4.0f), FRotator::ZeroRotator))
		{
			Scorch->SetDiameter(420.0f);
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Burnable %s burnt out (rubble)"), *GetName());
}

void ABurnablePropActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (BurnState != EBurnState::Burning)
	{
		return;
	}
	BurnTimeRemaining -= DeltaSeconds;
	if (BurnTimeRemaining <= 0.0f)
	{
		EnterBurnt();
	}
}
