// Copyright Under the Mountain contributors.
// Purpose: headless authoring of ABP_Dragon's AnimGraph (T-139).
// docs/ABP-DRAGON-BUILDSHEET.md is the contract; UDragonAnimInstance
// (C++) owns every locomotion rule and publishes the variables this
// graph only reads (LocomotionState, FlapPlayRate, MeshPitchDegrees,
// BankDegrees). Programmatic graph authoring was declared unavailable
// in T-125 from editor/MCP probes, but the AnimGraph editor classes
// (UAnimGraphNode_*, UAnimStateNode, UK2Node_VariableGet) are plain
// UObjects: a commandlet can spawn, wire and compile them without
// Slate, which is what this commandlet proves or disproves.
//
//   UnrealEditor-Cmd <project> -run=ABPAuthor
//
// Created 2026-11-04 (T-139).
// Invariants: idempotent (the AnimGraph, state graphs and transition
// graphs are rebuilt from scratch on every run, never appended to),
// and honest (prints ABP_AUTHOR DONE only after a zero-error compile
// AND a successful save; otherwise ABP_AUTHOR FAIL with the reason).

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ABPAuthorCommandlet.generated.h"

UCLASS()
class UNDERTHEMOUNTAINEDITOR_API UABPAuthorCommandlet : public UCommandlet
{
	GENERATED_BODY()

  public:
	UABPAuthorCommandlet();

	// [2026-11-04 T-139] Loads (or creates) /Game/Meshy/Dragon/ABP_Dragon,
	// rebuilds its AnimGraph per docs/ABP-DRAGON-BUILDSHEET.md, compiles
	// it, saves it, and logs ABP_AUTHOR DONE / FAIL.
	virtual int32 Main(const FString& Params) override;
};
