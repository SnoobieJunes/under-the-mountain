// Copyright Under the Mountain contributors.
// Purpose: T-068 animation-decision + new-verb contract tests — flap-rate
// curves, fold/tuck alphas, bank/pitch clamps, wing-buffet daze and tail-slam
// rear-arc damage, all headless against the pure seams (UDragonAnimInstance
// statics, UDragonCombatComponent, UThiefBrain). Created 2026-11-01 (T-068).

#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Dragon/DragonAnimInstance.h"
#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonCombatComponent.h"
#include "Dragon/DragonStatsComponent.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonAnimCurvesTest, "UnderTheMountain.Dragon.AnimCurves",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonAnimCurvesTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-068] Wingbeat curves: climb > cruise > sink, hover deep,
	// [2026-11-01 T-068] dive near-held, everything inside the hard clamp.
	const float Climb = UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 900.0f, 600.0f);
	const float Cruise = UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 900.0f, 0.0f);
	const float Sink = UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 900.0f, -600.0f);
	TestTrue(TEXT("climbing flaps faster than cruise"), Climb > Cruise);
	TestTrue(TEXT("cruise flaps faster than sinking"), Cruise > Sink);
	TestTrue(TEXT("hover beats faster than cruise"),
			 UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Hover, 0.0f, 0.0f) > Cruise);
	TestTrue(TEXT("dive is near-held, not fast-flapped"),
			 UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Dive, 3000.0f, -2000.0f) < 0.3f);
	TestTrue(TEXT("extreme climb stays clamped"),
			 UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 0.0f, 100000.0f) <= 1.8f);

	// [2026-11-01 T-068] Fold/tuck contract the baked clips satisfy.
	TestEqual(TEXT("ground wings folded"), UDragonAnimInstance::ComputeFoldAlpha(EDragonAnimState::Idle), 1.0f);
	TestEqual(TEXT("air wings spread"), UDragonAnimInstance::ComputeFoldAlpha(EDragonAnimState::Fly), 0.0f);
	TestEqual(TEXT("flight legs tucked"), UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState::Fly), 1.0f);
	TestEqual(TEXT("hover legs half-ready"), UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState::Hover), 0.5f);
	TestEqual(TEXT("ground legs deployed"), UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState::Walk), 0.0f);

	// [2026-11-01 T-068] Bank/pitch: zero without airspeed/turn, capped always.
	TestEqual(TEXT("no bank at rest"), UDragonAnimInstance::ComputeBankDegrees(90.0f, 0.0f), 0.0f);
	TestTrue(TEXT("hard turn banks but caps at 35"),
			 FMath::Abs(UDragonAnimInstance::ComputeBankDegrees(720.0f, 3000.0f)) <= 35.0f);
	TestEqual(TEXT("level flight no pitch"), UDragonAnimInstance::ComputeMeshPitchDegrees(1000.0f, 0.0f), 0.0f);
	TestTrue(TEXT("steep dive pitch caps at 30"),
			 UDragonAnimInstance::ComputeMeshPitchDegrees(100.0f, -5000.0f) >= -30.0f);
	TestTrue(TEXT("descending pitches nose-down"),
			 UDragonAnimInstance::ComputeMeshPitchDegrees(1000.0f, -500.0f) < 0.0f);

	// [2026-11-04 T-125] State order is serialized (logs name states by int;
	// ABP_Dragon transitions test the published LocomotionState): the T-043
	// values hold and Hover/Dive stay appended.
	TestEqual(TEXT("Idle is state 0"), static_cast<int32>(EDragonAnimState::Idle), 0);
	TestEqual(TEXT("Walk is state 1"), static_cast<int32>(EDragonAnimState::Walk), 1);
	TestEqual(TEXT("Fly is state 2"), static_cast<int32>(EDragonAnimState::Fly), 2);
	TestEqual(TEXT("Glide is state 3"), static_cast<int32>(EDragonAnimState::Glide), 3);
	TestEqual(TEXT("Hover is state 4"), static_cast<int32>(EDragonAnimState::Hover), 4);
	TestEqual(TEXT("Dive is state 5"), static_cast<int32>(EDragonAnimState::Dive), 5);

	// [2026-11-04 feel/spam] Hysteresis/dwell: held Glide exits only past
	// [2026-11-04 feel/spam] VS -50, held Hover outside 350/350, airborne
	// [2026-11-04 feel/spam] changes wait out the 0.3 s dwell, and Dive /
	// [2026-11-04 feel/spam] ground switches are never dwell-blocked.
	using Anim = UDragonAnimInstance;
	TestTrue(TEXT("held glide survives the -250..-50 band"),
			 Anim::SelectLocomotionState(EDragonAnimState::Glide, true, false, 900.0f, -100.0f, 1.0f) ==
				 EDragonAnimState::Glide);
	TestTrue(TEXT("held glide exits above -50"),
			 Anim::SelectLocomotionState(EDragonAnimState::Glide, true, false, 900.0f, 0.0f, 1.0f) ==
				 EDragonAnimState::Fly);
	TestTrue(TEXT("held hover survives to the 350 exit band"),
			 Anim::SelectLocomotionState(EDragonAnimState::Hover, true, false, 300.0f, 0.0f, 1.0f) ==
				 EDragonAnimState::Hover);
	TestTrue(TEXT("held hover exits past 350"),
			 Anim::SelectLocomotionState(EDragonAnimState::Hover, true, false, 400.0f, 0.0f, 1.0f) ==
				 EDragonAnimState::Fly);
	TestTrue(TEXT("dwell holds Fly against a fresh Hover candidate"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, true, false, 10.0f, 0.0f, 0.1f) ==
				 EDragonAnimState::Fly);
	TestTrue(TEXT("dwell elapsed lets Hover in"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, true, false, 10.0f, 0.0f, 0.3f) ==
				 EDragonAnimState::Hover);
	TestTrue(TEXT("dive is never dwell-blocked"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, true, true, 900.0f, -500.0f, 0.0f) ==
				 EDragonAnimState::Dive);
	TestTrue(TEXT("landing is never dwell-blocked"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, false, false, 0.0f, 0.0f, 0.0f) ==
				 EDragonAnimState::Idle);

	// [2026-11-04 feel/spam] Walk rate + combined slither bone rotation.
	TestEqual(TEXT("walk rate floors at 0.5"), Anim::ComputeWalkPlayRate(0.0f), 0.5f);
	TestEqual(TEXT("walk rate is 1.0 at 300 cm/s"), Anim::ComputeWalkPlayRate(300.0f), 1.0f);
	TestEqual(TEXT("walk rate caps at 1.5"), Anim::ComputeWalkPlayRate(900.0f), 1.5f);
	TestTrue(TEXT("slither bone segment 0 is neutral at phase 0"),
			 FMath::Abs(Anim::ComputeSlitherBoneRotation(0, 0.0f, 6.0f, 0.0f, 0.0f).Yaw) < 0.01f);
	TestTrue(TEXT("slither bone yaw lags with segment index"),
			 Anim::ComputeSlitherBoneRotation(3, 0.0f, 6.0f, 0.0f, 0.0f).Yaw < 0.0f);
	return true;
}

