// Copyright Under the Mountain contributors.
// [2026-11-01 T-016] UHoardSubsystem implementation.

#include "Hoard/HoardSubsystem.h"
#include "UnderTheMountain.h"

EHoardOutcome UHoardSubsystem::ClassifyOutcome(int64 CurrentHoard, int64 StartingHoard,
											   float WinFraction, float LoseFraction,
											   bool bFinalWaveSurvived, bool bDragonDown)
{
	// [2026-11-01 T-016] Driven out beats every gold clause: a
	// dragon at 0 HP loses even sitting on the full hoard.
	if (bDragonDown)
	{
		return EHoardOutcome::LoseDrivenOut;
	}
	if (!bFinalWaveSurvived || StartingHoard <= 0)
	{
		return EHoardOutcome::InProgress;
	}
	const double Fraction =
		static_cast<double>(CurrentHoard) / static_cast<double>(StartingHoard);
	// [2026-11-01 T-016] Epsilon at the boundary: 1700/8500 in
	// double lands a hair under the float 0.2f, and the spec says
	// 1700 exactly IS the pyrrhic floor - boundary gold values
	// must classify inclusively, never by float dust.
	if (Fraction + 1e-6 >= WinFraction)
	{
		return EHoardOutcome::Win;
	}
	if (Fraction + 1e-6 >= LoseFraction)
	{
		return EHoardOutcome::PyrrhicWin;
	}
	return EHoardOutcome::LosePlundered;
}

void UHoardSubsystem::RecordSessionStart(int64 RegistryTotalGold)
{
	if (bSessionStarted)
	{
		return;
	}
	bSessionStarted = true;
	StartingHoard = RegistryTotalGold;
	// [2026-11-01 T-016] Thresholds asset is optional tuning; the
	// class defaults carry the spec fractions when it is absent.
	if (ThresholdsAsset == nullptr)
	{
		ThresholdsAsset = LoadObject<UHoardThresholdsAsset>(
			nullptr, TEXT("/Game/Gameplay/Waves/DA_HoardThresholds.DA_HoardThresholds"));
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Hoard session start: %lld gold registered"),
		   StartingHoard);
}

void UHoardSubsystem::RecordTheft(int64 GoldValue, const TCHAR* ThiefName)
{
	StolenGold += GoldValue;
	UE_LOG(LogUnderTheMountain, Log, TEXT("Hoard theft: %s escaped with %lld gold (stolen %lld)"),
		   ThiefName, GoldValue, StolenGold);
}

int64 UHoardSubsystem::GetStartingHoard() const
{
	return StartingHoard;
}

int64 UHoardSubsystem::GetStolenGold() const
{
	return StolenGold;
}

int64 UHoardSubsystem::GetCurrentHoard(int64 RegistryTotalGold) const
{
	// [2026-11-01 T-016] What is still yours is what is still
	// registered minus what walked out the door.
	return FMath::Max<int64>(0, RegistryTotalGold - StolenGold);
}

const UHoardThresholdsAsset* UHoardSubsystem::Thresholds() const
{
	return ThresholdsAsset;
}

int32 UHoardSubsystem::HoardMoundStep(int64 CurrentHoard, int64 StartingHoard, float WarnFraction,
									  float WinFraction, float LoseFraction)
{
	// [2026-11-03 T-086] Same boundary contract as ClassifyOutcome:
	// double-precision fraction with the 1e-6 epsilon, inclusive —
	// sitting exactly on a line reads the fuller mound, never the
	// emptier one by float dust.
	if (StartingHoard <= 0)
	{
		return 0;
	}
	const double Fraction = static_cast<double>(CurrentHoard) / static_cast<double>(StartingHoard);
	if (Fraction + 1e-6 >= WarnFraction)
	{
		return 0;
	}
	if (Fraction + 1e-6 >= WinFraction)
	{
		return 1;
	}
	if (Fraction + 1e-6 >= LoseFraction)
	{
		return 2;
	}
	return 3;
}

float UHoardSubsystem::HoardMoundScaleForStep(int32 Step)
{
	// [2026-11-03 T-086] Fixed presentation sizes: the boundaries are
	// data (see above), these multipliers are just what reads.
	switch (FMath::Clamp(Step, 0, 3))
	{
	case 0:
		return 1.0f;
	case 1:
		return 0.8f;
	case 2:
		return 0.6f;
	default:
		return 0.4f;
	}
}
