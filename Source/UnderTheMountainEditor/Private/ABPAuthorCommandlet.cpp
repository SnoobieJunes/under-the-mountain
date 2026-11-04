// Copyright Under the Mountain contributors.
// [2026-11-04 T-139] ABP author commandlet implementation. See the
// header for why this is C++ and the invariants it keeps. Everything
// below is editor-only.

#include "ABPAuthorCommandlet.h"

#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ModifyBone.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_RotateRootBone.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimStateEntryNode.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AnimationStateGraph.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationTransitionGraph.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dragon/DragonAnimInstance.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Factories/AnimBlueprintFactory.h"
#include "K2Node_EnumEquality.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
const TCHAR* ABPPackagePath = TEXT("/Game/Meshy/Dragon/ABP_Dragon");
const TCHAR* SkeletonPath = TEXT("/Game/Meshy/Dragon/SK_Dragon_v6_Skeleton.SK_Dragon_v6_Skeleton");

// [2026-11-04 T-139] One row of the build sheet §2 table. ClipName is
// the asset under /Game/Meshy/Dragon/; Dive reuses the Glide clip (no
// Dive clip exists — C++ holds it near-frozen via FlapPlayRate).
struct FStateSpec
{
	const TCHAR* StateName;
	const TCHAR* ClipName;
	EDragonAnimState EnumValue;
};

const FStateSpec StateSpecs[] = {
	{TEXT("Idle"), TEXT("SK_Dragon_v4_IdleAnim"), EDragonAnimState::Idle},
	{TEXT("Walk"), TEXT("SK_Dragon_v4_Anim"), EDragonAnimState::Walk},
	{TEXT("Fly"), TEXT("SK_Dragon_v4_FlyAnim"), EDragonAnimState::Fly},
	{TEXT("Glide"), TEXT("SK_Dragon_v4_GlideAnim"), EDragonAnimState::Glide},
	{TEXT("Hover"), TEXT("SK_Dragon_v4_HoverAnim"), EDragonAnimState::Hover},
	{TEXT("Dive"), TEXT("SK_Dragon_v4_GlideAnim"), EDragonAnimState::Dive},
};

int32 Fail(const FString& Reason)
{
	UE_LOG(LogTemp, Error, TEXT("ABP_AUTHOR FAIL %s"), *Reason);
	return 1;
}

// [2026-11-04 T-139] Spawn a node the way the graph editors do:
// FGraphNodeCreator runs PostPlacedNewNode (which is what creates a
// state machine's EditorStateMachineGraph, a state's BoundGraph and
// a transition's BoundGraph) and allocates its pins.
template <typename NodeType>
NodeType* SpawnNode(UEdGraph* Graph, float X, float Y)
{
	FGraphNodeCreator<NodeType> Creator(*Graph);
	NodeType* Node = Creator.CreateNode(false);
	if (!Node)
	{
		return nullptr;
	}
	Node->NodePosX = static_cast<int32>(X);
	Node->NodePosY = static_cast<int32>(Y);
	Creator.Finalize();
	return Node;
}

UEdGraphPin* FindPin(UEdGraphNode* Node, const TCHAR* Name, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == Direction && Pin->PinName == FName(Name))
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* FindPinContaining(UEdGraphNode* Node, const TCHAR* Substring, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == Direction &&
			Pin->PinName.ToString().Contains(Substring))
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* FirstPin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == Direction)
		{
			return Pin;
		}
	}
	return nullptr;
}

// [2026-11-04 T-139] Pose output of an anim node, pose input when a
// preferred property name exists (BasePose/Source/Result...) and the
// first input pin otherwise. Pin names come from the FAnimNode_*
// struct properties, so the preferred names are load-bearing.
UEdGraphPin* PoseOut(UEdGraphNode* Node)
{
	return FirstPin(Node, EGPD_Output);
}

UEdGraphPin* PoseIn(UEdGraphNode* Node, const TCHAR* PreferredName)
{
	if (UEdGraphPin* Pin = FindPin(Node, PreferredName, EGPD_Input))
	{
		return Pin;
	}
	return FirstPin(Node, EGPD_Input);
}

