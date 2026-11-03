// Copyright Under the Mountain contributors.
// Purpose: T-083 dragon XP pool (GAMEPLAY.md section 8, minimal M1:
// earn + display, levels arrive M2). Single writer like the T-016
// seams: the pawn's audit loop calls the Claim* methods; each is
// idempotent per thief/wave, so reprocessing the same corpse (eat
// after kill) can never double-award. No level curve, no stat
// growth in M1 (DA_HoardThresholds.XPCurve is reserved empty).
// Created 2026-11-03 (T-083).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "XPSubsystem.generated.h"

UCLASS()
class UNDERTHEMOUNTAIN_API UXPSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-11-03 T-083] Spec rates (GAMEPLAY.md section 8,
	// [tuning: PT-WAVE]): +10 per thief killed inside, +5 per thief
	// scared off lootless, +1 per 25 gold net-gained per wave.
	static constexpr int32 KillXP = 10;
	static constexpr int32 ScareXP = 5;
	static constexpr int32 GoldPerWaveXP = 25;

	int32 GetTotalXP() const
	{
		return TotalXP;
	}

	// [2026-11-03 T-083] Pure award rules (headless-tested): how much
	// one event is worth before idempotence. Outside kills and looted
	// escapes earn nothing; wave gold below one full bracket (or a net
	// loss) earns nothing.
	static int32 AwardForKill(bool bInsideLair);
	static int32 AwardForScare(bool bLootless);
	static int32 AwardForWaveGold(int64 NetGold);

	// [2026-11-03 T-083] Idempotent claims: first call for an ID (or
	// wave) awards, repeats return 0. Eat-after-kill reprocesses the
	// same corpse ID, which is already claimed — no double award.
	int32 ClaimKill(const FName& ThiefId, bool bInsideLair);
	int32 ClaimScare(const FName& ThiefId, bool bLootless);
	int32 ClaimWaveGold(int32 Wave, int64 NetGold);

  private:
	UPROPERTY()
	int32 TotalXP = 0;

	TSet<FName> ClaimedKills;
	TSet<FName> ClaimedScares;
	TSet<int32> ClaimedWaves;
};
