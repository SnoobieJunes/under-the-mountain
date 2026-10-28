// Copyright Under the Mountain contributors.
// Purpose: own the dragon's player-chosen scale tint behind one component
// so UI, the pawn and materials never reach into each other. Created
// 2026-10-28 (T-027).
// Invariants: ScaleTint channels are always in [0, 1] with alpha forced
// to 1; only this component writes the tint and the matching save slot.
// C++ only ever sets the 'ScaleTint' VectorParameter on M_DragonScales —
// the crimson membranes are protected inside that material's graph, so
// no code here needs (or gets) a way to recolour them.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "DragonAppearanceComponent.generated.h"

class UMaterialInstanceDynamic;

// [2026-10-28 T-027] Save payload for the scale tint. It lives in this
// header (not its own file) because nothing else reads the slot: the
// component below is the single writer/reader, and a second file pair
// for one FLinearColor was rejected as ceremony.
UCLASS()
class UNDERTHEMOUNTAIN_API UDragonTintSaveGame : public USaveGame
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-027] White = untinted scales (the material's authored
	// colour shows through unchanged), so a fresh save reads as default.
	UPROPERTY()
	FLinearColor ScaleTint = FLinearColor::White;

	// [2026-10-28 T-027] Fixed slot identity: one dragon, one tint. The
	// user index stays 0 (single-player, no profiles) until a task
	// introduces them.
	static const FString SaveSlotName;
	static constexpr int32 SaveUserIndex = 0;
};

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UDragonAppearanceComponent();

	// [2026-10-28 T-027] The material contract in one place: M_DragonScales
	// exposes a VectorParameter with exactly this name. Callers and tests
	// ask here instead of spelling the string, so a rename is one edit.
	static FName TintParameterName();

	// [2026-10-28 T-027] Current tint. Always clamped (see SetScaleTint),
	// so readers never need to re-validate it.
	FLinearColor GetScaleTint() const;

	// [2026-10-28 T-027] Store, apply and (unless bSave is false) persist
	// a new tint. Each channel is clamped to [0, 1] and alpha forced to 1
	// — the parameter is a colour multiplier, and an alpha below 1 would
	// silently darken in materials that read it as opacity. Safe with no
	// owner/mesh/world: the value is still stored (automation tests drive
	// this headless), only the mesh write is skipped. bSave=false exists
	// for tests and previews that must not touch the player's slot.
	void SetScaleTint(const FLinearColor& Tint, bool bSave = true);

	// [2026-10-28 T-027] Load the saved tint (if the slot exists) and
	// apply it without re-saving. Called from BeginPlay; also callable
	// headless — UGameplayStatics slot I/O needs no world.
	void LoadTint();

	// [2026-10-28 T-027] Push the current tint onto an explicit mesh
	// (one reused dynamic instance per slot). ApplyTint() calls this
	// with the owner's skeletal mesh; tests call it directly so the
	// ScaleTint parameter write is exercised headless.
	void ApplyTintToMesh(class USkeletalMeshComponent* Mesh);

	// [2026-10-28 T-027] Visible (not editable) in the editor: this is
	// player state owned by SetScaleTint/LoadTint, not authored tuning.
	UPROPERTY(VisibleAnywhere, Category = "Dragon|Appearance")
	FLinearColor ScaleTint = FLinearColor::White;

  protected:
	virtual void BeginPlay() override;

  private:
	// [2026-10-28 T-027] Push ScaleTint onto the owner's skeletal mesh:
	// one UMaterialInstanceDynamic per material slot, created once and
	// reused (recreated only if a slot's material changed under us).
	// No-ops when there is no owner or no skeletal mesh.
	void ApplyTint();

	// [2026-10-28 T-027] Write ScaleTint to the DragonScaleTint slot.
	void SaveTint() const;

	// [2026-10-28 T-027] Per-slot dynamic instances created by ApplyTint.
	// UPROPERTY so the GC keeps them alive between applies.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};
