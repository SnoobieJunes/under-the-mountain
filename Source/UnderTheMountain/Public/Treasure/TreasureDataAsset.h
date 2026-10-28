// Copyright Under the Mountain contributors.
// Purpose: authored definition of one treasure (name, gold value and
// whether it is the Seeing Stone) as a primary data asset, so designers
// tune treasure in data and actors only instantiate it. Created
// 2026-10-28 (T-013).
// Invariants: GoldValue is never negative (clamped in the editor and
// again when an actor registers it); this asset holds no runtime state —
// identity, location and carried state live in UTreasureRegistry.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TreasureDataAsset.generated.h"

// [2026-10-28 T-013] UPrimaryDataAsset (not plain UDataAsset) so treasure
// definitions are addressable by the Asset Manager under the "Treasure"
// primary asset type, which later loot-table loading builds on.
UCLASS(BlueprintType)
class UNDERTHEMOUNTAIN_API UTreasureDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-013] Human-readable name for HUD/log use; gameplay
	// identity is the registry id, never this string.
	UPROPERTY(EditAnywhere, Category = "Treasure")
	FString DisplayName;

	// [2026-10-28 T-013] Gold value driving scent-ribbon width
	// (docs/GAMEPLAY.md section 3) and hoard totals. Negative treasure
	// has no meaning in the design, so the floor is 0.
	UPROPERTY(EditAnywhere, Category = "Treasure", meta = (ClampMin = "0"))
	int32 GoldValue = 0;

	// [2026-10-28 T-013] Exactly one Seeing Stone gates true-sight
	// (docs/GAMEPLAY.md section 3): while a carried entry with this flag
	// exists, UScentComponent refuses to enable true-sight.
	UPROPERTY(EditAnywhere, Category = "Treasure")
	bool bIsSeeingStone = false;

	// [2026-10-28 T-013] Primary asset id under the shared "Treasure"
	// type, keyed by asset name so the Asset Manager can enumerate every
	// treasure definition without loading the whole Content tree.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
