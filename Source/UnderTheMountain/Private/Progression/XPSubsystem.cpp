// Copyright Under the Mountain contributors.
// [2026-11-03 T-083] XP awards: single-writer idempotent claims over
// pure per-event rules. First claim wins; everything after is 0.

#include "Progression/XPSubsystem.h"

int32 UXPSubsystem::AwardForKill(bool bInsideLair)
{
	return bInsideLair ? KillXP : 0;
}

int32 UXPSubsystem::AwardForScare(bool bLootless)
{
	return bLootless ? ScareXP : 0;
}

int32 UXPSubsystem::AwardForWaveGold(int64 NetGold)
{
	if (NetGold < GoldPerWaveXP)
	{
		return 0;
	}
	return static_cast<int32>(NetGold / GoldPerWaveXP);
}

int32 UXPSubsystem::ClaimKill(const FName& ThiefId, bool bInsideLair)
{
	if (ClaimedKills.Contains(ThiefId))
	{
		return 0;
	}
	ClaimedKills.Add(ThiefId);
	const int32 Award = AwardForKill(bInsideLair);
	TotalXP += Award;
	return Award;
}

int32 UXPSubsystem::ClaimScare(const FName& ThiefId, bool bLootless)
{
	if (ClaimedScares.Contains(ThiefId))
	{
		return 0;
	}
	ClaimedScares.Add(ThiefId);
	const int32 Award = AwardForScare(bLootless);
	TotalXP += Award;
	return Award;
}

int32 UXPSubsystem::ClaimWaveGold(int32 Wave, int64 NetGold)
{
	if (ClaimedWaves.Contains(Wave))
	{
		return 0;
	}
	ClaimedWaves.Add(Wave);
	const int32 Award = AwardForWaveGold(NetGold);
	TotalXP += Award;
	return Award;
}
