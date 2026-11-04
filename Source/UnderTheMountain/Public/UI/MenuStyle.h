// Copyright Under the Mountain contributors.
// Purpose: [2026-11-04] Shared code-built menu styling for the
// game-flow and settings screens (title / pause / end / settings
// must read as one menu system, not two widgets that drifted).
// Also owns the one menu Back-key definition both screens test
// against, so Esc and DualSense Circle can never diverge.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "MenuStyle.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetTree;
struct FKey;

// [2026-11-04] A menu button that takes keyboard focus on hover
// (bound to its own OnHovered in UtmMenuStyle::MakeButton), so
// mouse and pad share one focus cursor. Self-bound: the dynamic
// delegate dies with the button, no extra owner needed.
UCLASS()
class UNDERTHEMOUNTAIN_API UMenuButton : public UButton
{
	GENERATED_BODY()

  public:
	UFUNCTION()
	void HandleHoverFocus();
};

namespace UtmMenuStyle
{
// [2026-11-04] Palette: gold headings, ember primary action,
// slate secondary actions, muted body copy.
FLinearColor Gold();
FLinearColor Ember();
FLinearColor Slate();
FLinearColor Muted();
FLinearColor PanelTint();

// [2026-11-04] The menu Back chord, one definition for every
// screen: keyboard Esc and DualSense Circle (FaceButton_Right).
// Cross (FaceButton_Bottom) is select and is deliberately not
// Back. Pure and headless-testable.
bool IsMenuBackKey(const FKey& Key);

// [2026-11-04] One menu button: distinct Normal / Hovered /
// Pressed colours, and hover takes keyboard focus so the mouse
// and the pad share one focus cursor from the first frame (a
// click therefore never needs an arrow-key warm-up, and pad
// navigation resumes from wherever the mouse last hovered).
UButton* MakeButton(UWidgetTree* Tree, const TCHAR* Name, const FString& Label,
					FLinearColor Normal, int32 FontSize);

// [2026-11-04] One centred menu label; headings pass bBold.
UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size,
					 FLinearColor Color, bool bBold = false);

// [2026-11-04] Vertical breathing room between menu sections.
void AddSpacer(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, float Height);

// [2026-11-04] A thin gold rule separating heading from actions.
void AddDivider(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name);

// [2026-11-04] Add a child with generous menu padding around it.
void AddPadded(UVerticalBox* Box, UWidget* Child, float Top, float Bottom);
} // namespace UtmMenuStyle
