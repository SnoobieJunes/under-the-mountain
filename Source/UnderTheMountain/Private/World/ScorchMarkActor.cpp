// Copyright Under the Mountain contributors.
// [2026-11-01 T-055] AScorchMarkActor implementation: an engine
// plane wearing M_Scorch (authored by Scripts/
// create_combat_assets.py), scaled to the spawner's diameter.

#include "World/ScorchMarkActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AScorchMarkActor::AScorchMarkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	SetRootComponent(Disc);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// [2026-11-01 T-055] The engine plane is 100x100 cm facing +Z;
	// at scale 1 the disc is 1 m across and SetDiameter rescales.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		Disc->SetStaticMesh(PlaneMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ScorchMaterial(
		TEXT("/Game/Dragon/M_Scorch.M_Scorch"));
	if (ScorchMaterial.Succeeded())
	{
		Disc->SetMaterial(0, ScorchMaterial.Object);
	}
}

void AScorchMarkActor::SetDiameter(float DiameterCm)
{
	// [2026-11-01 T-055] 100 cm plane -> scale is diameter in
	// metres. Z scale stays 1 (flat); the spawner offsets the disc
	// a few cm above the ground to avoid z-fighting.
	const float Scale = FMath::Max(0.1f, DiameterCm / 100.0f);
	Disc->SetRelativeScale3D(FVector(Scale, Scale, 1.0f));
}
