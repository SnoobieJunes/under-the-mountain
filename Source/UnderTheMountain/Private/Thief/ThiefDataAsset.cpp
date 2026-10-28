// Copyright Under the Mountain contributors.
// [2026-10-28 T-014] UThiefDataAsset implementation. Only the primary
// asset id needs code; the rest is authored data by design (see header).

#include "Thief/ThiefDataAsset.h"

FPrimaryAssetId UThiefDataAsset::GetPrimaryAssetId() const
{
	// [2026-10-28 T-014] One shared "Thief" type for every thief
	// definition; the per-instance part is this asset's name.
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("Thief")), GetFName());
}
