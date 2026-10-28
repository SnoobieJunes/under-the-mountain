// Copyright Under the Mountain contributors.
// Purpose: the world-scoped owner of UTreasureRegistry, so actors in a
// level share one treasure truth while tests use the registry directly
// without a world. Created 2026-10-28 (T-013).
// Invariants: exactly one registry per world, created in Initialize and
// owned for the subsystem's lifetime; forwarding calls before
// Initialize (or on a torn-down world) fail explicitly, never crash.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Treasure/TreasureRegistry.h"
#include "TreasureSubsystem.generated.h"

// [2026-10-28 T-013] UWorldSubsystem is the injected seam between the
// hermetic registry and live actors: ATreasureActor registers into it on
// BeginPlay, UScentComponent reads it each tick.
UCLASS()
class UNDERTHEMOUNTAIN_API UTreasureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-013] Creates the registry. Called by the engine
	// when the world comes up, before actor BeginPlay.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// [2026-10-28 T-013] Drops the registry reference so world teardown
	// releases it with the subsystem instead of outliving the world.
	virtual void Deinitialize() override;

	// [2026-10-28 T-013] The owned registry, or nullptr before
	// Initialize. Callers that iterate every entry (scent) use this;
	// one-shot callers use the forwarding verbs below.
	UTreasureRegistry* Registry() const;

	// [2026-10-28 T-013] Convenience forwarding to the registry.
	// Register returns NAME_None and Unregister false when no registry
	// exists yet, so a missing subsystem is a visible failure rather
	// than a null dereference.
	FName Register(const FTreasureEntry& Entry);
	bool Unregister(FName Id);

  private:
	// [2026-10-28 T-013] TObjectPtr with UPROPERTY keeps the registry
	// reachable for garbage collection for exactly the subsystem's
	// lifetime (a raw pointer would let GC collect it under us).
	UPROPERTY()
	TObjectPtr<UTreasureRegistry> RegistryObject;
};
