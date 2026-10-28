// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] ATrueSightMarkerActor implementation: a hidden
// sphere UScentComponent parks on registered treasure locations and
// reveals only while true-sight is active.

#include "Scent/TrueSightMarkerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATrueSightMarkerActor::ATrueSightMarkerActor()
{
	// [2026-10-28 T-013] No tick: position and visibility are driven
	// by UScentComponent each frame.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	// [2026-10-28 T-013] A marker is pure display; it must never block
	// the dragon or a thief standing on the treasure it marks.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	// [2026-10-28 T-013] Seeing-stone teal, depth-test disabled by
	// Scripts/create_treasures.py so the marker reads through rock
	// (the whole point of true-sight). Absent material = default grey
	// sphere, still depth-tested — colour/depth are lost together and
	// the absence is visible, not silent.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MarkerMaterial(
		TEXT("/Game/Gameplay/Treasures/M_TrueSight.M_TrueSight"));
	if (MarkerMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, MarkerMaterial.Object);
	}

	// [2026-10-28 T-013] Starts hidden so a freshly spawned marker can
	// never flash a treasure position before true-sight is on.
	SetActorHiddenInGame(true);
}

void ATrueSightMarkerActor::SetMarkerVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
}
