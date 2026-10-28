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
	// [2026-10-28 T-013] No-information rule enforced structurally:
	// this function never reads Entry.Location, so width, opacity and
	// color are identical for two equal-value treasures at any
	// distance apart (pinned by the UnderTheMountain.Scent
	// .NoDistanceAndTrueSight test).
	FScentRibbonParams Params;
	Params.Width = PulsedWidth(RibbonWidthForValue(Entry.GoldValue), TimeSeconds, Entry.bCarried);
	Params.Opacity = RibbonOpacity(bTrueSightActive);
	Params.Color = FLinearColor(0.686f, 0.525f, 0.345f, 1.0f);
	Params.bCarried = Entry.bCarried;
	return Params;
}
