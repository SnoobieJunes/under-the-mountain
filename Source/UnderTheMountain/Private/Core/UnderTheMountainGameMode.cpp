// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Constructor-only game mode: DefaultPawnClass is the
// dragon; everything else is engine defaults until a task needs more.

#include "Core/UnderTheMountainGameMode.h"
#include "Dragon/DragonCharacter.h"

AUnderTheMountainGameMode::AUnderTheMountainGameMode()
{
	DefaultPawnClass = ADragonCharacter::StaticClass();
}