bool Link(UEdGraph* Graph, UEdGraphPin* From, UEdGraphPin* To, const TCHAR* What)
{
	if (!From || !To)
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_AUTHOR link '%s': missing pin (from=%p to=%p)"), What,
			   static_cast<void*>(From), static_cast<void*>(To));
		return false;
	}
	const UEdGraphSchema* Schema = Graph ? Graph->GetSchema() : nullptr;
	if (!Schema || !Schema->TryCreateConnection(From, To))
	{
		From->MakeLinkTo(To);
	}
	if (!From->LinkedTo.Contains(To))
	{
		UE_LOG(LogTemp, Error, TEXT("ABP_AUTHOR link '%s': connection not formed (%s.%s -> %s.%s)"),
			   What, *From->GetOwningNode()->GetName(), *From->PinName.ToString(),
			   *To->GetOwningNode()->GetName(), *To->PinName.ToString());
		return false;
	}
	return true;
}

void LogPins(UEdGraphNode* Node, const TCHAR* Label)
{
	if (!Node)
	{
		return;
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR pin %s %s: %s dir=%d cat=%s"), Label,
			   *Node->GetName(), *Pin->PinName.ToString(), static_cast<int32>(Pin->Direction),
			   *Pin->PinType.PinCategory.ToString());
	}
}

// [2026-11-04 T-139] A pure "Get <Variable>" node for a published
// UDragonAnimInstance variable. The variable reference must be set
// before Finalize allocates pins, or the output pin has no type.
UK2Node_VariableGet* SpawnVariableGet(UEdGraph* Graph, FName VariableName, float X, float Y)
{
	FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);
	UK2Node_VariableGet* Node = Creator.CreateNode(false);
	if (!Node)
	{
		return nullptr;
	}
	Node->VariableReference.SetSelfMember(VariableName);
	Node->NodePosX = static_cast<int32>(X);
	Node->NodePosY = static_cast<int32>(Y);
	Creator.Finalize();
	return Node;
}

UEdGraphPin* VariableOut(UK2Node_VariableGet* Node)
{
	if (!Node)
	{
		return nullptr;
	}
	if (UEdGraphPin* Pin = Node->FindPin(Node->GetVarName()))
	{
		return Pin;
	}
	return FirstPin(Node, EGPD_Output);
}

} // namespace

