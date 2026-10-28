// Copyright Under the Mountain contributors.
// [2026-10-28 T-027] Scale-tint implementation. All clamping, mesh
// writes and slot I/O live here; UI only ever calls SetScaleTint and
// the pawn never touches the material parameter directly (PLAN.md seams).
// [2026-10-28 T-041] That is the production-path contract, not
// encapsulation: ScaleTint is a public UPROPERTY (see the header).

#include "Dragon/DragonAppearanceComponent.h"
#include "UnderTheMountain.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

// [2026-10-28 T-027] Slot name as a TEXT literal: the on-disk file is
// Saved/SaveGames/DragonScaleTint.sav, and tests rely on that name
// staying stable.
const FString UDragonTintSaveGame::SaveSlotName = TEXT("DragonScaleTint");

namespace
{
// [2026-10-28 T-027] Single clamp used on every entry path (Set and
// Load) so a hand-edited or stale save cannot smuggle out-of-range
// values past the header's invariant.
FLinearColor ClampTint(const FLinearColor& In)
{
	return FLinearColor(FMath::Clamp(In.R, 0.0f, 1.0f), FMath::Clamp(In.G, 0.0f, 1.0f),
						FMath::Clamp(In.B, 0.0f, 1.0f), 1.0f);
}
} // namespace

UDragonAppearanceComponent::UDragonAppearanceComponent()
{
	// [2026-10-28 T-027] Event-driven only (BeginPlay load + explicit
	// Set calls); nothing here needs a tick.
	PrimaryComponentTick.bCanEverTick = false;
}

FName UDragonAppearanceComponent::TintParameterName()
{
	return FName(TEXT("ScaleTint"));
}

const FString& UDragonAppearanceComponent::TestSaveSlotName()
{
	// [2026-10-28 T-039] Function-local static: one stable test-slot
	// name ('DragonScaleTintTest') distinct from the production
	// UDragonTintSaveGame::SaveSlotName, returned by reference so
	// callers never spell (or typo) the literal themselves.
	static const FString TestSlotName = TEXT("DragonScaleTintTest");
	return TestSlotName;
}

void UDragonAppearanceComponent::SetSaveSlotForTesting(const FString& SlotName)
{
	// [2026-10-28 T-039] Test-only seam: redirect SaveTint/LoadTint
	// away from the player's slot. Production code never calls this.
	SaveSlotName = SlotName;
}

FString UDragonAppearanceComponent::GetSaveSlotName() const
{
	return SaveSlotName;
}

FLinearColor UDragonAppearanceComponent::GetScaleTint() const
{
	return ScaleTint;
}

void UDragonAppearanceComponent::SetScaleTint(const FLinearColor& Tint, bool bSave)
{
	ScaleTint = ClampTint(Tint);
	ApplyTint();
	if (bSave)
	{
		SaveTint();
	}
}

void UDragonAppearanceComponent::LoadTint()
{
	// [2026-10-28 T-039] Missing slot = first run: keep the current
	// (default white) tint silently and still apply it, so the mesh
	// shows the same colour the component reports. Uses the
	// SaveSlotName member (production by default, test slot in tests).
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UDragonTintSaveGame::SaveUserIndex))
	{
		// [2026-10-28 T-039] A slot that exists but loads as null or
		// the wrong class is corruption, not a first run — warn
		// naming the slot instead of silently keeping the tint.
		if (UDragonTintSaveGame* Loaded = Cast<UDragonTintSaveGame>(
				UGameplayStatics::LoadGameFromSlot(SaveSlotName, UDragonTintSaveGame::SaveUserIndex)))
		{
			ScaleTint = ClampTint(Loaded->ScaleTint);
		}
		else
		{
			UE_LOG(LogUnderTheMountain, Warning,
				   TEXT("Tint load FAILED: slot '%s' exists but could not be loaded; keeping current tint"),
				   *SaveSlotName);
		}
	}
	ApplyTint();
}

void UDragonAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadTint();
}

void UDragonAppearanceComponent::ApplyTint()
{
	// [2026-10-28 T-027] Null-guarded on purpose: headless tests (and a
	// pawn whose mesh asset failed to load) still store the tint via
	// SetScaleTint; only this mesh write is skipped.
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	ApplyTintToMesh(Owner->FindComponentByClass<USkeletalMeshComponent>());
}

void UDragonAppearanceComponent::ApplyTintToMesh(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	const int32 NumSlots = Mesh->GetNumMaterials();
	if (DynamicMaterials.Num() != NumSlots)
	{
		DynamicMaterials.SetNum(NumSlots);
	}
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		// [2026-10-28 T-027] Reuse the slot's dynamic instance across
		// applies (slider drags call this continuously; recreating MIDs
		// per change would churn shader state). Recreate only when the
		// slot no longer holds our instance — e.g. the mesh or its
		// materials were swapped after our first apply.
		if (!DynamicMaterials[Slot] || Mesh->GetMaterial(Slot) != DynamicMaterials[Slot])
		{
			DynamicMaterials[Slot] = Mesh->CreateDynamicMaterialInstance(Slot);
		}
		if (DynamicMaterials[Slot])
		{
			// [2026-10-28 T-027] The only parameter C++ ever sets.
			// Membrane colour is masked inside M_DragonScales itself,
			// so setting ScaleTint on every slot is safe.
			DynamicMaterials[Slot]->SetVectorParameterValue(TintParameterName(), ScaleTint);
		}
	}
}

void UDragonAppearanceComponent::SaveTint() const
{
	UDragonTintSaveGame* Save = Cast<UDragonTintSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UDragonTintSaveGame::StaticClass()));
	if (!Save)
	{
		// [2026-10-28 T-039] Never silent: a failed save-object
		// creation otherwise looks exactly like a persisted tint.
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Tint save FAILED: could not create save object"));
		return;
	}
	Save->ScaleTint = ScaleTint;
	// [2026-10-28 T-039] Honour SaveGameToSlot's result and name the
	// slot (the SaveSlotName member — production by default, test
	// slot in tests) so a failed write is diagnosable from the log.
	if (!UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, UDragonTintSaveGame::SaveUserIndex))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Tint save FAILED: SaveGameToSlot returned false for slot '%s'"), *SaveSlotName);
	}
}
