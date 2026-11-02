// Copyright Under the Mountain contributors.
// Purpose: T-019 HUD formatting tests - star pips, the gold line
// and bar fractions, verbatim (what the player reads is what the
// test proves).

#include "Misc/AutomationTest.h"
#include "UI/DragonHUDWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-01 T-019] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonHUDTest, "UnderTheMountain.UI.HUD",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonHUDTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("two stars render two filled pips"),
			  UDragonHUDWidget::FormatStars(2, 35),
			  FString(TEXT("\u2605\u2605\u2606\u2606\u2606  35 pts")));
	TestEqual(TEXT("zero stars render all empty"),
			  UDragonHUDWidget::FormatStars(0, 0),
			  FString(TEXT("\u2606\u2606\u2606\u2606\u2606  0 pts")));
	TestEqual(TEXT("five stars render all filled"),
			  UDragonHUDWidget::FormatStars(5, 160),
			  FString(TEXT("\u2605\u2605\u2605\u2605\u2605  160 pts")));
	TestEqual(TEXT("over-max stars clamp to five pips"),
			  UDragonHUDWidget::FormatStars(9, 999),
			  FString(TEXT("\u2605\u2605\u2605\u2605\u2605  999 pts")));
	TestEqual(TEXT("gold line"),
			  UDragonHUDWidget::FormatHoard(2750, 3000, 3, TEXT("Active")),
			  FString(TEXT("2750 / 3000 gold")));
	TestEqual(TEXT("bar fraction mid"), UDragonHUDWidget::BarFraction(55.0f, 100.0f), 0.55f);
	TestEqual(TEXT("bar fraction clamps over max"), UDragonHUDWidget::BarFraction(150.0f, 100.0f), 1.0f);
	TestEqual(TEXT("bar fraction clamps under zero"), UDragonHUDWidget::BarFraction(-5.0f, 100.0f), 0.0f);
	TestEqual(TEXT("bar fraction with broken max is empty"),
			  UDragonHUDWidget::BarFraction(50.0f, 0.0f), 0.0f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
