// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] UTreasureDataAsset implementation. Only the primary
// asset id needs code; the rest is authored data by design (see header).

#include "Treasure/TreasureDataAsset.h"

FPrimaryAssetId UTreasureDataAsset::GetPrimaryAssetId() const
{
	// [2026-10-28 T-013] One shared "Treasure" type for every treasure
	// definition; the per-instance part is this asset's name.
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("Treasure")), GetFName());
}
