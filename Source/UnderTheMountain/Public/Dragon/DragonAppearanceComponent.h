// Copyright Under the Mountain contributors.
// Purpose: own the dragon's player-chosen scale tint behind one component
// so UI, the pawn and materials never reach into each other. Created
// 2026-10-28 (T-027).
// Invariants: ScaleTint channels are in [0, 1] with alpha forced to 1
// when written through SetScaleTint/LoadTint (which clamp).
// [2026-10-28 T-041] Honest contract, not encapsulation: ScaleTint
// below is a public UPROPERTY, so the editor/tests can write it
// directly, bypassing clamp/apply/save. In production code paths
// only this component's methods write the tint and the save slot.
// [2026-11-04 T-119] C++ only ever sets the 'ScaleTint' VectorParameter,
// which only the body material M_DragonScalesV6 exposes (body base x
// ScaleTint); the wing membranes use the separate, untinted
// M_DragonWingV6 material, which has no such parameter, so setting it
// on every slot leaves the wings exactly as authored.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "DragonAppearanceComponent.generated.h"

class UMaterialInstanceDynamic;

// [2026-10-28 T-027] Save payload for the scale tint. It lives in this
// header (not its own file) because nothing else reads the slot in
// production code paths, and a second file pair for one FLinearColor
// was rejected as ceremony.
// [2026-10-28 T-041] The payload field is a public UPROPERTY like the
// component's ScaleTint; "single writer/reader" of the slot is the
// production-path contract above, not an access restriction.
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

	// [2026-11-04 T-119] The material contract in one place:
	// M_DragonScalesV6 (the body material) exposes a VectorParameter
	// with exactly this name; the untinted M_DragonWingV6 wing
	// material does not. Callers and tests ask here instead of
	// spelling the string, so a rename is one edit.
	static FName TintParameterName();

	// [2026-10-28 T-039] Separate save slot for automation tests, so the
	// suite never writes the player's real 'DragonScaleTint' slot (the
	// T-004 sweep found the old test destroyed a saved tint and an
	// aborted run left its probe colour behind).
	static const FString& TestSaveSlotName();

	// [2026-10-28 T-039] Point SaveTint/LoadTint at another slot. Tests
	// pass TestSaveSlotName() on every component they save/load with;
	// production code never calls this, so a normally-constructed
	// component keeps the production default (see SaveSlotName below).
	void SetSaveSlotForTesting(const FString& SlotName);

	// [2026-10-28 T-039] The slot SaveTint/LoadTint currently use —
	// production by default, the test slot after SetSaveSlotForTesting.
	FString GetSaveSlotName() const;

	// [2026-10-28 T-027] Current tint, returned as stored.
	// [2026-10-28 T-041] It is clamped only if it arrived via
	// SetScaleTint/LoadTint; a direct public-field write (editor/tests)
	// is returned unclamped — this getter does not re-validate.
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
	// player state, not authored tuning.
	// [2026-10-28 T-041] Public UPROPERTY for editor/tests; in
	// production code paths SetScaleTint/LoadTint are the only writers.
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

	// [2026-10-28 T-039] Write ScaleTint to the SaveSlotName slot
	// (production 'DragonScaleTint' unless a test swapped it).
	void SaveTint() const;

	// [2026-10-28 T-039] Slot SaveTint/LoadTint use. Defaults to the
	// production UDragonTintSaveGame::SaveSlotName ('DragonScaleTint')
	// and is swappable only for tests via SetSaveSlotForTesting, so
	// automation never overwrites the player's saved tint.
	FString SaveSlotName = UDragonTintSaveGame::SaveSlotName;

	// [2026-10-28 T-027] Per-slot dynamic instances created by ApplyTint.
	// UPROPERTY so the GC keeps them alive between applies.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};
