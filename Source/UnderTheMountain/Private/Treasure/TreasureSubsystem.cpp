// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] UTreasureSubsystem implementation: owns the one
// per-world UTreasureRegistry and forwards the two lifecycle verbs
// treasure actors need. All treasure logic stays in the registry so it
// remains testable without this world-bound shell.

#include "Treasure/TreasureSubsystem.h"

void UTreasureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// [2026-10-28 T-013] Outered to this subsystem so the registry's
	// lifetime is the world's, matching the actors registered in it.
	RegistryObject = NewObject<UTreasureRegistry>(this);
}

void UTreasureSubsystem::Deinitialize()
{
	RegistryObject = nullptr;
	Super::Deinitialize();
}

UTreasureRegistry* UTreasureSubsystem::Registry() const
{
	return RegistryObject;
}

FName UTreasureSubsystem::Register(const FTreasureEntry& Entry)
{
	if (RegistryObject == nullptr)
	{
		return NAME_None;
	}
	return RegistryObject->Register(Entry);
}

bool UTreasureSubsystem::Unregister(FName Id)
{
	if (RegistryObject == nullptr)
	{
		return false;
	}
	return RegistryObject->Unregister(Id);
}
