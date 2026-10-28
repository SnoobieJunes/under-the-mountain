// Copyright Under the Mountain contributors.
// [2026-10-28 T-027] Scale-tint implementation. All clamping, mesh
// writes and slot I/O live here; UI only ever calls SetScaleTint and
// the pawn never touches the material parameter directly (PLAN.md
// seams, mirroring UDragonStatsComponent's write-only-via-component
// rule).

#include "Dragon/DragonAppearanceComponent.h"

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
	// [2026-10-28 T-027] Missing slot = first run: keep the current
	// (default white) tint and still apply it, so the mesh shows the
	// same colour the component reports.
	if (UGameplayStatics::DoesSaveGameExist(UDragonTintSaveGame::SaveSlotName,
											UDragonTintSaveGame::SaveUserIndex))
	{
		if (UDragonTintSaveGame* Loaded = Cast<UDragonTintSaveGame>(UGameplayStatics::LoadGameFromSlot(
				UDragonTintSaveGame::SaveSlotName, UDragonTintSaveGame::SaveUserIndex)))
		{
			ScaleTint = ClampTint(Loaded->ScaleTint);
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
		return;
	}
	Save->ScaleTint = ScaleTint;
	UGameplayStatics::SaveGameToSlot(Save, UDragonTintSaveGame::SaveSlotName,
									 UDragonTintSaveGame::SaveUserIndex);
}
