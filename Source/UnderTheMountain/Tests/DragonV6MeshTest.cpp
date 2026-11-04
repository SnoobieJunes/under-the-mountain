// Copyright Under the Mountain contributors.
// Purpose: T-110 v6 mesh-swap contract — the pawn renders SK_Dragon_v6
// (39-bone rig, body + wing slots, head bone for the T-111 camera anchor)
// and plays the owner-drop clips. Created 2026-11-03; clips landed T-113
// (2026-11-04), flipping the T-110 skeleton-gate pins from null to live.

#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Dragon/DragonCharacter.h"
#include "Engine/SkeletalMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-03 T-110] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonV6MeshTest, "UnderTheMountain.Dragon.V6Mesh",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonV6MeshTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("v6 pawn constructs"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	USkeletalMesh* DragonMesh = Pawn->GetMesh()->GetSkeletalMeshAsset();
	TestNotNull(TEXT("v6 pawn has a mesh asset"), DragonMesh);
	if (DragonMesh == nullptr)
	{
		return false;
	}
	// [2026-11-03 T-110] Identity: the playable dragon is v6, not v4.
	TestEqual(TEXT("pawn mesh is SK_Dragon_v6"), DragonMesh->GetName(), FString(TEXT("SK_Dragon_v6")));
	// [2026-11-03 T-110] Two slots: body (M_DragonScalesV6, tinted) + wing
	// (M_DragonWingV6, untinted). The tint component already loops slots.
	TestEqual(TEXT("v6 mesh has body + wing slots"), Pawn->GetMesh()->GetNumMaterials(), 2);
	// [2026-11-03 T-110] The T-111 camera anchors the chase pivot + eyes to
	// this bone; a rename without a pawn update must go red here, not in PIE.
	TestTrue(TEXT("v6 skeleton has a head bone"),
			 DragonMesh->GetRefSkeleton().FindBoneIndex(TEXT("head")) != INDEX_NONE);
	// [2026-11-04 T-118] Pinned rig bones: the chase pivot reads chest and
	// [2026-11-04 T-118] the Eyes pivot reads head_end (sweep #3 finding 8) —
	// [2026-11-04 T-118] a rename without a pawn update must go red here.
	TestTrue(TEXT("v6 skeleton has a chest bone"),
			 DragonMesh->GetRefSkeleton().FindBoneIndex(TEXT("chest")) != INDEX_NONE);
	TestTrue(TEXT("v6 skeleton has a head_end bone"),
			 DragonMesh->GetRefSkeleton().FindBoneIndex(TEXT("head_end")) != INDEX_NONE);
	// [2026-11-04 T-113] T-113: the newest owner-drop clips (Scripts/
	// [2026-11-04 T-113] import_newest_dragon_anims.py) land on the pawn's
	// [2026-11-04 T-113] nine clip paths on the v6 skeleton, so the T-110
	// [2026-11-04 T-113] gate self-heals: every clip survives it non-null,
	// [2026-11-04 T-113] on the mesh's own skeleton. A null here means the
	// [2026-11-04 T-113] import regressed or the skeleton drifted — red here,
	// [2026-11-04 T-113] not a silent bind-pose dragon in PIE.
	USkeleton* MeshSkeleton = DragonMesh->GetSkeleton();
	TestNotNull(TEXT("v6 mesh has a skeleton"), MeshSkeleton);
	int32 LiveClips = 0;
	for (const TPair<const TCHAR*, UAnimSequence*>& Clip :
		 {TPair<const TCHAR*, UAnimSequence*>(TEXT("WalkAnim"), Pawn->WalkAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("FlyAnim"), Pawn->FlyAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("GlideAnim"), Pawn->GlideAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("HoverAnim"), Pawn->HoverAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("IdleAnim"), Pawn->IdleAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("TakeoffAnim"), Pawn->TakeoffAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("LandAnim"), Pawn->LandAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("RoarAnim"), Pawn->RoarAnim),
		  TPair<const TCHAR*, UAnimSequence*>(TEXT("HitAnim"), Pawn->HitAnim)})
	{
		TestNotNull(FString::Printf(TEXT("%s is live on the newest rig"), Clip.Key), Clip.Value);
		if (Clip.Value == nullptr)
		{
			continue;
		}
		++LiveClips;
		TestEqual(
			FString::Printf(TEXT("%s is on the v6 skeleton"), Clip.Key), Clip.Value->GetSkeleton(),
			MeshSkeleton);
		TestTrue(
			FString::Printf(TEXT("%s has length"), Clip.Key), Clip.Value->GetPlayLength() > 0.0f);
	}
	TestEqual(TEXT("all nine newest clips live"), LiveClips, 9);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
