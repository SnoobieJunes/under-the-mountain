// Copyright Under the Mountain contributors.
// Purpose: T-011 flight-economy contract tests — landing-damage curve, HP
// clamps, exhaustion drain and the new flap cost, exercised directly
// against UDragonStatsComponent with no world spawned (GAMEPLAY.md s2).
// Created 2026-10-28 (T-011). The pawn-side wiring (Landed impact capture,
// per-tick glide/hover/dive selection) is PIE/PT-STAM territory; the math
// it delegates to is proven here.

#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonStatsComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-10-28 T-011] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonFlightTest, "UnderTheMountain.Dragon.Flight",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonFlightTest::RunTest(const FString& Parameters)
{
	// [2026-10-28 T-011] Defaults first, on a fresh component, before any
	// test-local tuning overwrites them: these lock the GAMEPLAY.md s2
	// numbers (flap 3/beat, glide 1/s, hover 8/s, exhaustion 2 HP/s) and
	// the landing tuning picked in the header (800..2200 cm/s -> 5..25).
	UDragonStatsComponent* Defaults = NewObject<UDragonStatsComponent>();
	TestEqual(TEXT("flap cost default is 3 per beat"), Defaults->FlapStaminaCost, 3.0f);
	TestEqual(TEXT("glide drain default is 1/s"), Defaults->GlideDrainPerSecond, 1.0f);
	TestEqual(TEXT("hover drain default is 8/s"), Defaults->HoverDrainPerSecond, 8.0f);
	TestEqual(TEXT("exhaustion HP drain default is 2/s"), Defaults->ExhaustionHPDrainPerSecond, 2.0f);
	TestEqual(TEXT("landing threshold default is 800 cm/s"), Defaults->LandingSpeedThreshold, 800.0f);
	TestEqual(TEXT("landing max-damage speed default is 2200 cm/s"), Defaults->LandingSpeedMaxDamage, 2200.0f);
	TestEqual(TEXT("HP starts at max"), Defaults->HP, Defaults->MaxHP);

	// [2026-10-28 T-011] Flap math at the new cost: exactly one beat's
	// worth of stamina buys exactly one flap and no more.
	Defaults->Stamina = Defaults->FlapStaminaCost;
	TestTrue(TEXT("flap allowed at exactly the cost"), Defaults->CanFlap());
	TestTrue(TEXT("flap consumes at exactly the cost"),
			 Defaults->TryConsumeStamina(Defaults->FlapStaminaCost));
	TestEqual(TEXT("flap cost subtracted to empty"), Defaults->Stamina, 0.0f);
	TestFalse(TEXT("no second flap when empty"), Defaults->CanFlap());

	// [2026-10-28 T-011] Landing curve against explicitly set tuning, so
	// the shape (not today's numbers) is what this block locks.
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->LandingSpeedThreshold = 800.0f;
	Stats->LandingSpeedMaxDamage = 2200.0f;
	Stats->MinLandingDamage = 5.0f;
	Stats->MaxLandingDamage = 25.0f;

	TestEqual(TEXT("no landing damage at zero speed"), Stats->LandingDamageForImpactSpeed(0.0f), 0.0f);
	TestEqual(TEXT("no landing damage below threshold"), Stats->LandingDamageForImpactSpeed(799.0f), 0.0f);
	TestEqual(TEXT("no landing damage at threshold"), Stats->LandingDamageForImpactSpeed(800.0f), 0.0f);
	TestTrue(TEXT("minimum damage just above threshold"),
			 Stats->LandingDamageForImpactSpeed(801.0f) >= 5.0f);
	TestEqual(TEXT("max landing damage at max-damage speed"),
			  Stats->LandingDamageForImpactSpeed(2200.0f), 25.0f);
	TestEqual(TEXT("landing damage clamped above max-damage speed"),
			  Stats->LandingDamageForImpactSpeed(5000.0f), 25.0f);

	// [2026-10-28 T-011] Monotone between threshold and cap: faster must
	// never hurt less, and a midpoint impact lands strictly inside 5..25.
	float PreviousDamage = Stats->LandingDamageForImpactSpeed(800.0f);
	for (float Speed = 900.0f; Speed <= 2200.0f; Speed += 100.0f)
	{
		const float Damage = Stats->LandingDamageForImpactSpeed(Speed);
		TestTrue(FString::Printf(TEXT("landing damage monotone at %.0f cm/s"), Speed),
				 Damage >= PreviousDamage);
		PreviousDamage = Damage;
	}
	const float MidDamage = Stats->LandingDamageForImpactSpeed(1500.0f);
	TestTrue(TEXT("mid-range landing damage strictly inside 5..25"), MidDamage > 5.0f && MidDamage < 25.0f);

	// [2026-10-28 T-011] HP pool: damage subtracts, clamps at 0, and a
	// negative "damage" can never heal through the damage verb.
	Stats->MaxHP = 100.0f;
	Stats->HP = 100.0f;
	Stats->ApplyHPDamage(30.0f);
	TestEqual(TEXT("HP damage subtracted"), Stats->HP, 70.0f);
	Stats->ApplyHPDamage(-50.0f);
	TestEqual(TEXT("negative damage does not heal"), Stats->HP, 70.0f);
	Stats->ApplyHPDamage(1000.0f);
	TestEqual(TEXT("HP clamps at 0"), Stats->HP, 0.0f);

	// [2026-10-28 T-011] Exhaustion: 2 HP/s off an explicitly set rate,
	// and clamped by the same HP floor.
	Stats->HP = 100.0f;
	Stats->ExhaustionHPDrainPerSecond = 2.0f;
	Stats->DrainHPForExhaustion(2.5f);
	TestEqual(TEXT("exhaustion drains 2 HP per second"), Stats->HP, 95.0f);
	Stats->DrainHPForExhaustion(1000.0f);
	TestEqual(TEXT("exhaustion drain clamps HP at 0"), Stats->HP, 0.0f);
	Stats->HP = 50.0f;
	Stats->DrainHPForExhaustion(-5.0f);
	TestEqual(TEXT("negative exhaustion time drains nothing"), Stats->HP, 50.0f);

	// [2026-10-28 T-011] Explicit-rate flight drain: glide rate for 10 s
	// costs 10, a zero rate (dive) is free.
	Stats->MaxStamina = 100.0f;
	Stats->Stamina = 100.0f;
	Stats->DrainForFlight(10.0f, Stats->GlideDrainPerSecond);
	TestEqual(TEXT("glide-rate drain over 10 s"), Stats->Stamina, 90.0f);
	Stats->DrainForFlight(10.0f, 0.0f);
	TestEqual(TEXT("zero-rate (dive) drain is free"), Stats->Stamina, 90.0f);

	// [2026-10-28 T-036] Regression: zero stamina must not end a free
	// dive. Before this rule was extracted, the pawn drained HP only
	// for positive rates but forced MOVE_Falling for every rate, so a
	// dive at empty stamina was cancelled despite costing nothing.
	TestFalse(TEXT("zero-stamina dive does not force a fall"),
			  Stats->ShouldForceFallAtZeroStamina(0.0f));
	TestFalse(TEXT("negative flight rate does not force a fall"),
			  Stats->ShouldForceFallAtZeroStamina(-1.0f));
	TestTrue(TEXT("zero-stamina glide forces a fall"),
			 Stats->ShouldForceFallAtZeroStamina(Stats->GlideDrainPerSecond));
	TestTrue(TEXT("zero-stamina hover forces a fall"),
			 Stats->ShouldForceFallAtZeroStamina(Stats->HoverDrainPerSecond));
	return true;
}