// [2026-11-04 WALKFIX] Walk-stride pin: the clip bound to the ABP Walk state
// (SK_Dragon_v4_Anim, the pawn's WalkAnim) must actually swing the leg
// bones through a stride. A clip whose leg tracks sit at bind reads
// in-game as a sliding statue — state=Walk, speed 600, feet planted —
// which is the owner-reported "dragon does NOT walk at all". Samples the
// raw clip (no world, no graph), so a red here isolates clip vs graph.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonWalkClipStrideTest, "UnderTheMountain.Dragon.WalkClipStride",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonWalkClipStrideTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for walk-clip check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	UAnimSequence* WalkAnim = Pawn->WalkAnim;
	TestNotNull(TEXT("walk clip is live on the pawn"), WalkAnim);
	USkeletalMesh* DragonMesh = Pawn->GetMesh()->GetSkeletalMeshAsset();
	TestNotNull(TEXT("pawn mesh loads for walk-clip check"), DragonMesh);
	if (WalkAnim == nullptr || DragonMesh == nullptr)
	{
		return false;
	}
	const USkeleton* Skeleton = DragonMesh->GetSkeleton();
	TestNotNull(TEXT("dragon skeleton loads"), Skeleton);
	if (Skeleton == nullptr)
	{
		return false;
	}
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const float Length = WalkAnim->GetPlayLength();
	// [2026-11-04 WALKFIX] Diagnostic: which bones DOES this clip move?
	{
		TArray<TPair<double, FString>> Movers;
		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			FQuat First;
			double Swing = 0.0;
			for (int32 Sample = 0; Sample < 8; ++Sample)
			{
				FTransform BoneTransform;
				WalkAnim->GetBoneTransform(BoneTransform, FSkeletonPoseBoneIndex(BoneIndex),
										   FAnimExtractContext(Length * Sample / 8.0f), true);
				if (Sample == 0)
				{
					First = BoneTransform.GetRotation();
				}
				Swing = FMath::Max(Swing, First.AngularDistance(BoneTransform.GetRotation()));
			}
			Movers.Add(TPair<double, FString>(Swing, RefSkeleton.GetBoneName(BoneIndex).ToString()));
		}
		Movers.Sort([](const TPair<double, FString>& A, const TPair<double, FString>& B)
					{ return A.Key > B.Key; });
		for (int32 Index = 0; Index < FMath::Min(10, Movers.Num()); ++Index)
		{
			UE_LOG(LogTemp, Display, TEXT("WALKCLIP mover %s swing=%.3f"), *Movers[Index].Value,
				   Movers[Index].Key);
		}
	}
	TestTrue(FString::Printf(TEXT("walk clip has length (%.2fs)"), Length), Length > 0.2f);
	int32 StridingLegs = 0;
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
		// Swing = widest rotation spread across 8 samples of the cycle.
		FQuat First;
		double MaxSwing = 0.0;
		for (int32 Sample = 0; Sample < 8; ++Sample)
		{
			FTransform BoneTransform;
			WalkAnim->GetBoneTransform(BoneTransform, FSkeletonPoseBoneIndex(LegIndex),
									   FAnimExtractContext(Length * Sample / 8.0f), true);
			if (Sample == 0)
			{
				First = BoneTransform.GetRotation();
			}
			MaxSwing = FMath::Max(MaxSwing, First.AngularDistance(BoneTransform.GetRotation()));
		}
		TestTrue(FString::Printf(TEXT("%s swings through the walk cycle (%.3f rad)"), LegBone,
								 MaxSwing),
				 MaxSwing > 0.15);
		if (MaxSwing > 0.15)
		{
			++StridingLegs;
		}
	}
	// [2026-11-04 ANIMFIX] All four legs must stride — the owner report was
	// front-only walking, so two striding legs is no longer enough.
	TestTrue(FString::Printf(TEXT("all four legs stride (%d)"), StridingLegs),
			 StridingLegs == 4);
	return true;
}

