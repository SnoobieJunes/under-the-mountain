// Copyright Under the Mountain contributors.
// [2026-10-28 T-031] AScentRibbonActor implementation: a Niagara
// component swept nose<->treasure. The NS_ScentRibbon emitter spawns
// location-based particles (they stay where they were spawned), so
// the ribbon the renderer draws through them is the swept path.
// Replaces the T-013 stretched-cube placeholder wholesale — same
// SetEndpoints/SetOpacityFactor contract for UScentComponent.

#include "Scent/ScentRibbonActor.h"
#include "UnderTheMountain.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AScentRibbonActor::AScentRibbonActor()
{
	// [2026-10-28 T-031] Ticks because the emitter sweep is the
	// ribbon; without the sweep there is no line to render.
	PrimaryActorTick.bCanEverTick = true;

	FX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FX"));
	SetRootComponent(FX);
	// [2026-10-28 T-031] A smell blocks nothing and is hit by
	// nothing; Niagara components do not collide in any case.
	FX->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SystemAsset(
		TEXT("/Game/Gameplay/Treasures/NS_ScentRibbon.NS_ScentRibbon"));
	if (SystemAsset.Succeeded())
	{
		RibbonSystem = SystemAsset.Object;
		FX->SetAsset(RibbonSystem);
	}
}

void AScentRibbonActor::BeginPlay()
{
	Super::BeginPlay();

	if (RibbonSystem == nullptr)
	{
		// [2026-10-28 T-031] Surfaced, never swallowed: without the
		// authored system this ribbon renders nothing, and the scent
		// system would otherwise fail invisibly.
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("ScentRibbonActor: NS_ScentRibbon not found; ribbon will not render."));
		return;
	}
	FX->SetVariableFloat(FName(TEXT("User.RibbonWidth")), RibbonWidthCm);
	FX->SetVariableFloat(FName(TEXT("User.RibbonOpacity")), VisibleOpacity);
	FX->Activate(true);
}

void AScentRibbonActor::SetEndpoints(const FVector& From, const FVector& To, float WidthCm)
{
	SegmentFrom = From;
	SegmentTo = To;
	RibbonWidthCm = WidthCm;
	if (FX != nullptr && RibbonSystem != nullptr)
	{
		FX->SetVariableFloat(FName(TEXT("User.RibbonWidth")), RibbonWidthCm);
	}
}

void AScentRibbonActor::SetOpacityFactor(float Opacity)
{
	VisibleOpacity = Opacity;
	if (FX != nullptr && RibbonSystem != nullptr)
	{
		FX->SetVariableFloat(FName(TEXT("User.RibbonOpacity")), VisibleOpacity);
	}
}

void AScentRibbonActor::SetAppearanceFactors(float DensityScale, float SheenScale,
											 const FLinearColor& SheenColor)
{
	// [2026-11-04 T-127] Stored even without a system so tests/inspection
	// [2026-11-04 T-127] see the intended retune on a headless ribbon.
	RibbonDensityScale = DensityScale;
	RibbonSheenScale = SheenScale;
	if (FX != nullptr && RibbonSystem != nullptr)
	{
		FX->SetVariableFloat(FName(TEXT("User.RibbonDensity")), RibbonDensityScale);
		FX->SetVariableFloat(FName(TEXT("User.RibbonSheen")), RibbonSheenScale);
		FX->SetVariableLinearColor(FName(TEXT("User.RibbonSheenColor")), SheenColor);
	}
}

void AScentRibbonActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FX == nullptr || RibbonSystem == nullptr)
	{
		return;
	}

	const float LengthCm = FVector::Dist(SegmentFrom, SegmentTo);
	if (LengthCm <= KINDA_SMALL_NUMBER)
	{
		FX->SetWorldLocation(SegmentFrom);
		return;
	}

	// [2026-10-28 T-040] Ping-pong sweep at the per-segment effective
	// speed, not raw SweepSpeed: particles live only
	// ParticleLifetimeSeconds, so the live trail spans speed x
	// lifetime and a fixed 6000 cm/s would cover just 3600 cm of a
	// 19000 cm segment. EffectiveSweepSpeed makes the emitter
	// traverse the whole segment within one lifetime on long
	// segments while short ones keep the base speed and density.
	SweepPhase += SweepDirection *
				  (EffectiveSweepSpeed(LengthCm, SweepSpeed, ParticleLifetimeSeconds) *
				   DeltaSeconds / LengthCm);
	if (SweepPhase >= 1.0f)
	{
		SweepPhase = 1.0f;
		SweepDirection = -1.0f;
	}
	else if (SweepPhase <= 0.0f)
	{
		SweepPhase = 0.0f;
		SweepDirection = 1.0f;
	}
	FX->SetWorldLocation(FMath::Lerp(SegmentFrom, SegmentTo, SweepPhase));
	// [2026-11-04 T-127] End-on fade: recompute effective opacity from the
	// [2026-11-04 T-127] live camera each tick (cheap: one dot product).
	// [2026-11-04 T-127] No camera (headless) leaves stored opacity alone.
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager != nullptr)
			{
				const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
				const float Factor = EndOnOpacityFactor(SegmentTo - SegmentFrom, FX->GetComponentLocation() - CamLoc);
				FX->SetVariableFloat(FName(TEXT("User.RibbonOpacity")), VisibleOpacity * Factor);
			}
		}
	}
}