// [2026-10-28 T-043] Pure SelectAnimState contract: dive is powered
// flight, slow non-climbing flight is a glide, slow climbing is still
// powered, and dive is ignored on the ground.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonAnimStateTest, "UnderTheMountain.Dragon.AnimState",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonAnimStateTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-068] Contract updated with Hover/Dive: a dive is its
	// [2026-11-01 T-068] own swept-wing state (not fast Fly), the slow
	// [2026-11-01 T-068] precision band is Hover, and Glide now means fast
	// [2026-11-01 T-068] descending cruise with wings held. Ground rules and
	// [2026-11-01 T-068] "dive ignored on the ground" are unchanged.
	TestTrue(TEXT("flying and diving is Dive"),
			 ADragonCharacter::SelectAnimState(true, true, 900.0f, -500.0f) == EDragonAnimState::Dive);
	TestTrue(TEXT("fast climbing flight is Fly"),
			 ADragonCharacter::SelectAnimState(true, false, 900.0f, 300.0f) == EDragonAnimState::Fly);
	TestTrue(TEXT("fast descending cruise is Glide"),
			 ADragonCharacter::SelectAnimState(true, false, 900.0f, -200.0f) == EDragonAnimState::Glide);
	TestTrue(TEXT("slow descending precision flight is Hover"),
			 ADragonCharacter::SelectAnimState(true, false, 10.0f, -100.0f) == EDragonAnimState::Hover);
	TestTrue(TEXT("slow climbing precision flight is Hover"),
			 ADragonCharacter::SelectAnimState(true, false, 10.0f, 200.0f) == EDragonAnimState::Hover);
	TestTrue(TEXT("moving on the ground is Walk"),
			 ADragonCharacter::SelectAnimState(false, false, 300.0f, 0.0f) == EDragonAnimState::Walk);
	TestTrue(TEXT("still on the ground is Idle"),
			 ADragonCharacter::SelectAnimState(false, false, 0.0f, 0.0f) == EDragonAnimState::Idle);
	TestTrue(TEXT("dive input on the ground is Walk"),
			 ADragonCharacter::SelectAnimState(false, true, 900.0f, 0.0f) == EDragonAnimState::Walk);
	return true;
}