// [2026-11-04 ANIMFIX] Full-family audit: every shipped clip must move its
// signature bone(s) and (for looping families) close its loop seam. Logs a
// per-family ANIMAUDIT table (bone swings in rad) so a statue clip is
// visible in the log even before the assertion that catches it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonAnimFamilyAuditTest,
								 "UnderTheMountain.Dragon.AnimFamilyAudit",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonAnimFamilyAuditTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* DragonMesh = LoadObject<USkeletalMesh>(
		nullptr, TEXT("/Game/Meshy/Dragon/SK_Dragon_v6.SK_Dragon_v6"));
	TestNotNull(TEXT("SK_Dragon_v6 loads for family audit"), DragonMesh);
	if (DragonMesh == nullptr)
	{
		return false;
	}
	const FReferenceSkeleton& RefSkeleton = DragonMesh->GetSkeleton()->GetReferenceSkeleton();
	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		UE_LOG(LogTemp, Display, TEXT("ANIMAUDIT bone %s"),
			   *RefSkeleton.GetBoneName(BoneIndex).ToString());
	}

	auto BoneSwing = [&RefSkeleton](UAnimSequence* Anim, const TCHAR* BoneName,
									double* OutSeam) -> double
	{
		const int32 Index = RefSkeleton.FindBoneIndex(BoneName);
		if (Index == INDEX_NONE || Anim == nullptr)
		{
			if (OutSeam != nullptr)
			{
				*OutSeam = -1.0;
			}
			return -1.0;
		}
		const float Length = Anim->GetPlayLength();
		FQuat First;
		FQuat Last;
		double MaxSwing = 0.0;
		for (int32 Sample = 0; Sample <= 16; ++Sample)
		{
			FTransform BoneTransform;
			Anim->GetBoneTransform(BoneTransform, FSkeletonPoseBoneIndex(Index),
								   FAnimExtractContext(Length * Sample / 16.0f), true);
			if (Sample == 0)
			{
				First = BoneTransform.GetRotation();
			}
			if (Sample == 16)
			{
				Last = BoneTransform.GetRotation();
			}
			MaxSwing = FMath::Max(MaxSwing, First.AngularDistance(BoneTransform.GetRotation()));
		}
		if (OutSeam != nullptr)
		{
			*OutSeam = First.AngularDistance(Last);
		}
		return MaxSwing;
	};

	struct FFamily
	{
		const TCHAR* Name;
		const TCHAR* Path;
		const TCHAR* SignatureBone;
		double MinSwing;
		bool bLoop;
	};
	const FFamily Families[] = {
		{TEXT("Idle"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_IdleAnim.SK_Dragon_v4_IdleAnim"),
		 TEXT("chest"), 0.015, true},
		{TEXT("Walk"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_Anim.SK_Dragon_v4_Anim"),
		 TEXT("hindleg_upper_L"), 0.15, true},
		{TEXT("Fly"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_FlyAnim.SK_Dragon_v4_FlyAnim"),
		 TEXT("wing_shoulder_L"), 0.5, true},
		{TEXT("Glide"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_GlideAnim.SK_Dragon_v4_GlideAnim"),
		 TEXT("wing_shoulder_L"), 0.03, true},
		{TEXT("Hover"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_HoverAnim.SK_Dragon_v4_HoverAnim"),
		 TEXT("wing_shoulder_L"), 0.5, true},
		{TEXT("Takeoff"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_TakeoffAnim.SK_Dragon_v4_TakeoffAnim"),
		 TEXT("wing_shoulder_L"), 0.5, false},
		{TEXT("Land"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_LandAnim.SK_Dragon_v4_LandAnim"),
		 TEXT("wing_shoulder_L"), 0.5, false},
		{TEXT("Roar"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_RoarAnim.SK_Dragon_v4_RoarAnim"),
		 TEXT("neck"), 0.15, false},
		{TEXT("Hit"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_HitAnim.SK_Dragon_v4_HitAnim"),
		 TEXT("head"), 0.15, false},
		{TEXT("HitHeavy"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_HitHeavyAnim.SK_Dragon_v4_HitHeavyAnim"),
		 TEXT("head"), 0.3, false},
		{TEXT("Defeat"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_DefeatAnim.SK_Dragon_v4_DefeatAnim"),
		 TEXT("frontleg_upper_L"), 0.5, false},
		{TEXT("SleepIdle"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_SleepIdleAnim.SK_Dragon_v4_SleepIdleAnim"),
		 TEXT("chest"), 0.015, true},
		{TEXT("Wake"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_WakeAnim.SK_Dragon_v4_WakeAnim"),
		 TEXT("frontleg_upper_L"), 0.5, false},
		{TEXT("LieDown"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_LieDownAnim.SK_Dragon_v4_LieDownAnim"),
		 TEXT("frontleg_upper_L"), 0.5, false},
		{TEXT("FirePose"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_FirePoseAnim.SK_Dragon_v4_FirePoseAnim"),
		 TEXT("wing_shoulder_L"), 0.05, false},
		{TEXT("Claw"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_ClawAnim.SK_Dragon_v4_ClawAnim"),
		 TEXT("frontleg_upper_R"), 0.5, false},
		{TEXT("BiteEat"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_BiteEatAnim.SK_Dragon_v4_BiteEatAnim"),
		 TEXT("neck"), 0.3, false},
		{TEXT("TailWhip"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_TailWhipAnim.SK_Dragon_v4_TailWhipAnim"),
		 TEXT("tail_06"), 0.3, false},
		{TEXT("WingBuffet"), TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_WingBuffetAnim.SK_Dragon_v4_WingBuffetAnim"),
		 TEXT("wing_shoulder_L"), 0.5, false},
	};
	for (const FFamily& Family : Families)
	{
		UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, Family.Path);
		TestNotNull(FString::Printf(TEXT("%s clip loads"), Family.Name), Anim);
		if (Anim == nullptr)
		{
			continue;
		}
		double Seam = -1.0;
		const double Swing = BoneSwing(Anim, Family.SignatureBone, &Seam);
		// Top movers: what this clip actually animates, whatever its signature.
		TArray<TPair<double, FString>> Movers;
		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			const FString BoneName = RefSkeleton.GetBoneName(BoneIndex).ToString();
			Movers.Add(TPair<double, FString>(BoneSwing(Anim, *BoneName, nullptr), BoneName));
		}
		Movers.Sort([](const TPair<double, FString>& A, const TPair<double, FString>& B)
					{ return A.Key > B.Key; });
		FString Top;
		for (int32 Index = 0; Index < FMath::Min(3, Movers.Num()); ++Index)
		{
			Top += FString::Printf(TEXT("%s=%.3f "), *Movers[Index].Value, Movers[Index].Key);
		}
		UE_LOG(LogTemp, Display, TEXT("ANIMAUDIT %s top %s"), Family.Name, *Top);
		UE_LOG(LogTemp, Display,
			   TEXT("ANIMAUDIT %s len=%.2f signature=%s swing=%.3f seam=%.3f"), Family.Name,
			   Anim->GetPlayLength(), Family.SignatureBone, Swing, Seam);
		// [2026-11-04 ANIMFIX] Authored clips are 0.7-2.0 s; the 8 stale
		// shipped clips measure 10.38 s (never re-exported), which plays a
		// 1 s flap as a 10 s statue-drift. Length is part of the contract.
		TestTrue(FString::Printf(TEXT("%s has authored length (%.2fs)"), Family.Name,
								 Anim->GetPlayLength()),
				 Anim->GetPlayLength() > 0.2f && Anim->GetPlayLength() < 3.0f);
		TestTrue(FString::Printf(TEXT("%s signature %s moves (%.3f rad)"), Family.Name,
								 Family.SignatureBone, Swing),
				 Swing > Family.MinSwing);
		if (Family.bLoop)
		{
			TestTrue(FString::Printf(TEXT("%s loop seam closes (%.3f rad)"), Family.Name, Seam),
					 Seam >= 0.0 && Seam < 0.10);
		}
	}
	// Fly leg tuck (T-058 contract, also checked by FlyAnimPose): hind legs
	// must sit off bind in Fly, not dangle at bind while the wings flap.
	if (UAnimSequence* FlyAnim = LoadObject<UAnimSequence>(
			nullptr, TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_FlyAnim.SK_Dragon_v4_FlyAnim")))
	{
		for (const TCHAR* LegBone : {TEXT("hindleg_upper_L"), TEXT("hindleg_upper_R")})
		{
			const int32 LegIndex = RefSkeleton.FindBoneIndex(LegBone);
			if (LegIndex == INDEX_NONE)
			{
				TestTrue(FString::Printf(TEXT("%s exists"), LegBone), false);
				continue;
			}
			FTransform LegTransform;
			FlyAnim->GetBoneTransform(LegTransform, FSkeletonPoseBoneIndex(LegIndex),
									  FAnimExtractContext(0.25f), true);
			const double Tuck = LegTransform.GetRotation().AngularDistance(
				RefSkeleton.GetRefBonePose()[LegIndex].GetRotation());
			UE_LOG(LogTemp, Display, TEXT("ANIMAUDIT Fly tuck %s=%.3f"), LegBone, Tuck);
			TestTrue(FString::Printf(TEXT("Fly %s tucked off bind (%.3f rad)"), LegBone, Tuck),
					 Tuck > 0.3);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonBuffetSlamTest, "UnderTheMountain.Dragon.BuffetSlam",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonBuffetSlamTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-068] Brain fixtures mirror DragonCombatTest's burglar
	// [2026-11-01 T-068] (20 HP) against a one-treasure registry.
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1200;
	Crown.Location = FVector(100.0f, 0.0f, 0.0f);
	Registry->Register(Crown);

	auto MakeBrain = [Registry](const TCHAR* Name)
	{
		UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
		Config->MaxHP = 20.0f;
		Config->GrabTimeSeconds = 1.5f;
		Config->CarriedCap = 1;
		Config->bDropsAllLootOnRoar = true;
		Config->bCanPlaceTraps = false;
		Config->MaxTraps = 3;
		UThiefBrain* Brain = NewObject<UThiefBrain>();
		Brain->Init(Config, Registry, FName(Name));
		Brain->SetExitLocation(FVector(-9000.0f, 0.0f, 0.0f));
		return Brain;
	};

	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->Stamina = 100.0f;
	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward(1.0f, 0.0f, 0.0f);

	// [2026-11-01 T-068] Buffet: frontal thief dazed, rear thief untouched,
	// [2026-11-01 T-068] cooldown blocks an instant repeat for free.
	UThiefBrain* FrontBrain = MakeBrain(TEXT("FrontBurglar"));
	UThiefBrain* RearBrain = MakeBrain(TEXT("RearBurglar"));
	const TArray<UThiefBrain*> Brains = {FrontBrain, RearBrain};
	const TArray<FVector> Locations = {FVector(300.0f, 0.0f, 0.0f), FVector(-300.0f, 0.0f, 0.0f)};
	int32 NumDazed = -1;
	TestTrue(TEXT("buffet succeeds"),
			 Combat->TryWingBuffet(100.0f, Stats, Origin, Forward, Brains, Locations, NumDazed));
	TestEqual(TEXT("exactly the frontal thief is dazed"), NumDazed, 1);
	TestTrue(TEXT("frontal thief is Dazed"), FrontBrain->State() == EThiefState::Dazed);
	TestFalse(TEXT("rear thief is not dazed"), RearBrain->State() == EThiefState::Dazed);
	TestEqual(TEXT("buffet costs 8 stamina"), Stats->Stamina, 92.0f);
	int32 NumAgain = 0;
	TestFalse(TEXT("buffet on cooldown is blocked"),
			  Combat->TryWingBuffet(101.0f, Stats, Origin, Forward, Brains, Locations, NumAgain));
	TestEqual(TEXT("blocked buffet spends nothing"), Stats->Stamina, 92.0f);

	// [2026-11-01 T-068] Daze expiry: stands still, then flees.
	FrontBrain->Tick(1.0f, Locations[0]);
	TestTrue(TEXT("still dazed after 1s"), FrontBrain->State() == EThiefState::Dazed);
	FrontBrain->Tick(2.0f, Locations[0]);
	TestTrue(TEXT("daze fades into Flee"), FrontBrain->State() == EThiefState::Flee);

	// [2026-11-01 T-068] Tail slam: rear thief takes the GAMEPLAY 20 (lethal
	// [2026-11-01 T-068] to a 20 HP burglar), frontal thief is safe.
	int32 NumHit = -1;
	TestTrue(TEXT("tail slam succeeds"),
			 Combat->TryTailSlam(110.0f, Stats, Origin, Forward, Brains, Locations, NumHit));
	TestEqual(TEXT("exactly the rear thief is hit"), NumHit, 1);
	TestTrue(TEXT("rear thief died to 20 damage"), RearBrain->State() == EThiefState::Dead);
	TestEqual(TEXT("slam costs 4 stamina"), Stats->Stamina, 88.0f);

	// [2026-11-01 T-068] Arc edges: out of range never counts, point-blank does.
	TestFalse(TEXT("beyond buffet radius is outside"),
			  UDragonCombatComponent::IsInArc2D(Origin, Forward, FVector(701.0f, 0.0f, 0.0f), 700.0f, 70.0f));
	TestTrue(TEXT("point-blank is inside"),
			 UDragonCombatComponent::IsInArc2D(Origin, Forward, Origin, 700.0f, 70.0f));
	TestFalse(TEXT("directly behind is outside the frontal arc"),
			  UDragonCombatComponent::IsInArc2D(Origin, Forward, FVector(-300.0f, 0.0f, 0.0f), 700.0f, 70.0f));
	return true;
}

// [2026-11-04 T-125] Dual-mode contract: the pawn runs ABP_Dragon iff the
// graph asset is on disk (the constructor's class finder decides), else
// the shipped single-node driver. The expectation reads the same world
// the pawn saw (FindObject AFTER construction, so a landed graph that
// the constructor just loaded is visible) — future-proof across T-139,
// no hardcoded mode. Slot names pin the C++ side of the build-sheet
// contract (the graph's Slot nodes must carry these exact names).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonABPFallbackTest, "UnderTheMountain.Dragon.ABPFallback",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonABPFallbackTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for ABP check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	const UClass* AbpClass =
		FindObject<UClass>(nullptr, TEXT("/Game/Meshy/Dragon/ABP_Dragon.ABP_Dragon_C"));
	const bool bExpectABP = (AbpClass != nullptr);
	TestTrue(TEXT("ABP flag follows graph presence"), Pawn->IsUsingABP() == bExpectABP);
	const EAnimationMode::Type ExpectedMode = bExpectABP ? EAnimationMode::AnimationBlueprint
														 : EAnimationMode::AnimationSingleNode;
	TestTrue(TEXT("mesh mode follows ABP flag"),
			 Pawn->GetMesh()->GetAnimationMode() == ExpectedMode);
	TestTrue(TEXT("full-body slot name"), ADragonCharacter::AnimSlotFullBody == FName(TEXT("FullBody")));
	TestTrue(TEXT("upper-body slot name"),
			 ADragonCharacter::AnimSlotUpperBody == FName(TEXT("UpperBody")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
