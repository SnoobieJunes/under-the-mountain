// Copyright Under the Mountain contributors.
// Purpose: T-110 v6 mesh-swap contract — the pawn renders SK_Dragon_v6
// (39-bone rig, body + wing slots, head bone for the T-111 camera anchor)
// and holds bind pose because the v4 clips are skeleton-gated off until
// T-068 authors v6 replacements. Created 2026-11-03.

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
	// [2026-11-03 T-110] Every v4 clip is skeleton-gated to null in the
	// constructor (mesh skeleton vs clip skeleton mismatch), so the pawn
	// holds bind pose. When T-068 lands v6 clips it updates these pins.
	TestNull(TEXT("WalkAnim gated off until v6 clips"), Pawn->WalkAnim);
	TestNull(TEXT("FlyAnim gated off until v6 clips"), Pawn->FlyAnim);
	TestNull(TEXT("GlideAnim gated off until v6 clips"), Pawn->GlideAnim);
	TestNull(TEXT("HoverAnim gated off until v6 clips"), Pawn->HoverAnim);
	TestNull(TEXT("IdleAnim gated off until v6 clips"), Pawn->IdleAnim);
	TestNull(TEXT("TakeoffAnim gated off until v6 clips"), Pawn->TakeoffAnim);
	TestNull(TEXT("LandAnim gated off until v6 clips"), Pawn->LandAnim);
	TestNull(TEXT("RoarAnim gated off until v6 clips"), Pawn->RoarAnim);
	TestNull(TEXT("HitAnim gated off until v6 clips"), Pawn->HitAnim);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
