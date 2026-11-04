// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] FScentMath implementation. Pure functions only —
// no world, no clock reads (time is a parameter), no entry location —
// so Tests/TreasureScentTest.cpp can pin the docs/GAMEPLAY.md section 3
// numbers exactly.

#include "Scent/ScentMath.h"

namespace
{
// [2026-10-28 T-013] One bracket row for RibbonWidthForValue: values in
// [Lo, Hi] map around BaseWidth cm. The crown row's Hi is a shaping cap
// (5000), not a bracket edge — see the header.
struct FScentWidthBracket
{
	int32 Lo;
	int32 Hi;
	float BaseWidth;
};
} // namespace

float FScentMath::RibbonWidthForValue(int32 GoldValue)
{
	// [2026-10-28 T-013] docs/GAMEPLAY.md section 3 value brackets.
	static const FScentWidthBracket Brackets[] = {
		{1, 24, 4.0f},
		{25, 99, 8.0f},
		{100, 399, 14.0f},
		{400, 999, 22.0f},
		{1000, 5000, 32.0f},
	};

	// [2026-10-28 T-013] First bracket whose Hi covers the value;
	// values above the crown cap stay in the crown bracket with the
	// fraction clamped below. Values <= 0 land in the trinket bracket
	// with a fraction clamped to 0, i.e. the bracket floor.
	const FScentWidthBracket* Chosen = &Brackets[UE_ARRAY_COUNT(Brackets) - 1];
	for (const FScentWidthBracket& Bracket : Brackets)
	{
		if (GoldValue <= Bracket.Hi)
		{
			Chosen = &Bracket;
			break;
		}
	}

	const int32 Span = Chosen->Hi - Chosen->Lo;
	const float Fraction =
		Span > 0 ? FMath::Clamp(static_cast<float>(GoldValue - Chosen->Lo) / static_cast<float>(Span),
								0.0f, 1.0f)
				 : 0.0f;
	return Chosen->BaseWidth * (0.9f + 0.2f * Fraction);
}

float FScentMath::RibbonDensityScaleForValue(int32 GoldValue)
{
	// [2026-11-04 T-127] Derived from the width so the two cues can never
	// [2026-11-04 T-127] disagree about which treasure is richer.
	const float Width = RibbonWidthForValue(GoldValue);
	return FMath::Clamp(0.75f + Width / 20.0f, 1.0f, 2.5f);
}

float FScentMath::RibbonSheenScaleForValue(int32 GoldValue)
{
	// [2026-11-04 T-127] Log-ish value ramp: most of the sheen arrives by
	// [2026-11-04 T-127] mid-value treasure, crown is fully metallic.
	if (GoldValue <= 0)
	{
		return 0.0f;
	}
	return FMath::Clamp(FMath::Loge(1.0f + static_cast<float>(GoldValue)) / FMath::Loge(1001.0f), 0.0f, 1.0f);
}

FLinearColor FScentMath::RibbonSheenColorForValue(int32 GoldValue)
{
	// [2026-11-04 T-127] Tarnished gold -> bright metallic gold (linear).
	const float T = RibbonSheenScaleForValue(GoldValue);
	return FLinearColor(FMath::Lerp(0.55f, 1.0f, T), FMath::Lerp(0.38f, 0.78f, T),
						FMath::Lerp(0.12f, 0.28f, T), 1.0f);
}

float FScentMath::RibbonOpacity(bool bTrueSightActive)
{
	return bTrueSightActive ? 0.3f : 1.0f;
}

float FScentMath::PulsedWidth(float BaseWidth, float TimeSeconds, bool bCarried)
{
	if (!bCarried)
	{
		return BaseWidth;
	}
	// [2026-10-28 T-013] 0.5 Hz sine, +/-10%: 2*pi*0.5*t. The literal
	// keeps this file free of engine math-header constants so the
	// pulse reads exactly as docs/GAMEPLAY.md section 3 states it.
	const float Phase = 6.28318530718f * 0.5f * TimeSeconds;
	return BaseWidth * (1.0f + 0.1f * FMath::Sin(Phase));
}

FScentRibbonParams FScentMath::ParamsForEntry(const FTreasureEntry& Entry, float TimeSeconds,
											  bool bTrueSightActive)
{
	// [2026-10-28 T-041] No-information rule enforced structurally:
	// this function never reads Entry.Location, so width and opacity
	// are identical for two equal-value treasures at any distance
	// apart (pinned by the UnderTheMountain.Scent
	// .NoDistanceAndTrueSight test).
	FScentRibbonParams Params;
	Params.Width = PulsedWidth(RibbonWidthForValue(Entry.GoldValue), TimeSeconds, Entry.bCarried);
	Params.Opacity = RibbonOpacity(bTrueSightActive);
	Params.bCarried = Entry.bCarried;
	// [2026-11-04 T-127] Density/sheen from value only (no-information rule
	// [2026-11-04 T-127] unchanged: no location, no distance).
	Params.DensityScale = RibbonDensityScaleForValue(Entry.GoldValue);
	Params.SheenScale = RibbonSheenScaleForValue(Entry.GoldValue);
	Params.SheenColor = RibbonSheenColorForValue(Entry.GoldValue);
	return Params;
}