// [2026-10-28 T-043] Pose-level flap proof: the imported Fly
// AnimSequence must actually move the wing bones in UE (a
// same-named asset with no wing tracks would pass every other
// check while the dragon holds bind pose). Samples the raw
// wing_finger_L track at the up key (t=0) and the down key
// (t=0.5s of the 1s loop) and requires real travel.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonFlyAnimPoseTest, "UnderTheMountain.Dragon.FlyAnimPose",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonFlyAnimPoseTest::RunTest(const FString& Parameters)
{
	UAnimSequence* FlyAnim = LoadObject<UAnimSequence>(
		nullptr, TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_FlyAnim.SK_Dragon_v4_FlyAnim"));
	TestNotNull(TEXT("SK_Dragon_v4_FlyAnim loads"), FlyAnim);
	USkeletalMesh* DragonMesh = LoadObject<USkeletalMesh>(
		nullptr, TEXT("/Game/Meshy/Dragon/SK_Dragon_v4.SK_Dragon_v4"));
	TestNotNull(TEXT("SK_Dragon_v4 loads"), DragonMesh);
	if (FlyAnim == nullptr || DragonMesh == nullptr)
	{
		return false;
	}
	TestTrue(TEXT("fly anim is about a second long"), FlyAnim->GetPlayLength() > 0.8f);

	const USkeleton* Skeleton = DragonMesh->GetSkeleton();
	TestNotNull(TEXT("dragon skeleton loads"), Skeleton);
	if (Skeleton == nullptr)
	{
		return false;
	}
	const int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(TEXT("wing_shoulder_L"));
	TestTrue(TEXT("wing_shoulder_L bone exists"), BoneIndex != INDEX_NONE);
	if (BoneIndex == INDEX_NONE)
	{
		return false;
	}

	// A bone's local LOCATION is constant under rotation animation;
	// the flap lives in its local ROTATION, so compare orientations.
	FTransform UpTransform;
	FTransform DownTransform;
	FlyAnim->GetBoneTransform(UpTransform, FSkeletonPoseBoneIndex(BoneIndex), FAnimExtractContext(0.0),
							  true);
	FlyAnim->GetBoneTransform(DownTransform, FSkeletonPoseBoneIndex(BoneIndex),
							  FAnimExtractContext(0.5), true);
	const double Swing =
		UpTransform.GetRotation().AngularDistance(DownTransform.GetRotation());
	TestTrue(FString::Printf(TEXT("wing shoulder swings between flap keys (%.3f rad)"), Swing),
			 Swing > 0.15);

	// [2026-11-01 T-068] T-058 tuck proof: every leg's upper bone must sit
	// [2026-11-01 T-068] far from its bind rotation in Fly (T-043 left legs
	// [2026-11-01 T-068] unkeyed, i.e. exactly at bind — the owner-visible
	// [2026-11-01 T-068] defect). 0.3 rad ~ 17 deg is far under the authored
	// [2026-11-01 T-068] 48-62 deg tuck but far above bind-pose noise.
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	for (const TCHAR* LegBone :
		 {TEXT("frontleg_upper_L"), TEXT("frontleg_upper_R"), TEXT("hindleg_upper_L"),
		  TEXT("hindleg_upper_R")})
	{
		const int32 LegIndex = RefSkeleton.FindBoneIndex(LegBone);
		TestTrue(FString::Printf(TEXT("%s bone exists"), LegBone), LegIndex != INDEX_NONE);
		if (LegIndex == INDEX_NONE)
		{
			continue;
		}
		FTransform LegTransform;
		FlyAnim->GetBoneTransform(LegTransform, FSkeletonPoseBoneIndex(LegIndex),
								  FAnimExtractContext(0.25), true);
		const FTransform& RefTransform = RefSkeleton.GetRefBonePose()[LegIndex];
		const double Tuck =
			LegTransform.GetRotation().AngularDistance(RefTransform.GetRotation());
		TestTrue(FString::Printf(TEXT("%s is tucked off bind in Fly (%.3f rad)"), LegBone, Tuck),
				 Tuck > 0.3);
	}
	// [2026-11-01 T-068] Real-span proof (v5 rig, Scripts/lengthen_wings.py):
	// [2026-11-01 T-068] compose every bone's local Fly transform up the
	// [2026-11-01 T-068] reference hierarchy and measure the distance between
	// [2026-11-01 T-068] the two wing_finger origins in the held Glide pose
	// [2026-11-01 T-068] (mid-flap Fly converges over the back). The v4 rig
	// [2026-11-01 T-068] (0.43-unit chain, tips beside the hips) cannot pass
	// [2026-11-01 T-068] the ratio below; v5 (0.85 chain) does. No pose-scale:
	// [2026-11-01 T-068] this measures bone lengths + rotations only.
	{
		UAnimSequence* GlideAnim = LoadObject<UAnimSequence>(
			nullptr, TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_GlideAnim.SK_Dragon_v4_GlideAnim"));
		TestNotNull(TEXT("SK_Dragon_v4_GlideAnim loads"), GlideAnim);
		const int32 NumBones = RefSkeleton.GetNum();
		TArray<FTransform> Component;
		Component.SetNum(NumBones);
		for (int32 Index = 0; Index < NumBones && GlideAnim != nullptr; ++Index)
		{
			FTransform Local;
			GlideAnim->GetBoneTransform(Local, FSkeletonPoseBoneIndex(Index), FAnimExtractContext(0.0),
										true);
			const int32 Parent = RefSkeleton.GetParentIndex(Index);
			Component[Index] = Parent == INDEX_NONE ? Local : Local * Component[Parent];
		}
		const int32 TipL = RefSkeleton.FindBoneIndex(TEXT("wing_finger_L"));
		const int32 TipR = RefSkeleton.FindBoneIndex(TEXT("wing_finger_R"));
		TestTrue(TEXT("wing finger bones exist"), TipL != INDEX_NONE && TipR != INDEX_NONE);
		if (TipL != INDEX_NONE && TipR != INDEX_NONE)
		{
			const double Span = FVector::Distance(Component[TipL].GetLocation(),
												  Component[TipR].GetLocation());
			const int32 PelvisIndex = RefSkeleton.FindBoneIndex(TEXT("pelvis"));
			const int32 HeadIndex = RefSkeleton.FindBoneIndex(TEXT("head"));
			const double Body = (PelvisIndex != INDEX_NONE && HeadIndex != INDEX_NONE)
									? FVector::Distance(Component[PelvisIndex].GetLocation(),
														Component[HeadIndex].GetLocation())
									: 0.0;
			// [2026-11-01 T-068] Ratio, not cm: FBX unit scale is an import
			// [2026-11-01 T-068] detail, span-vs-body is the design contract.
			// [2026-11-01 T-068] v4's ratio was ~0.55 (95 cm span measured on
			// [2026-11-01 T-068] v5 here vs ~55 by chain proportion); 0.9
			// [2026-11-01 T-068] separates the rigs with margin.
			TestTrue(FString::Printf(TEXT("wingtip span %.1f vs body %.1f (ratio %.2f)"), Span, Body,
									 Body > 0.0 ? Span / Body : 0.0),
					 Body > 0.0 && Span / Body > 0.9);
		}
		// [2026-11-02 T-068] Sweep proof (owner screenshot 2026-11-02: wings
		// [2026-11-02 T-068] straight but swept back delta-style in game).
		// [2026-11-02 T-068] Measure each fingertip in the body frame
		// [2026-11-02 T-068] (pelvis->head = forward): straight-out means the
		// [2026-11-02 T-068] forward offset from the shoulder is near zero
		// [2026-11-02 T-068] next to the lateral reach. Straightness alone
		// [2026-11-02 T-068] (below) cannot see sweep; this can.
		if (TipL != INDEX_NONE && TipR != INDEX_NONE)
		{
			const int32 PelvisIndex = RefSkeleton.FindBoneIndex(TEXT("pelvis"));
			const int32 HeadIndex = RefSkeleton.FindBoneIndex(TEXT("head"));
			const int32 ShoulderL = RefSkeleton.FindBoneIndex(TEXT("wing_shoulder_L"));
			const int32 ShoulderR = RefSkeleton.FindBoneIndex(TEXT("wing_shoulder_R"));
			if (PelvisIndex != INDEX_NONE && HeadIndex != INDEX_NONE && ShoulderL != INDEX_NONE &&
				ShoulderR != INDEX_NONE)
			{
				const FVector Forward =
					(Component[HeadIndex].GetLocation() - Component[PelvisIndex].GetLocation())
						.GetSafeNormal();
				for (const TTuple<int32, int32, const TCHAR*> Wing :
					 {MakeTuple(TipL, ShoulderL, TEXT("L")), MakeTuple(TipR, ShoulderR, TEXT("R"))})
				{
					const FVector Reach =
						Component[Wing.Get<0>()].GetLocation() - Component[Wing.Get<1>()].GetLocation();
					const double Fwd = FVector::DotProduct(Reach, Forward);
					const double Lateral = (Reach - Forward * Fwd).Size();
					TestTrue(FString::Printf(TEXT("wing %s points out not back (fwd %.1f lat %.1f ratio %.2f)"),
											 Wing.Get<2>(), Fwd, Lateral,
											 Lateral > 0.0 ? FMath::Abs(Fwd) / Lateral : 99.0),
							 Lateral > 0.0 && FMath::Abs(Fwd) / Lateral < 0.30);
				}
			}
		}
		// [2026-11-02 T-068] Straightness proof (owner playtest 2026-11-02: tips
		// [2026-11-02 T-068] straight out, fully extended, bird flap). Segment
		// [2026-11-02 T-068] directions between composed bone origins must stay
		// [2026-11-02 T-068] near-collinear in Glide: the shipped zigzag measured
		// [2026-11-02 T-068] 30-57 deg at the elbow in Blender's evaluated pose.
		if (TipL != INDEX_NONE && TipR != INDEX_NONE)
		{
			for (const TCHAR* Side : {TEXT("L"), TEXT("R")})
			{
				TArray<FVector> Points;
				for (const TCHAR* Base :
					 {TEXT("wing_shoulder"), TEXT("wing_elbow"), TEXT("wing_wrist"), TEXT("wing_finger")})
				{
					const int32 Idx = RefSkeleton.FindBoneIndex(FName(FString::Printf(TEXT("%s_%s"), Base, Side)));
					if (Idx != INDEX_NONE)
					{
						Points.Add(Component[Idx].GetLocation());
					}
				}
				TestEqual(FString::Printf(TEXT("wing chain %s has 4 joints"), Side), Points.Num(), 4);
				for (int32 Seg = 0; Seg + 2 < Points.Num(); ++Seg)
				{
					const FVector A = (Points[Seg + 1] - Points[Seg]).GetSafeNormal();
					const FVector B = (Points[Seg + 2] - Points[Seg + 1]).GetSafeNormal();
					const double Bend = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
						static_cast<double>(FVector::DotProduct(A, B)), -1.0, 1.0)));
					TestTrue(FString::Printf(TEXT("wing %s joint %d stays straight (%.1f deg)"), Side, Seg, Bend),
							 Bend < 25.0);
				}
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
