// Copyright Under the Mountain contributors.
// [2026-10-28 T-014] ATrapActor implementation: arm an overlap sphere
// at BeginPlay, slow the first dragon that enters it, then destroy —
// traps are spent resources, never re-armable.

#include "Thief/TrapActor.h"

#include "Dragon/DragonCharacter.h"
#include "UnderTheMountain.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATrapActor::ATrapActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(150.0f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	// [2026-10-28 T-014] Flattened disc look; visual only, so the
	// trigger sphere stays the single interaction volume.
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Trigger);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.15f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Visual->SetStaticMesh(SphereMesh.Object);
	}
}

void ATrapActor::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ATrapActor::OnTriggerBeginOverlap);
}

void ATrapActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent,
									   AActor* OtherActor, UPrimitiveComponent* OtherComp,
									   int32 OtherBodyIndex, bool bFromSweep,
									   const FHitResult& SweepResult)
{
	if (bHasTriggered)
	{
		return;
	}

	// [2026-10-28 T-014] Only the dragon springs a thief trap; thieves
	// crossing their own work must not waste it.
	ADragonCharacter* Dragon = Cast<ADragonCharacter>(OtherActor);
	if (Dragon == nullptr)
	{
		return;
	}

	Dragon->ApplyTrapSlow(SlowFactor, SlowSeconds);
	bHasTriggered = true;
	UE_LOG(LogUnderTheMountain, Log, TEXT("Trap triggered: dragon slowed x%.2f for %.1fs"),
		   SlowFactor, SlowSeconds);
	Destroy();
}
