// Copyright Under the Mountain contributors.
// Purpose: a flammable world prop (T-055) - crates, barrels, carts
// and fences that catch fire under the dragon's breath, burn for a
// few seconds, and stay burnt (rubble mesh) for the session. The
// state machine is the testable seam; meshes and FX are data the
// placement script assigns.
// Created 2026-11-01 (T-055).
// Invariants: state only moves forward (Intact -> Burning ->
// Burnt); exposure accumulates only while the breath is actually
// on the prop (the combat seam calls AddFireExposure per tick in
// the cone) and never decays - a half-burnt crate stays primed.
// Burnt is permanent for the session: no verb un-burns a prop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BurnablePropActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UNiagaraComponent;

// [2026-11-01 T-055] Plain enum class, mirroring the project's
// pure-logic types (EThiefState, EVillagerState).
enum class EBurnState : uint8
{
	Intact,
	Burning,
	Burnt
};

UCLASS()
class UNDERTHEMOUNTAIN_API ABurnablePropActor : public AActor
{
	GENERATED_BODY()

  public:
	ABurnablePropActor();

	virtual void Tick(float DeltaSeconds) override;

	// [2026-11-01 T-055] Seconds of cumulative breath exposure
	// needed to ignite, and how long the burn lasts once lit.
	// Wood catches fast under a dragon (a held breath kills a
	// burglar in ~1.2 s; a crate should not outlast the burglar).
	UPROPERTY(EditAnywhere, Category = "World|Burnable")
	float IgniteExposureSeconds = 1.2f;

	UPROPERTY(EditAnywhere, Category = "World|Burnable")
	float BurnSeconds = 4.0f;

	// [2026-11-01 T-055] Radius (cm) around a burning prop that
	// takes the breath's fire DPS while it burns - standing in a
	// burning market stall hurts like standing in the breath.
	UPROPERTY(EditAnywhere, Category = "World|Burnable")
	float BurnAreaRadius = 350.0f;

	UPROPERTY(EditAnywhere, Category = "World|Burnable")
	TObjectPtr<UStaticMesh> IntactMesh;

	UPROPERTY(EditAnywhere, Category = "World|Burnable")
	TObjectPtr<UStaticMesh> RubbleMesh;

	// [2026-11-01 T-055] Feed breath exposure (seconds of this
	// tick's cone time). Returns the state after the exposure;
	// ignition is logged by the actor. No-op once Burning/Burnt.
	EBurnState AddFireExposure(float ExposureSeconds);

	// [2026-11-01 T-055] Force ignition (captures/dev). Same
	// transition as reaching the exposure threshold.
	void IgniteNow();

	EBurnState GetBurnState() const;
	float GetExposure() const;

	// [2026-11-01 T-055] Pure state rule, tested verbatim:
	// exposure at/over the threshold ignites an Intact prop;
	// Burning and Burnt never move backwards or re-ignite.
	static EBurnState StateAfterExposure(EBurnState Current, float Exposure, float Threshold);

  protected:
	virtual void BeginPlay() override;

  private:
	// [2026-11-01 T-055] Enter Burning: swap on the fire FX and
	// start the burn clock. Enter Burnt (from Tick): swap mesh to
	// rubble, FX off, leave a scorch mark under the prop.
	void EnterBurning();
	void EnterBurnt();

	UPROPERTY(VisibleAnywhere, Category = "World|Burnable")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "World|Burnable")
	TObjectPtr<UNiagaraComponent> FireFX;

	EBurnState BurnState = EBurnState::Intact;
	float ExposureSeconds = 0.0f;
	float BurnTimeRemaining = 0.0f;
};
