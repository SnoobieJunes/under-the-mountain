// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] AScentRibbonActor implementation: stretches one
// engine cube between two points. All scent meaning (how wide, how
// opaque) is decided by FScentMath/UScentComponent; this actor only
// knows endpoint geometry and one material scalar.

#include "Scent/ScentRibbonActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

AScentRibbonActor::AScentRibbonActor()
{
	// [2026-10-28 T-013] No tick: UScentComponent drives endpoint and
	// opacity updates each frame.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	// [2026-10-28 T-013] A smell blocks nothing and is hit by nothing.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// [2026-10-28 T-013] Engine cube (100 cm) as the ribbon body; the
	// /100 terms in SetEndpoints assume exactly this unit size.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	// [2026-10-28 T-013] Scent-gold material authored by
	// Scripts/create_treasures.py; absent in a fresh clone until that
	// script runs, in which case the ribbon renders in the cube's
	// default material — geometry (and its value-scaled width) still
	// reads, colour is the only thing lost.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ScentMaterial(
		TEXT("/Game/Gameplay/Treasures/M_ScentGold.M_ScentGold"));
	if (ScentMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ScentMaterial.Object);
	}
}

void AScentRibbonActor::BeginPlay()
{
	Super::BeginPlay();

	// [2026-10-28 T-013] One dynamic instance per ribbon so opacity is
	// per-treasure. A mesh with no material is valid (greybox), so a
	// null instance just means SetOpacityFactor stores without
	// applying — guarded there, not treated as an error here.
	if (Mesh != nullptr && Mesh->GetMaterial(0) != nullptr)
	{
		DynamicMaterial = Mesh->CreateDynamicMaterialInstance(0);
		if (DynamicMaterial != nullptr)
		{
			DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), VisibleOpacity);
		}
	}
}

void AScentRibbonActor::SetEndpoints(const FVector& From, const FVector& To, float WidthCm)
{
	const FVector Delta = To - From;
	const float LengthCm = Delta.Size();
	const float WidthScale = WidthCm / 100.0f;

	SetActorLocation((From + To) * 0.5f);
	if (LengthCm <= KINDA_SMALL_NUMBER)
	{
		// [2026-10-28 T-013] Degenerate ribbon (dragon standing on the
		// treasure): zero length, keep the last rotation — MakeFromX
		// on a zero vector would be meaningless.
		SetActorScale3D(FVector(0.0f, WidthScale, WidthScale));
		return;
	}

	SetActorRotation(FRotationMatrix::MakeFromX(Delta / LengthCm).Rotator());
	SetActorScale3D(FVector(LengthCm / 100.0f, WidthScale, WidthScale));
}

void AScentRibbonActor::SetOpacityFactor(float Opacity)
{
	VisibleOpacity = Opacity;
	if (DynamicMaterial != nullptr)
	{
		DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), VisibleOpacity);
	}
}