UABPAuthorCommandlet::UABPAuthorCommandlet()
{
	// [2026-11-04 T-139] Same commandlet contract as UTerrainCommandlet:
	// editor world + asset loading, log to console for the caller.
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UABPAuthorCommandlet::Main(const FString& Params)
{
	// ---- Assets the graph references (fail before touching the ABP) ----
	USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, SkeletonPath);
	if (!Skeleton)
	{
		return Fail(FString::Printf(TEXT("skeleton not found: %s"), SkeletonPath));
	}

	TArray<UAnimSequenceBase*> Clips;
	for (const FStateSpec& Spec : StateSpecs)
	{
		const FString ClipPath =
			FString::Printf(TEXT("/Game/Meshy/Dragon/%s.%s"), Spec.ClipName, Spec.ClipName);
		UAnimSequenceBase* Clip = LoadObject<UAnimSequenceBase>(nullptr, *ClipPath);
		if (!Clip)
		{
			return Fail(FString::Printf(TEXT("clip not found for state %s: %s"), Spec.StateName, *ClipPath));
		}
		Clips.Add(Clip);
	}

	// ---- Load or create the AnimBlueprint ----
	bool bCreated = false;
	UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, ABPPackagePath);
	if (!AnimBP)
	{
		UPackage* Package = CreatePackage(ABPPackagePath);
		if (!Package)
		{
			return Fail(TEXT("could not create package for ABP_Dragon"));
		}
		UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
		Factory->ParentClass = UDragonAnimInstance::StaticClass();
		Factory->TargetSkeleton = Skeleton;
		AnimBP = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(
			UAnimBlueprint::StaticClass(), Package, TEXT("ABP_Dragon"),
			RF_Public | RF_Standalone, nullptr, GWarn));
		if (!AnimBP)
		{
			return Fail(TEXT("AnimBlueprintFactory failed to create ABP_Dragon"));
		}
		bCreated = true;
		FAssetRegistryModule::AssetCreated(AnimBP);
		UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR created %s (parent=%s skeleton=%s)"), ABPPackagePath,
			   *GetNameSafe(AnimBP->ParentClass), *GetNameSafe(AnimBP->TargetSkeleton));
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR loaded existing %s (parent=%s skeleton=%s)"),
			   ABPPackagePath, *GetNameSafe(AnimBP->ParentClass), *GetNameSafe(AnimBP->TargetSkeleton));
		if (AnimBP->ParentClass != UDragonAnimInstance::StaticClass())
		{
			return Fail(FString::Printf(TEXT("existing ABP parent is %s, expected DragonAnimInstance; "
											 "refusing to reparent programmatically"),
										*GetNameSafe(AnimBP->ParentClass)));
		}
		if (!AnimBP->TargetSkeleton)
		{
			AnimBP->TargetSkeleton = Skeleton;
		}
	}

	// ---- Find (or create) the main AnimGraph ----
	UAnimationGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AnimBP->FunctionGraphs)
	{
		if (UAnimationGraph* Candidate = Cast<UAnimationGraph>(Graph))
		{
			AnimGraph = Candidate;
			break;
		}
	}
	if (!AnimGraph)
	{
		for (UEdGraph* Graph : AnimBP->UbergraphPages)
		{
			if (UAnimationGraph* Candidate = Cast<UAnimationGraph>(Graph))
			{
				AnimGraph = Candidate;
				break;
			}
		}
	}
	if (!AnimGraph)
	{
		AnimGraph = Cast<UAnimationGraph>(FBlueprintEditorUtils::CreateNewGraph(
			AnimBP, TEXT("AnimGraph"), UAnimationGraph::StaticClass(),
			UAnimationGraphSchema::StaticClass()));
		if (AnimGraph)
		{
			AnimBP->FunctionGraphs.Add(AnimGraph);
			const UEdGraphSchema* Schema = AnimGraph->GetSchema();
			Schema->CreateDefaultNodesForGraph(*AnimGraph);
		}
		if (!AnimGraph)
		{
			return Fail(TEXT("ABP has no AnimGraph and CreateNewGraph failed"));
		}
	}
	UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR AnimGraph=%s nodes=%d (created_asset=%d)"),
		   *AnimGraph->GetName(), AnimGraph->Nodes.Num(), bCreated ? 1 : 0);

	// ---- Idempotent rebuild: delete every node except the Output Pose
	// root. DestroyNode (not RemoveNode) so state-machine/state/
	// transition sub-graphs are removed from the blueprint too. ----
	UAnimGraphNode_Root* RootNode = FBlueprintEditorUtils::GetAnimGraphRoot(AnimGraph);
	for (int32 Guard = 0; Guard < 512; ++Guard)
	{
		UEdGraphNode* Victim = nullptr;
		for (UEdGraphNode* Node : AnimGraph->Nodes)
		{
			if (Node && Node != RootNode)
			{
				Victim = Node;
				break;
			}
		}
		if (!Victim)
		{
			break;
		}
		Victim->DestroyNode();
	}
	if (!RootNode)
	{
		const UEdGraphSchema* Schema = AnimGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*AnimGraph);
		RootNode = FBlueprintEditorUtils::GetAnimGraphRoot(AnimGraph);
	}
	if (!RootNode)
	{
		return Fail(TEXT("AnimGraph has no Output Pose (UAnimGraphNode_Root) after rebuild prep"));
	}
	LogPins(RootNode, TEXT("root"));

	// ---- (1) AnimGraph chain ----
	// OutputPose <- RotateRootBone <- slither ModifyBone chain (below) <-
	// LayeredBoneBlend; base = Slot FullBody wrapping the state machine,
	// branch 0 = Slot UpperBody.
	UAnimGraphNode_StateMachine* StateMachineNode =
		SpawnNode<UAnimGraphNode_StateMachine>(AnimGraph, -600.0f, 0.0f);
	UAnimGraphNode_Slot* FullBodySlot = SpawnNode<UAnimGraphNode_Slot>(AnimGraph, -300.0f, 0.0f);
	UAnimGraphNode_Slot* UpperBodySlot = SpawnNode<UAnimGraphNode_Slot>(AnimGraph, -300.0f, 220.0f);
	UAnimGraphNode_LayeredBoneBlend* LayeredNode =
		SpawnNode<UAnimGraphNode_LayeredBoneBlend>(AnimGraph, 0.0f, 0.0f);
	UAnimGraphNode_RotateRootBone* RotateNode =
		SpawnNode<UAnimGraphNode_RotateRootBone>(AnimGraph, 300.0f, 0.0f);
	// [2026-11-04 feel/spam] Slither/tail-lag chain: additive ModifyBone
	// [2026-11-04 feel/spam] (TransformBone) nodes, Layered -> pelvis ->
	// [2026-11-04 feel/spam] tail.01 -> tail.03 -> tail.05 -> Rotate. Each
	// [2026-11-04 feel/spam] binds one published FRotator from the
	// [2026-11-04 feel/spam] AnimInstance (component-space yaw, slither +
	// [2026-11-04 feel/spam] lag pre-combined in C++), so the graph needs
	// [2026-11-04 feel/spam] no math nodes and the statics stay the rule.
	struct FSlitherBoneSpec
	{
		const TCHAR* BoneName;
		const TCHAR* RotationVariable;
	};
	const FSlitherBoneSpec SlitherBones[] = {
		{TEXT("pelvis"), TEXT("SpineSlitherRotation")},
		{TEXT("tail.01"), TEXT("TailBaseSlitherRotation")},
		{TEXT("tail.03"), TEXT("TailMidSlitherRotation")},
		{TEXT("tail.05"), TEXT("TailTipSlitherRotation")},
	};
	TArray<UAnimGraphNode_ModifyBone*> SlitherNodes;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SlitherBones); ++Index)
	{
		UAnimGraphNode_ModifyBone* ModifyNode = SpawnNode<UAnimGraphNode_ModifyBone>(
			AnimGraph, 80.0f + 60.0f * Index, -220.0f - 60.0f * Index);
		if (!ModifyNode)
		{
			return Fail(TEXT("failed to spawn a slither ModifyBone node"));
		}
		ModifyNode->Node.BoneToModify.BoneName = FName(SlitherBones[Index].BoneName);
		ModifyNode->Node.RotationMode = BMM_Additive;
		ModifyNode->Node.TranslationMode = BMM_Ignore;
		ModifyNode->Node.ScaleMode = BMM_Ignore;
		ModifyNode->Node.RotationSpace = BCS_ComponentSpace;
		SlitherNodes.Add(ModifyNode);
	}
	if (!StateMachineNode || !FullBodySlot || !UpperBodySlot || !LayeredNode || !RotateNode)
	{
		return Fail(TEXT("failed to spawn an AnimGraph chain node"));
	}
	if (!StateMachineNode->EditorStateMachineGraph)
	{
		return Fail(TEXT("state machine node has no EditorStateMachineGraph after placement"));
	}
	// [2026-11-04 T-139] Idempotency note: any pre-existing state
	// machine node (e.g. the MCP-placed AnimGraphNode_StateMachine_0
	// in the main repo's asset) was destroyed by the rebuild prep
	// above, so this fresh node is the only machine in the graph.
	FBlueprintEditorUtils::RenameGraph(StateMachineNode->EditorStateMachineGraph,
									   TEXT("Locomotion"));

	FullBodySlot->Node.SlotName = TEXT("FullBody");
	UpperBodySlot->Node.SlotName = TEXT("UpperBody");

	// LayeredBoneBlend branch 0: bone 'chest', blend depth 0. The node
	// constructor already creates one blend pose (AddFirstPose); make
	// the invariant explicit instead of assuming it.
	while (LayeredNode->Node.BlendPoses.Num() < 1)
	{
		LayeredNode->Node.AddPose();
	}
	while (LayeredNode->Node.BlendPoses.Num() > 1)
	{
		LayeredNode->Node.RemovePose(LayeredNode->Node.BlendPoses.Num() - 1);
	}
	LayeredNode->Node.LayerSetup.SetNum(1);
	LayeredNode->Node.LayerSetup[0].BranchFilters.Reset();
	{
		FBranchFilter& Filter = LayeredNode->Node.LayerSetup[0].BranchFilters.AddDefaulted_GetRef();
		Filter.BoneName = TEXT("chest");
		Filter.BlendDepth = 0;
	}
	LayeredNode->Node.BlendWeights.SetNum(1);
	LayeredNode->Node.BlendWeights[0] = 1.0f;
	LayeredNode->ReconstructNode();

	LogPins(StateMachineNode, TEXT("statemachine"));
	LogPins(FullBodySlot, TEXT("slot-fullbody"));
	LogPins(LayeredNode, TEXT("layered"));
	LogPins(RotateNode, TEXT("rotate"));

	bool bLinked = true;
	bLinked &= Link(AnimGraph, PoseOut(StateMachineNode), PoseIn(FullBodySlot, TEXT("Source")),
					TEXT("SM->FullBodySlot"));
	bLinked &= Link(AnimGraph, PoseOut(StateMachineNode), PoseIn(UpperBodySlot, TEXT("Source")),
					TEXT("SM->UpperBodySlot"));
	bLinked &= Link(AnimGraph, PoseOut(FullBodySlot), PoseIn(LayeredNode, TEXT("BasePose")),
					TEXT("FullBodySlot->Layered.BasePose"));
	{
		UEdGraphPin* BlendPosePin = FindPinContaining(LayeredNode, TEXT("BlendPose"), EGPD_Input);
		if (!BlendPosePin)
		{
			BlendPosePin = FindPin(LayeredNode, TEXT("BlendPoses"), EGPD_Input);
		}
		bLinked &= Link(AnimGraph, PoseOut(UpperBodySlot), BlendPosePin, TEXT("UpperBodySlot->Layered.BlendPose0"));
	}
	// [2026-11-04 T-139 fix] ModifyBone is a component-space skeletal
	// control: the local-space Layered pose must convert in/out around
	// the chain or the schema refuses the link (proven by ABP_AUTHOR
	// link errors). Chain: Layered -> LocalToComponent -> ModifyBones
	// -> ComponentToLocal -> RotateRootBone.
	UAnimGraphNode_LocalToComponentSpace* ToComponent =
		SpawnNode<UAnimGraphNode_LocalToComponentSpace>(AnimGraph, 1330.0f, -120.0f);
	UAnimGraphNode_ComponentToLocalSpace* ToLocal =
		SpawnNode<UAnimGraphNode_ComponentToLocalSpace>(AnimGraph, 1330.0f, 260.0f);
	if (!ToComponent || !ToLocal)
	{
		return Fail(TEXT("failed to spawn pose space conversion nodes"));
	}
	bLinked &= Link(AnimGraph, PoseOut(LayeredNode), PoseIn(ToComponent, TEXT("Pose")),
					TEXT("Layered->LocalToComponent"));
	UEdGraphNode* ChainTail = ToComponent;
	for (int32 Index = 0; Index < SlitherNodes.Num(); ++Index)
	{
		UAnimGraphNode_ModifyBone* ModifyNode = SlitherNodes[Index];
		bLinked &= Link(AnimGraph, PoseOut(ChainTail), PoseIn(ModifyNode, TEXT("ComponentPose")),
						TEXT("SlitherChain pose"));
		if (UEdGraphPin* RotationPin = FindPin(ModifyNode, TEXT("Rotation"), EGPD_Input))
		{
			UK2Node_VariableGet* GetRotation = SpawnVariableGet(
				AnimGraph, FName(SlitherBones[Index].RotationVariable), 80.0f, -360.0f - 60.0f * Index);
			bLinked &= Link(AnimGraph, VariableOut(GetRotation), RotationPin,
							TEXT("SlitherRotation->ModifyBone.Rotation"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_AUTHOR slither bone %s: no Rotation pin"),
				   SlitherBones[Index].BoneName);
			bLinked = false;
		}
		ChainTail = ModifyNode;
	}
	bLinked &= Link(AnimGraph, PoseOut(ChainTail), PoseIn(ToLocal, TEXT("ComponentPose")),
					TEXT("SlitherChain->ComponentToLocal"));
	bLinked &= Link(AnimGraph, PoseOut(ToLocal), PoseIn(RotateNode, TEXT("BasePose")),
					TEXT("ComponentToLocal->Rotate.BasePose"));
	bLinked &= Link(AnimGraph, PoseOut(RotateNode), PoseIn(RootNode, TEXT("Result")),
					TEXT("Rotate->OutputPose"));
	if (!bLinked)
	{
		return Fail(TEXT("AnimGraph chain wiring failed (see link errors above)"));
	}

	// RotateRootBone published-variable bindings. The runtime node
	// exposes Pitch and Yaw pins only (FAnimNode_RotateRootBone::
	// Evaluate builds FRotator(Pitch, Yaw, 0)); the build sheet's
	// "Roll = BankDegrees" is therefore wired to the node's second
	// axis (Yaw pin) — the lean direction check in sheet §1 still
	// applies in the editor preview.
	if (UEdGraphPin* PitchPin = FindPin(RotateNode, TEXT("Pitch"), EGPD_Input))
	{
		UK2Node_VariableGet* GetPitch =
			SpawnVariableGet(AnimGraph, TEXT("MeshPitchDegrees"), 300.0f, 200.0f);
		bLinked &= Link(AnimGraph, VariableOut(GetPitch), PitchPin, TEXT("MeshPitchDegrees->Rotate.Pitch"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ABP_AUTHOR RotateRootBone has no Pitch pin; leaving Node.Pitch=%f"),
			   RotateNode->Node.Pitch);
	}
	if (UEdGraphPin* YawPin = FindPin(RotateNode, TEXT("Yaw"), EGPD_Input))
	{
		UK2Node_VariableGet* GetBank = SpawnVariableGet(AnimGraph, TEXT("BankDegrees"), 300.0f, 320.0f);
		bLinked &= Link(AnimGraph, VariableOut(GetBank), YawPin, TEXT("BankDegrees->Rotate.Yaw(second axis)"));
	}
	if (!bLinked)
	{
		return Fail(TEXT("RotateRootBone variable wiring failed"));
	}

	// ---- (2)+(3) State machine ----
	UAnimationStateMachineGraph* StateMachineGraph = StateMachineNode->EditorStateMachineGraph;
	UAnimStateEntryNode* EntryNode = StateMachineGraph->EntryNode;
	if (!EntryNode)
	{
		for (UEdGraphNode* Node : StateMachineGraph->Nodes)
		{
			if (UAnimStateEntryNode* Candidate = Cast<UAnimStateEntryNode>(Node))
			{
				EntryNode = Candidate;
				break;
			}
		}
	}
	if (!EntryNode)
	{
		return Fail(TEXT("state machine graph has no entry node"));
	}

	TArray<UAnimStateNode*> StateNodes;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(StateSpecs); ++Index)
	{
		const FStateSpec& Spec = StateSpecs[Index];
		UAnimStateNode* StateNode = SpawnNode<UAnimStateNode>(
			StateMachineGraph, -200.0f + 260.0f * (Index % 3), -260.0f + 260.0f * (Index / 3));
		if (!StateNode || !StateNode->BoundGraph)
		{
			return Fail(FString::Printf(TEXT("failed to spawn state %s"), Spec.StateName));
		}
		FBlueprintEditorUtils::RenameGraph(StateNode->BoundGraph, Spec.StateName);
		StateNodes.Add(StateNode);

		UAnimationStateGraph* StateGraph = Cast<UAnimationStateGraph>(StateNode->BoundGraph);
		UAnimGraphNode_StateResult* StateResult = nullptr;
		if (StateGraph && StateGraph->MyResultNode)
		{
			StateResult = StateGraph->MyResultNode;
		}
		else
		{
			StateResult = StateNode->GetResultNodeInsideState();
		}
		if (!StateGraph || !StateResult)
		{
			return Fail(FString::Printf(TEXT("state %s has no StateResult node"), Spec.StateName));
		}

		UAnimGraphNode_SequencePlayer* SequencePlayer =
			SpawnNode<UAnimGraphNode_SequencePlayer>(StateGraph, -200.0f, 0.0f);
		if (!SequencePlayer)
		{
			return Fail(FString::Printf(TEXT("state %s: failed to spawn SequencePlayer"), Spec.StateName));
		}
		SequencePlayer->SetAnimationAsset(Clips[Index]);

		// PlayRate: expose the pin (PinHiddenByDefault on the struct
		// property) and wire FlapPlayRate; fall back to the property.
		bool bPlayRateBound = false;
		for (FOptionalPinFromProperty& OptionalPin : SequencePlayer->ShowPinForProperties)
		{
			if (OptionalPin.PropertyName == TEXT("PlayRate"))
			{
				OptionalPin.bShowPin = true;
			}
		}
		SequencePlayer->ReconstructNode();
		if (UEdGraphPin* PlayRatePin = FindPin(SequencePlayer, TEXT("PlayRate"), EGPD_Input))
		{
			// [2026-11-04 feel/spam] Per-state rate variable: Walk strides
			// [2026-11-04 feel/spam] bind WalkPlayRate, every other state
			// [2026-11-04 feel/spam] keeps the FlapPlayRate wingbeat curve.
			const TCHAR* RateVariable =
				Spec.EnumValue == EDragonAnimState::Walk ? TEXT("WalkPlayRate") : TEXT("FlapPlayRate");
			UK2Node_VariableGet* GetFlap =
				SpawnVariableGet(StateGraph, FName(RateVariable), -420.0f, 140.0f);
			bPlayRateBound = Link(StateGraph, VariableOut(GetFlap), PlayRatePin,
								  TEXT("PlayRate->SequencePlayer.PlayRate"));
		}
		if (!bPlayRateBound)
		{
			SequencePlayer->Node.SetPlayRate(1.0f);
			UE_LOG(LogTemp, Warning,
				   TEXT("ABP_AUTHOR state %s: no PlayRate pin exposed; set Node.PlayRate=1.0 "
						"(C++ curve returns 1.0 for ground states)"),
				   Spec.StateName);
		}

		if (!Link(StateGraph, PoseOut(SequencePlayer), PoseIn(StateResult, TEXT("Result")),
				  TEXT("SequencePlayer->StateResult")))
		{
			return Fail(FString::Printf(TEXT("state %s: SequencePlayer->StateResult wiring failed"),
										Spec.StateName));
		}
	}

	// Entry -> Idle (first state).
	if (!Link(StateMachineGraph, EntryNode->GetOutputPin(), StateNodes[0]->GetInputPin(),
			  TEXT("Entry->Idle")))
	{
		return Fail(TEXT("entry -> Idle wiring failed"));
	}

	// All 30 ordered-pair transitions: rule is exactly
	// LocomotionState == <TargetState>, crossfade 0.25 s.
	UEnum* AnimStateEnum = StaticEnum<EDragonAnimState>();
	int32 TransitionCount = 0;
	for (int32 SourceIndex = 0; SourceIndex < StateNodes.Num(); ++SourceIndex)
	{
		for (int32 TargetIndex = 0; TargetIndex < StateNodes.Num(); ++TargetIndex)
		{
			if (SourceIndex == TargetIndex)
			{
				continue;
			}
			UAnimStateTransitionNode* Transition = SpawnNode<UAnimStateTransitionNode>(
				StateMachineGraph, 0.0f, 0.0f);
			if (!Transition || !Transition->BoundGraph)
			{
				return Fail(TEXT("failed to spawn a transition node"));
			}
			Transition->CrossfadeDuration = 0.25f;
			Transition->PriorityOrder = TransitionCount + 1;
			Transition->CreateConnections(StateNodes[SourceIndex], StateNodes[TargetIndex]);

			UAnimationTransitionGraph* TransitionGraph =
				Cast<UAnimationTransitionGraph>(Transition->BoundGraph);
			UAnimGraphNode_TransitionResult* TransitionResult =
				TransitionGraph ? TransitionGraph->MyResultNode : nullptr;
			if (!TransitionGraph || !TransitionResult)
			{
				return Fail(TEXT("transition has no TransitionResult node"));
			}

			UK2Node_VariableGet* GetState =
				SpawnVariableGet(TransitionGraph, TEXT("LocomotionState"), -420.0f, 0.0f);
			UK2Node_EnumEquality* Equality =
				SpawnNode<UK2Node_EnumEquality>(TransitionGraph, -180.0f, 0.0f);
			if (!GetState || !Equality)
			{
				return Fail(TEXT("transition rule node spawn failed"));
			}
			bool bRule = true;
			bRule &= Link(TransitionGraph, VariableOut(GetState), Equality->GetInput1Pin(),
						  TEXT("LocomotionState->EnumEquality.A"));
			// B is the target enum value. Linking A typed the pin;
			// set the type explicitly too so the default survives
			// a schema that only types on connection notification.
			UEdGraphPin* BPin = Equality->GetInput2Pin();
			if (BPin)
			{
				if (BPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard && AnimStateEnum)
				{
					BPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
					BPin->PinType.PinSubCategoryObject = AnimStateEnum;
				}
				BPin->DefaultValue = StateSpecs[TargetIndex].StateName;
				BPin->AutogeneratedDefaultValue = StateSpecs[TargetIndex].StateName;
			}
			bRule &= Link(TransitionGraph, Equality->GetReturnValuePin(),
						  FindPin(TransitionResult, TEXT("bCanEnterTransition"), EGPD_Input),
						  TEXT("EnumEquality->TransitionResult"));
			if (!bRule)
			{
				return Fail(FString::Printf(TEXT("transition %s->%s rule wiring failed"),
											StateSpecs[SourceIndex].StateName,
											StateSpecs[TargetIndex].StateName));
			}
			++TransitionCount;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR states=%d transitions=%d"), StateNodes.Num(),
		   TransitionCount);

	// ---- (4) Compile with zero errors, then save ----
	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	FCompilerResultsLog CompilerLog;
	FKismetEditorUtilities::CompileBlueprint(AnimBP, EBlueprintCompileOptions::None, &CompilerLog);
	for (TSharedPtr<FTokenizedMessage> Message : CompilerLog.Messages)
	{
		if (Message.IsValid())
		{
			UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR compile: %s"), *Message->ToText().ToString());
		}
	}
	if (CompilerLog.NumErrors > 0 || AnimBP->Status == BS_Error)
	{
		return Fail(FString::Printf(TEXT("compile failed: %d error(s), %d warning(s), status=%d"),
									CompilerLog.NumErrors, CompilerLog.NumWarnings,
									static_cast<int32>(AnimBP->Status)));
	}
	UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR compile ok: 0 errors, %d warning(s)"), CompilerLog.NumWarnings);

	AnimBP->MarkPackageDirty();
	UPackage* Package = AnimBP->GetOutermost();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_None;
	if (!UPackage::SavePackage(Package, AnimBP, *Filename, SaveArgs))
	{
		return Fail(FString::Printf(TEXT("SavePackage failed: %s"), *Filename));
	}

	UE_LOG(LogTemp, Display, TEXT("ABP_AUTHOR DONE saved=%s states=%d transitions=%d warnings=%d"),
		   *Filename, StateNodes.Num(), TransitionCount, CompilerLog.NumWarnings);
	return 0;
}
