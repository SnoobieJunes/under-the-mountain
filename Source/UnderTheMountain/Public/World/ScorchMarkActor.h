// Copyright Under the Mountain contributors.
// Purpose: a persistent scorch mark (T-055) - the flat dark disc
// fire leaves behind on the ground, spawned by burnt props and by
// sustained breath on the ground (the pawn owns that spawning).
// Deliberately just a material on a plane: a decal system would
// conform to slopes, but the world's fight/flight surfaces are
// flat-authored (valley, terraces, chamber floors), and a disc
// that always lands is worth more than a decal that sometimes
// doesn't. Session-persistent, no tick, no collision.
// Created 2026-11-01 (T-055).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScorchMarkActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API AScorchMarkActor : public AActor
{
	GENERATED_BODY()

  public:
	AScorchMarkActor();

	// [2026-11-01 T-055] Disc diameter in cm (the burnt prop's
	// footprint, or the breath's ground patch). Set by the spawner
	// right after SpawnActor; the mesh scale follows it.
	void SetDiameter(float DiameterCm);

  private:
	UPROPERTY(VisibleAnywhere, Category = "World|Burnable")
	TObjectPtr<UStaticMeshComponent> Disc;
};
