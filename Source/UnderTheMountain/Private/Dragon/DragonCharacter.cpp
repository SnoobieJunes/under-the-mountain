// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Dragon walking-skeleton movement: ACharacter gives us
// walking + flying movement modes; flap/dive are velocity work on top, so
// T-011 feel tuning changes numbers, not architecture.
// [2026-10-28 T-011] The feel tuning that line promised: drain rate is
// chosen per tick from what the dragon is actually doing (dive/glide/
// hover), exhaustion burns HP, Landed() pays for hard impacts, and a
// debug readout makes velocity/drain visible while tuning in PIE.

#include "Dragon/DragonCharacter.h"

#include "Core/GameFlowMachine.h"
#include "Core/UnderTheMountainGameMode.h"
#include "Dragon/DragonAppearanceComponent.h"
#include "Dragon/OnboardingSaveGame.h"
#include "UI/OnboardingContent.h"
#include "UI/OnboardingWidget.h"
#include "Dragon/DragonCameraSaveGame.h"
#include "Dragon/DragonCombatComponent.h"
#include "Dragon/DragonMovementComponent.h"
#include "Dragon/DragonStatsComponent.h"
#include "Livestock/GoatActor.h"
#include "Scent/ScentComponent.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Thief/ThiefDataAsset.h"
#include "UI/DragonHUDWidget.h"
#include "UI/DragonTintWidget.h"
#include "UI/IntroChoiceWidget.h"
#include "UnderTheMountain.h"
#include "Hoard/HoardSubsystem.h"
#include "Intro/IntroSubsystem.h"
#include "Notoriety/NotorietySubsystem.h"
#include "Progression/XPSubsystem.h"
#include "Treasure/TreasureSubsystem.h"
#include "Waves/WaveSubsystem.h"
#include "World/BurnablePropActor.h"
#include "World/ScorchMarkActor.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Misc/VarArgs.h"
#include <cstdio>
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

namespace
{
// [2026-11-01 T-051] Dev/measurement switch: hold the breath
// visuals (Niagara + light) on without firing, for captures and
// fps measurement. Damage is never routed through this - it only
// feeds the presentation branch in Tick.
TAutoConsoleVariable<int32> CVarForceFireVisuals(
	TEXT("utm.fire.forcevisuals"), 0,
	TEXT("1 = hold fire-breath visuals on (Niagara stream + light) without firing. Dev/capture use only."));
bool ForceFireVisuals()
{
	return CVarForceFireVisuals.GetValueOnGameThread() != 0;
}

// [2026-11-01 T-052] Weather state (0 Clear, 1 Rain). Read by the
// pawn every Tick and applied on change, which makes the PIE
// console (`utm.weather.state 1`), the pawn's Exec commands, and
// headless -ExecCmds captures all drive the same state.
TAutoConsoleVariable<int32> CVarWeatherState(
	TEXT("utm.weather.state"), 0,
	TEXT("Weather state: 0 = Clear, 1 = Rain. Applied by the dragon pawn on change."));

// [2026-11-01 T-018] Intro master switch: 0 skips the wake-up
// sequence entirely (captures/tests that are not about the intro).
TAutoConsoleVariable<int32> CVarIntroEnabled(
	TEXT("utm.intro.enabled"), 1,
	TEXT("1 = play the intro wake-up sequence on spawn; 0 = straight to free play."));

// [2026-11-03 T-062] Scripted course runner: 1 = fly the legs and quit.
// Flies the current scheme's legs (default Scheme C hybrid); off by default.
TAutoConsoleVariable<int32> CVarFlightCourse(
	TEXT("utm.flight.course"), 0,
	TEXT("1 = fly the scripted T-075 course legs in the live world, dump CSV, quit."));

// [2026-11-03 T-065] Pitch-invert choices (Settings values until T-088's
// screen surfaces them). All default off = stick/mouse up is nose-up:
// mouse flight pitch, gamepad flight pitch (left stick), gamepad camera
// pitch (right stick).
TAutoConsoleVariable<int32> CVarInvertMousePitch(
	TEXT("utm.camera.invertMousePitch"), 0,
	TEXT("1 = mouse-up pitches the nose down (flight)."));
TAutoConsoleVariable<int32> CVarInvertPadFlight(
	TEXT("utm.camera.invertPadFlight"), 0,
	TEXT("1 = left-stick-up pitches the nose down (momentum band)."));
TAutoConsoleVariable<int32> CVarInvertPadCamera(
	TEXT("utm.camera.invertPadCamera"), 0,
	TEXT("1 = right-stick-up pitches the camera down."));
} // namespace

// [2026-11-03 T-075] Swap the movement component class for the prototype
// chassis (spike: UDragonMovementComponent/PhysCustom). Shipped tuning lines
// below address it through GetCharacterMovement(), unchanged.
ADragonCharacter::ADragonCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDragonMovementComponent>(
		  ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	Stats = CreateDefaultSubobject<UDragonStatsComponent>(TEXT("Stats"));
	// [2026-10-28 T-015] Combat verbs live on their own component; it
	// needs no BeginPlay (no world/time reads — the pawn injects both).
	Combat = CreateDefaultSubobject<UDragonCombatComponent>(TEXT("Combat"));
	Scent = CreateDefaultSubobject<UScentComponent>(TEXT("Scent"));
	Appearance = CreateDefaultSubobject<UDragonAppearanceComponent>(TEXT("Appearance"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);

	// [2026-11-01 T-051] Fire-breath presentation (replaces the T-015
	// cone mesh): a pooled Niagara stream at the nose plus a point
	// light, both attached to the capsule so capsule forward — the
	// pawn's +X — is the breath direction and the mesh's yaw/scale
	// can never skew it. The Niagara asset is authored outside code
	// (NS_FireBreath; see create_combat_assets.py); when it is absent
	// the pawn still breathes (damage is combat math, never the FX),
	// it just breathes invisibly - logged once below, not per tick.
	FireBreathFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireBreathFX"));
	FireBreathFX->SetupAttachment(GetCapsuleComponent());
	FireBreathFX->SetRelativeLocation(FVector(150.0f, 0.0f, 60.0f));
	FireBreathFX->SetAutoActivate(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FireSystem(
		TEXT("/Game/Dragon/NS_FireBreath.NS_FireBreath"));
	if (FireSystem.Succeeded())
	{
		FireBreathFX->SetAsset(FireSystem.Object);
	}
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(GetCapsuleComponent());
	FireLight->SetRelativeLocation(FVector(400.0f, 0.0f, 60.0f));
	FireLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.1f));
	FireLight->SetAttenuationRadius(1800.0f);
	FireLight->SetIntensity(0.0f);
	FireLight->SetVisibility(false);

	// [2026-11-01 T-052] Rain shower carried 9 m above the pawn
	// (NS_Rain authored like NS_FireBreath - recipe in
	// create_combat_assets.py's successor notes in create_sky.py's
	// header lineage; the pawn only activates it in Rain state).
	RainFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RainFX"));
	RainFX->SetupAttachment(GetCapsuleComponent());
	RainFX->SetRelativeLocation(FVector(0.0f, 0.0f, 900.0f));
	RainFX->SetAutoActivate(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> RainSystem(
		TEXT("/Game/Dragon/NS_Rain.NS_Rain"));
	if (RainSystem.Succeeded())
	{
		RainFX->SetAsset(RainSystem.Object);
	}

	// [2026-11-03 T-110] The real dragon is SK_Dragon_v6 (39-bone rig, 7 wing
	// [2026-11-03 T-110] bones/side + finger fan, decimated to 78,555 tris by
	// [2026-11-03 T-110] Scripts/decimate_dragon_v6.py, imported by
	// [2026-11-03 T-110] Scripts/import_dragon_v6.py). Measured v6 bounds match
	// [2026-11-03 T-110] v4 (local Y extent 95.14 vs 95.10, head at -Y in both
	// [2026-11-03 T-110] source rigs), so the T-032 offsets below transfer
	// [2026-11-03 T-110] unchanged: scale 2, z -24, yaw -90.
	// [2026-11-04 T-113] The newest owner drop re-uses this exact mesh
	// [2026-11-04 T-113] (proven identical: 289,485 verts / 413,454 tris /
	// [2026-11-04 T-113] same 39-bone rig), so SK_Dragon_v6 stands as
	// [2026-11-04 T-113] imported; T-113 retired the v4 mesh/skeleton and
	// [2026-11-04 T-113] the old v6 source beside it.
	// [2026-10-28 T-032] Probed bounds are 190 long (local Y, head at -Y
	// in the source rig) x 48 wide x 72 tall (cm), origin at body
	// centre — so scale 2 gives a 380cm dragon whose feet (local z -72
	// after scaling) sit exactly on the capsule floor at pawn z -96 when
	// the mesh is at z -24. Yaw -90 turns the mesh's -Y head onto the
	// pawn's +X forward. Collision stays on the capsule: the mesh is
	// display only.
	GetMesh()->SetVisibility(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DragonMesh(
		TEXT("/Game/Meshy/Dragon/SK_Dragon_v6.SK_Dragon_v6"));
	if (DragonMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(DragonMesh.Object);
	}
	else
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("SK_Dragon_v6 not found; dragon pawn will be invisible (capsule only)"));
	}
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -24.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimAsset(
		TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_Anim.SK_Dragon_v4_Anim"));
	if (WalkAnimAsset.Succeeded())
	{
		WalkAnim = WalkAnimAsset.Object;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}
	// [2026-10-28 T-043] Flight cycle, loaded like WalkAnim. The asset
	// is created by a separate import step; if it is absent FlyAnim
	// stays null (BeginPlay warns once, Tick falls back to bind pose).
	static ConstructorHelpers::FObjectFinder<UAnimSequence> FlyAnimAsset(
		TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_FlyAnim.SK_Dragon_v4_FlyAnim"));
	if (FlyAnimAsset.Succeeded())
	{
		FlyAnim = FlyAnimAsset.Object;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}
	// [2026-11-01 T-068] T-068 clips, loaded like FlyAnim; every one is
	// [2026-11-01 T-068] optional (Tick falls back per state) so a checkout
	// [2026-11-01 T-068] that has not run import_dragon_anims.py still works.
	{
		auto LoadClip = [this](const TCHAR* Path) -> UAnimSequence*
		{
			// [2026-11-01 T-068] Optional finder: a missing T-068 clip (import not
			// [2026-11-01 T-068] run yet) must not spam CDO errors; Tick's null
			// [2026-11-01 T-068] fallback is the contract.
			ConstructorHelpers::FObjectFinderOptional<UAnimSequence> Finder(Path);
			return Finder.Succeeded() ? Finder.Get() : nullptr;
		};
		GlideAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_GlideAnim.SK_Dragon_v4_GlideAnim"));
		HoverAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_HoverAnim.SK_Dragon_v4_HoverAnim"));
		IdleAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_IdleAnim.SK_Dragon_v4_IdleAnim"));
		TakeoffAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_TakeoffAnim.SK_Dragon_v4_TakeoffAnim"));
		LandAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_LandAnim.SK_Dragon_v4_LandAnim"));
		RoarAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_RoarAnim.SK_Dragon_v4_RoarAnim"));
		HitAnim = LoadClip(TEXT("/Game/Meshy/Dragon/SK_Dragon_v4_HitAnim.SK_Dragon_v4_HitAnim"));
	}
	// [2026-11-04 T-113] All nine clip paths hold the newest owner-drop
	// [2026-11-04 T-113] clips on this v6 skeleton (imported by Scripts/
	// [2026-11-04 T-113] import_newest_dragon_anims.py), so the gate below
	// [2026-11-04 T-113] passes them through and every flight state plays.
	// [2026-11-04 T-113] The gate stays: any future skeleton drift nulls the
	// [2026-11-04 T-113] drifting clip back to bind pose (loud in the log)
	// [2026-11-04 T-113] instead of playing garbage.
	if (GetMesh()->GetSkeletalMeshAsset() != nullptr)
	{
		USkeleton* MeshSkeleton = GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
		int32 SkippedClips = 0;
		for (TObjectPtr<UAnimSequence>* Clip :
			 {&WalkAnim, &FlyAnim, &GlideAnim, &HoverAnim, &IdleAnim, &TakeoffAnim, &LandAnim,
			  &RoarAnim, &HitAnim})
		{
			if (*Clip != nullptr && (*Clip)->GetSkeleton() != MeshSkeleton)
			{
				*Clip = nullptr;
				++SkippedClips;
			}
		}
		// [2026-11-04 T-113] Skeleton drift (a clip on the wrong rig plays
		// [2026-11-04 T-113] garbage): nulled here, loud in the log below.
		if (SkippedClips > 0)
		{
			UE_LOG(LogUnderTheMountain, Log,
				   TEXT("Dragon anims skipped: %d (skeleton mismatch vs %s — re-import the drifted clip)"),
				   SkippedClips, *GetMesh()->GetSkeletalMeshAsset()->GetName());
		}
	}

	// [2026-10-28 T-041] PlaceholderBody removed: the skeletal mesh
	// above is the visible body (T-032), and nothing referenced the
	// permanently hidden placeholder sphere any more.

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxFlySpeed = 1800.0f;
	Move->MaxWalkSpeed = 600.0f;
	Move->BrakingDecelerationFlying = 300.0f;
	bUseControllerRotationYaw = false;
	Move->bOrientRotationToMovement = true;

	// [2026-10-28 T-033] NOTE: input actions are NOT built here —
	// creating UObjects inside a class constructor asserts during
	// CDO creation (FObjectInitializer::AssertIfInConstructor, fatal
	// at module load; proven by the first T-033 attempt). They are
	// built lazily by EnsureInputActions() before first use.
}

bool ADragonCharacter::IsFlying() const
{
	return GetCharacterMovement()->MovementMode == MOVE_Flying;
}

EDragonAnimState ADragonCharacter::SelectAnimState(bool bFlying, bool bDiving, float HorizontalSpeed,
												   float VerticalSpeed)
{
	// [2026-11-01 T-068] Delegates to the AnimInstance statics (the single
	// [2026-11-01 T-068] owner of the rule); kept so the pawn's public seam
	// [2026-11-01 T-068] and its tests did not move when Hover/Dive landed.
	return UDragonAnimInstance::SelectLocomotionState(bFlying, bDiving, HorizontalSpeed, VerticalSpeed);
}

void ADragonCharacter::ApplyTrapSlow(float Factor, float Seconds)
{
	// [2026-10-28 T-014] First application captures the true speeds;
	// a re-trigger while slowed only refreshes the clock so the
	// restore point can never be a slowed value.
	// [2026-11-03 T-103] The hybrid chassis ignores MaxFlySpeed, so the
	// same first-application rule carries the hybrid thrust factor
	// (1.0 = unslowed): slowed flight scales precision thrust, impulses
	// and rise rate in PhysCustom until TickTrapSlow restores it.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (TrapSlowTimeRemaining <= 0.0f)
	{
		UnslowedWalkSpeed = Move->MaxWalkSpeed;
		UnslowedFlySpeed = Move->MaxFlySpeed;
		Move->MaxWalkSpeed *= Factor;
		Move->MaxFlySpeed *= Factor;
		if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(Move))
		{
			Proto->TrapSlowFactor = Factor;
		}
	}
	TrapSlowTimeRemaining = FMath::Max(TrapSlowTimeRemaining, Seconds);
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon trap-slowed: factor %.2f for %.1fs"), Factor, Seconds);
}

void ADragonCharacter::TickTrapSlow(float DeltaSeconds)
{
	// [2026-11-03 T-103] Trap-slow clock + expiry. Restores the captured
	// speeds AND the hybrid thrust factor in the same call the clock runs
	// out, so a slowed dragon can never keep slowed values past expiry.
	// World-free by construction (movement component + floats + one log
	// line): the automation test drives slow/restore directly.
	if (TrapSlowTimeRemaining <= 0.0f)
	{
		return;
	}
	TrapSlowTimeRemaining -= DeltaSeconds;
	if (TrapSlowTimeRemaining > 0.0f)
	{
		return;
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = UnslowedWalkSpeed;
		Move->MaxFlySpeed = UnslowedFlySpeed;
		if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(Move))
		{
			Proto->TrapSlowFactor = 1.0f;
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon trap slow expired; speeds restored"));
}

void ADragonCharacter::FlightStateFlags(bool& bOutDiving, bool& bOutBraking, bool& bOutHovering) const
{
	// [2026-11-03 T-064] One classification for every reader (HUD feed,
	// debug readout, and the FormatFlightState test contract): dive is
	// the shared posture rule, brake is the held brake input that is not
	// already a dive, hover is the precision band. Grounded leaves all
	// three false so callers show nothing.
	bOutDiving = false;
	bOutBraking = false;
	bOutHovering = false;
	if (IsPrototypeFlying())
	{
		if (const UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement()))
		{
			const float Airspeed = Proto->Velocity.Size();
			bOutDiving = UDragonMovementComponent::IsDivePosture(
				GetControlRotation().Pitch, Airspeed, Proto->FlightParams);
			bOutBraking = bDiving && !bOutDiving;
			bOutHovering = !bOutDiving && !bOutBraking &&
						   UDragonMovementComponent::PrecisionAuthorityWeight(Airspeed, Proto->FlightParams) > 0.5f;
		}
	}
	else if (IsFlying())
	{
		// Legacy scheme 0: the held Shift input IS the dive (camera-forward
		// rocket, preserved as the baseline); hover is the slow band.
		bOutDiving = bDiving;
		bOutHovering = !bDiving && GetCharacterMovement()->Velocity.Size2D() < Stats->HoverSpeedThreshold;
	}
}

bool ADragonCharacter::InvertMousePitch()
{
	return CVarInvertMousePitch.GetValueOnGameThread() != 0;
}

bool ADragonCharacter::InvertPadFlightPitch()
{
	return CVarInvertPadFlight.GetValueOnGameThread() != 0;
}

bool ADragonCharacter::InvertPadCameraPitch()
{
	return CVarInvertPadCamera.GetValueOnGameThread() != 0;
}

EDragonCameraMode ADragonCharacter::GetCameraMode() const
{
	return CameraMode;
}

void ADragonCharacter::SetCameraSaveSlotForTesting(const FString& SlotName)
{
	CameraSaveSlotName = SlotName;
}

void ADragonCharacter::HandleCameraCycle()
{
	// [2026-11-03 T-065] Steps the adopted set and persists the choice, so
	// the next session opens on the same camera.
	CycleCamera();
}

void ADragonCharacter::HandlePause()
{
	// [2026-11-03 T-072] Routing by flow state lives here so one key
	// does the right thing on every screen: pause/resume in play,
	// back out to title from an end screen, nothing on title (the
	// Start button owns that transition).
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	AUnderTheMountainGameMode* Mode =
		World->GetAuthGameMode<AUnderTheMountainGameMode>();
	if (Mode == nullptr)
	{
		return;
	}
	const UGameFlowMachine::EState State = Mode->GetFlowState();
	if (State == UGameFlowMachine::EState::Playing ||
		State == UGameFlowMachine::EState::Paused)
	{
		Mode->TogglePauseRun();
	}
	else if (State == UGameFlowMachine::EState::Won || State == UGameFlowMachine::EState::Lost)
	{
		Mode->QuitToTitle();
	}
}

void ADragonCharacter::HandleHelp()
{
	// [2026-11-03 T-073] Non-modal by widget construction (hit-test
	// invisible prompt line, scroll box the game runs behind): toggling
	// never pauses, never steals input.
	if (OnboardingWidget != nullptr)
	{
		OnboardingWidget->ToggleHelp();
	}
}

void ADragonCharacter::LoadOnboardingProgress()
{
	for (int32 I = 0; I < static_cast<int32>(EOnboardingVerb::Count); ++I)
	{
		OnboardingDone[I] = false;
	}
	if (UGameplayStatics::DoesSaveGameExist(OnboardingSaveSlotName, 0))
	{
		if (UOnboardingSaveGame* Save = Cast<UOnboardingSaveGame>(
				UGameplayStatics::LoadGameFromSlot(OnboardingSaveSlotName, 0)))
		{
			const int32 N = FMath::Min(static_cast<int32>(EOnboardingVerb::Count),
									   Save->VerbDone.Num());
			for (int32 I = 0; I < N; ++I)
			{
				OnboardingDone[I] = Save->VerbDone[I];
			}
		}
	}
}

void ADragonCharacter::MarkOnboardingDone(EOnboardingVerb Verb)
{
	const int32 I = static_cast<int32>(Verb);
	if (I < 0 || I >= static_cast<int32>(EOnboardingVerb::Count) || OnboardingDone[I])
	{
		return;
	}
	OnboardingDone[I] = true;
	// [2026-11-03 T-073] Persist every first success (slot choice
	// recorded on the member): prompts never reappear, across sessions.
	if (UOnboardingSaveGame* Save = Cast<UOnboardingSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UOnboardingSaveGame::StaticClass())))
	{
		Save->VerbDone.SetNum(static_cast<int32>(EOnboardingVerb::Count));
		for (int32 J = 0; J < static_cast<int32>(EOnboardingVerb::Count); ++J)
		{
			Save->VerbDone[J] = OnboardingDone[J];
		}
		UGameplayStatics::SaveGameToSlot(Save, OnboardingSaveSlotName, 0);
	}
}

void ADragonCharacter::TickOnboarding(float DeltaSeconds)
{
	// [2026-11-03 T-073] 1 Hz evaluation, gated to real play: the run
	// must have started (flow Playing — paused worlds never tick this
	// far anyway) and the intro must be done, and there must be work
	// left. The pure arbiter picks; this block only gathers context
	// and applies the verdict to the widget.
	bool bIncomplete = false;
	for (int32 I = 0; I < static_cast<int32>(EOnboardingVerb::Count); ++I)
	{
		bIncomplete = bIncomplete || !OnboardingDone[I];
	}
	if (!bIncomplete)
	{
		if (CurrentPrompt != EOnboardingVerb::Count && OnboardingWidget != nullptr)
		{
			OnboardingWidget->ClearPrompt();
			CurrentPrompt = EOnboardingVerb::Count;
		}
		return;
	}
	UWorld* World = GetWorld();
	if (World == nullptr || OnboardingWidget == nullptr)
	{
		return;
	}
	const AUnderTheMountainGameMode* Mode =
		World->GetAuthGameMode<AUnderTheMountainGameMode>();
	if (Mode == nullptr || Mode->GetFlowState() != UGameFlowMachine::EState::Playing)
	{
		return;
	}
	const UIntroSubsystem* Intro = World->GetSubsystem<UIntroSubsystem>();
	if (Intro != nullptr && Intro->GetState() != EIntroState::FreePlay)
	{
		return;
	}
	OnboardingTimer += DeltaSeconds;
	if (OnboardingTimer < 1.0f)
	{
		return;
	}
	OnboardingTimer = 0.0f;

	const FVector Origin = GetActorLocation();
	const float FireRangeSq = Combat != nullptr ? Combat->FireRange * Combat->FireRange : 0.0f;
	const float RoarRangeSq =
		Combat != nullptr ? Combat->RoarRadius * Combat->RoarRadius : 0.0f;
	const float EatRangeSq = Combat != nullptr ? Combat->EatRange * Combat->EatRange : 0.0f;
	bool bFireTarget = false;
	bool bThiefNear = false;
	bool bGoatNear = false;
	TArray<ABurnablePropActor*> Props;
	TArray<FVector> PropLocations;
	GatherBurnableTargets(Props, PropLocations);
	for (int32 I = 0; I < PropLocations.Num() && !bFireTarget; ++I)
	{
		bFireTarget = FVector::DistSquared(Origin, PropLocations[I]) <= FireRangeSq;
	}
	TArray<AActor*> ThiefActors;
	UGameplayStatics::GetAllActorsOfClass(World, AThiefCharacter::StaticClass(), ThiefActors);
	for (AActor* Actor : ThiefActors)
	{
		if (Actor != nullptr &&
			FVector::DistSquared(Origin, Actor->GetActorLocation()) <= RoarRangeSq)
		{
			bThiefNear = true;
			break;
		}
	}
	TArray<AGoatActor*> Goats;
	TArray<FVector> GoatLocations;
	GatherGoatTargets(Goats, GoatLocations);
	for (int32 I = 0; I < GoatLocations.Num() && !bGoatNear; ++I)
	{
		bGoatNear = FVector::DistSquared(Origin, GoatLocations[I]) <= EatRangeSq;
	}
	bool bDivingNow = false;
	bool bUnusedBrake = false;
	bool bUnusedHover = false;
	FlightStateFlags(bDivingNow, bUnusedBrake, bUnusedHover);
	if (bDivingNow)
	{
		MarkOnboardingDone(EOnboardingVerb::Dive);
	}
	const EOnboardingVerb Next =
		NextOnboardingPrompt(OnboardingDone, bFireTarget, bThiefNear, bGoatNear);
	if (Next != CurrentPrompt)
	{
		CurrentPrompt = Next;
		if (Next == EOnboardingVerb::Count)
		{
			OnboardingWidget->ClearPrompt();
			return;
		}
		for (const FOnboardingPromptDef& Def : GetOnboardingPrompts())
		{
			if (Def.Verb == Next)
			{
				UE_LOG(LogUnderTheMountain, Log, TEXT("Onboarding prompt: %s"), *Def.KeyboardText);
				OnboardingWidget->ShowPrompt(Def);
				break;
			}
		}
	}
}

void ADragonCharacter::CycleCamera()
{
	CameraMode = static_cast<EDragonCameraMode>((static_cast<int32>(CameraMode) + 1) % 3);
	SaveCameraChoice();
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon camera: %d"), static_cast<int32>(CameraMode));
	// [2026-11-03 T-073] First cycle dismisses the camera prompt.
	MarkOnboardingDone(EOnboardingVerb::Camera);
}

void ADragonCharacter::SaveCameraChoice() const
{
	UDragonCameraSaveGame* Save = Cast<UDragonCameraSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UDragonCameraSaveGame::StaticClass()));
	if (Save == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Camera save FAILED: could not create save object"));
		return;
	}
	Save->CameraModeIndex = static_cast<int32>(CameraMode);
	if (!UGameplayStatics::SaveGameToSlot(Save, CameraSaveSlotName, 0))
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Camera save FAILED: slot '%s' refused the write"),
			   *CameraSaveSlotName);
	}
}

void ADragonCharacter::LoadCameraChoice()
{
	// [2026-11-03 T-065] Missing slot = first run: keep the default head
	// chase silently. A slot that exists but loads wrong is corruption —
	// warn naming the slot (the T-039 tint-slot lesson).
	if (!UGameplayStatics::DoesSaveGameExist(CameraSaveSlotName, 0))
	{
		CameraMode = EDragonCameraMode::HeadChase;
		return;
	}
	if (UDragonCameraSaveGame* Loaded = Cast<UDragonCameraSaveGame>(
			UGameplayStatics::LoadGameFromSlot(CameraSaveSlotName, 0)))
	{
		const int32 Index = Loaded->CameraModeIndex;
		if (Index >= 0 && Index <= 2)
		{
			CameraMode = static_cast<EDragonCameraMode>(Index);
		}
		else
		{
			UE_LOG(LogUnderTheMountain, Warning,
				   TEXT("Camera load: slot '%s' holds out-of-range index %d; keeping head chase"),
				   *CameraSaveSlotName, Index);
			CameraMode = EDragonCameraMode::HeadChase;
		}
	}
	else
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Camera load FAILED: slot '%s' exists but could not be loaded; keeping head chase"),
			   *CameraSaveSlotName);
		CameraMode = EDragonCameraMode::HeadChase;
	}
}

void ADragonCharacter::ApplyCameraRig(float DeltaSeconds)
{
	// [2026-11-03 T-065] Per-tick rig: pivot follows the head bone so the
	// chase orbits the eyes (not the capsule); arm shortens in the
	// precision band; first-person hides the pawn mesh (head interior would
	// fill the frame — fire/rain FX are capsule-attached and stay visible).
	// Stall needs no work here: the boom rides control rotation, which never
	// follows the sinking mesh nose. Null-safe throughout: a missing bone
	// keeps the last pivot instead of snapping anywhere.
	if (CameraBoom == nullptr || GetMesh() == nullptr || GetCapsuleComponent() == nullptr)
	{
		return;
	}
	if (GetMesh()->GetBoneIndex(TEXT("head")) != INDEX_NONE)
	{
		const FVector HeadWorld = GetMesh()->GetBoneLocation(TEXT("head"));
		const FVector HeadLocal =
			GetCapsuleComponent()->GetComponentTransform().InverseTransformPosition(HeadWorld);
		CameraBoom->SetRelativeLocation(HeadLocal);
	}
	float DesiredArm = 500.0f;
	switch (CameraMode)
	{
	case EDragonCameraMode::WideScenic:
		DesiredArm = 1200.0f;
		break;
	case EDragonCameraMode::FirstPerson:
		DesiredArm = 0.0f;
		break;
	case EDragonCameraMode::HeadChase:
	default:
		DesiredArm = 500.0f;
		break;
	}
	if (CameraMode != EDragonCameraMode::FirstPerson && IsPrototypeFlying())
	{
		if (const UDragonMovementComponent* Proto =
				Cast<UDragonMovementComponent>(GetCharacterMovement()))
		{
			const float AuthorityW = UDragonMovementComponent::PrecisionAuthorityWeight(
				Proto->Velocity.Size(), Proto->FlightParams);
			DesiredArm = FMath::Max(120.0f, DesiredArm * (1.0f - 0.5f * AuthorityW));
		}
	}
	CameraBoom->TargetArmLength =
		FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredArm, DeltaSeconds, 6.0f);
	const bool bHideMesh = (CameraMode == EDragonCameraMode::FirstPerson);
	if (bHideMesh != bMeshHiddenForCamera)
	{
		bMeshHiddenForCamera = bHideMesh;
		GetMesh()->SetVisibility(!bHideMesh);
	}
}

bool ADragonCharacter::HasInputActions() const
{
	// [2026-10-28 T-033] Non-null check for the full code-created
	// input set; EnsureInputActions must have run (it does, at the
	// top of SetupPlayerInputComponent and BeginPlay).
	// [2026-10-28 T-015] Fire/Roar/Eat join the contract: Ensure must
	// not report ready while a combat action is still null.
	return MoveAction != nullptr && LookAction != nullptr && FlapAction != nullptr &&
		   ToggleFlightAction != nullptr && DiveAction != nullptr && TrueSightAction != nullptr &&
		   ScentToggleAction != nullptr &&
		   TintAction != nullptr && FireAction != nullptr && RoarAction != nullptr &&
		   EatAction != nullptr && WingBuffetAction != nullptr && TailSlamAction != nullptr &&
		   FreeLookAction != nullptr && SinkAction != nullptr && SteerLatchAction != nullptr &&
		   // [2026-11-03 T-065] Split look/move + camera cycle join the contract.
		   PadLookAction != nullptr && PadMoveAction != nullptr && CameraCycleAction != nullptr &&
		   PauseAction != nullptr && HelpAction != nullptr &&
		   MappingContext != nullptr;
}

void ADragonCharacter::EnsureInputActions()
{
	// [2026-10-28 T-033] Lazy build, idempotent: the original bug
	// was SetupPlayerInputComponent binding null actions (created
	// only in BeginPlay, which can run after possession). Building
	// here — first thing in both entry points — guarantees the
	// pointers BindAction stores are the real actions.
	if (!HasInputActions())
	{
		BuildInputActions();
	}
}

void ADragonCharacter::BeginPlay()
{
	Super::BeginPlay();
	// [2026-10-28 T-033] Ensure before registering the context (see
	// EnsureInputActions); SetupPlayerInputComponent does the same
	// before binding, so either order is now safe.
	EnsureInputActions();
	// [2026-11-03 T-065] Restored camera before anything reads the rig so
	// the first tick already orbits the persisted choice.
	LoadCameraChoice();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (MappingContext != nullptr)
			{
				Sub->AddMappingContext(MappingContext, 0);
			}
		}
	}
	// [2026-10-28 T-043] Surface a missing flight clip once here;
	// Tick's per-frame fallback to bind pose must stay silent.
	// [2026-11-04 T-113] T-113 landed the newest clips, so a null FlyAnim now
	// [2026-11-04 T-113] means the import regressed — loud, as before.
	if (FlyAnim == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("No compatible flight clip (newest import missing); flight will use bind pose"));
	}
	// [2026-11-03 T-073] Onboarding: load persisted progress and build
	// the (hidden) prompt + help widget before the first tick evaluates.
	LoadOnboardingProgress();
	OnboardingWidget =
		CreateWidget<UOnboardingWidget>(GetWorld(), UOnboardingWidget::StaticClass());
	if (OnboardingWidget != nullptr)
	{
		OnboardingWidget->AddToViewport(90);
		OnboardingWidget->ClearPrompt();
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon pawn spawned at %s"), *GetActorLocation().ToString());
	// [2026-11-03 T-105] The landing curve lives in Stats alone: push its
	// threshold into the movement params cache (the integrator's
	// floor-contact routing reads the cache; LandingSeverity reads Stats
	// directly). Tune Stats, never the cache.
	if (Stats != nullptr)
	{
		if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement()))
		{
			Proto->FlightParams.CachedLandingSpeedThreshold = Stats->LandingSpeedThreshold;
			// [2026-11-03 T-079] Wall-impact pricing reads the same
			// curve (movement reads Stats, never the reverse).
			Proto->CurveSource = Stats;
		}
	}
	// [2026-11-01 T-052] Initial weather from the cvar (0 Clear
	// unless a capture/console asked for Rain before spawn).
	ApplyWeather(FMath::Clamp(CVarWeatherState.GetValueOnGameThread(), 0, 1));
}

void ADragonCharacter::Weather()
{
	SetWeather(CurrentWeatherState == 1 ? 0 : 1);
}

void ADragonCharacter::SetWeather(int32 State)
{
	ApplyWeather(FMath::Clamp(State, 0, 1));
	// Keep the cvar in step so Tick's change-detection does not
	// immediately re-apply the console's older value.
	CVarWeatherState->Set(CurrentWeatherState, ECVF_SetByConsole);
}

void ADragonCharacter::BurnTest()
{
	TArray<ABurnablePropActor*> Props;
	TArray<FVector> Locations;
	GatherBurnableTargets(Props, Locations);
	int32 Ignited = 0;
	for (int32 Index = 0; Index < Props.Num(); ++Index)
	{
		if (Props[Index] != nullptr && Locations.IsValidIndex(Index) &&
			FVector::Dist(Locations[Index], GetActorLocation()) <= 3000.0f)
		{
			Props[Index]->IgniteNow();
			++Ignited;
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("BurnTest: ignited %d prop(s)"), Ignited);
}

void ADragonCharacter::ApplyWeather(int32 NewState)
{
	CurrentWeatherState = NewState;
	const bool bRain = (NewState == 1);
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	// [2026-11-01 T-052] Sky actors are found by class each apply
	// (weather switches are rare; a cached pointer would go stale
	// across level loads). Only the exterior sun/sky/fog are
	// touched - interior point lights are a different class and
	// the lair's boxes shadow the sun, so the interior is
	// unaffected. Rain: dim blue-grey sun, dimmer sky light,
	// ~4.7x fog density (the wet, closed-in look), rain FX on.
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			Sun->SetIntensity(bRain ? 0.7f : 3.0f);
			Sun->SetLightColor(bRain ? FLinearColor(0.62f, 0.7f, 0.85f)
									 : FLinearColor(1.0f, 0.95f, 0.88f));
		}
	}
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		if (USkyLightComponent* Sky = Cast<USkyLightComponent>(It->GetLightComponent()))
		{
			Sky->SetIntensity(bRain ? 0.4f : 1.0f);
		}
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		if (UExponentialHeightFogComponent* Fog = It->GetComponent())
		{
			Fog->SetFogDensity(bRain ? 0.0028f : 0.0006f);
		}
	}
	if (RainFX != nullptr)
	{
		if (bRain && !RainFX->IsActive())
		{
			RainFX->Activate(true);
		}
		else if (!bRain && RainFX->IsActive())
		{
			RainFX->Deactivate();
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Weather: %s"), bRain ? TEXT("Rain") : TEXT("Clear"));
}

void ADragonCharacter::BuildInputActions()
{
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	FlapAction = MakeAction(TEXT("IA_Flap"), EInputActionValueType::Boolean);
	ToggleFlightAction = MakeAction(TEXT("IA_ToggleFlight"), EInputActionValueType::Boolean);
	DiveAction = MakeAction(TEXT("IA_Dive"), EInputActionValueType::Boolean);
	// [2026-11-03 T-075] Prototype-only actions (T-075/T-063): RMB free-look,
	// Ctrl sink, D-pad Up steer latch. Handlers gate on the scheme, so the
	// shipped map gains bindings but no behavior.
	FreeLookAction = MakeAction(TEXT("IA_FreeLook"), EInputActionValueType::Boolean);
	SinkAction = MakeAction(TEXT("IA_Sink"), EInputActionValueType::Boolean);
	SteerLatchAction = MakeAction(TEXT("IA_SteerLatch"), EInputActionValueType::Boolean);
	TrueSightAction = MakeAction(TEXT("IA_TrueSight"), EInputActionValueType::Boolean);
	// [2026-11-03 T-061] Scent-layer toggle, press-edge like true-sight.
	ScentToggleAction = MakeAction(TEXT("IA_ScentToggle"), EInputActionValueType::Boolean);
	TintAction = MakeAction(TEXT("IA_TintPicker"), EInputActionValueType::Boolean);
	// [2026-10-28 T-015] Combat verbs: Boolean like Flap — Fire reads
	// the held value, Roar/Eat act on the press edge.
	FireAction = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	RoarAction = MakeAction(TEXT("IA_Roar"), EInputActionValueType::Boolean);
	EatAction = MakeAction(TEXT("IA_Eat"), EInputActionValueType::Boolean);
	// [2026-11-01 T-068] Buffet/slam verbs, press-edge like Roar.
	WingBuffetAction = MakeAction(TEXT("IA_WingBuffet"), EInputActionValueType::Boolean);
	TailSlamAction = MakeAction(TEXT("IA_TailSlam"), EInputActionValueType::Boolean);
	// [2026-11-03 T-065] Split look/move (mouse vs sticks need separate
	// pitch-invert choices; the left stick needs its own flight-pitch
	// feed) plus the camera-cycle action. All Axis2D/Boolean like the
	// actions they split from.
	PadLookAction = MakeAction(TEXT("IA_PadLook"), EInputActionValueType::Axis2D);
	PadMoveAction = MakeAction(TEXT("IA_PadMove"), EInputActionValueType::Axis2D);
	CameraCycleAction = MakeAction(TEXT("IA_CameraCycle"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Dragon"));
	auto Map = [this](UInputAction* Action, FKey Key)
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(Action, Key);
		return &M;
	};
	// Keyboard/mouse: WASD (W forward swizzled from Y), mouse look, Space flap,
	// F toggle flight, Left Shift dive.
	// [2026-10-27 T-003] Digital keys arrive on the X channel: A/D feed X
	// (A negated), W/S are swizzled into Y (S also negated).
	Map(MoveAction, EKeys::A)->Modifiers.Add(NewObject<UInputModifierNegate>(this));
	Map(MoveAction, EKeys::D);
	Map(MoveAction, EKeys::W)->Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
	{
		FEnhancedActionKeyMapping& M = *Map(MoveAction, EKeys::S);
		M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}
	Map(LookAction, EKeys::Mouse2D);
	Map(FlapAction, EKeys::SpaceBar);
	Map(ToggleFlightAction, EKeys::F);
	Map(DiveAction, EKeys::LeftShift);
	// [2026-11-03 T-075] Prototype keys: RMB free-look, Ctrl sink.
	Map(FreeLookAction, EKeys::RightMouseButton);
	Map(SinkAction, EKeys::LeftControl);
	// [2026-11-03 T-075] D-pad Up was reserved for T-063 steer-to-camera —
	// the prototype claims it; T-061 keeps D-pad Down.
	Map(SteerLatchAction, EKeys::Gamepad_DPad_Up);
	Map(TrueSightAction, EKeys::T);
	// [2026-11-03 T-061] N (nose) is free in both the shipped map and
	// the adopted flight map; D-pad Down was reserved for this toggle
	// by the flight decisions (Up went to T-063 steer-to-camera).
	Map(ScentToggleAction, EKeys::N);
	Map(TintAction, EKeys::C);
	// [2026-10-28 T-015] Combat keys: LMB hold = fire, R = roar, E = eat.
	Map(FireAction, EKeys::LeftMouseButton);
	Map(RoarAction, EKeys::R);
	Map(EatAction, EKeys::E);
	// [2026-11-01 T-068] X = wing buffet, Z = tail slam (Q stays reserved
	// [2026-11-01 T-068] for T-076 claw; both free in the shipped map).
	Map(WingBuffetAction, EKeys::X);
	Map(TailSlamAction, EKeys::Z);
	// Gamepad: left stick move, right stick look, bottom face flap,
	// top face toggle flight, right trigger dive.
	// [2026-11-03 T-065] Sticks live on their own actions (see creation):
	// the shared actions stay keyboard/mouse-only so each device gets its
	// own pitch-invert choice and the left stick gets its flight-pitch feed.
	Map(PadMoveAction, EKeys::Gamepad_Left2D);
	Map(PadLookAction, EKeys::Gamepad_Right2D);
	Map(FlapAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(ToggleFlightAction, EKeys::Gamepad_FaceButton_Top);
	Map(DiveAction, EKeys::Gamepad_RightTrigger);
	Map(TrueSightAction, EKeys::Gamepad_FaceButton_Left);
	Map(ScentToggleAction, EKeys::Gamepad_DPad_Down);
	Map(TintAction, EKeys::Gamepad_FaceButton_Right);
	// [2026-10-28 T-015] Combat pad: left trigger hold = fire (right
	// trigger is dive), left shoulder = roar, right shoulder = eat.
	Map(FireAction, EKeys::Gamepad_LeftTrigger);
	Map(RoarAction, EKeys::Gamepad_LeftShoulder);
	Map(EatAction, EKeys::Gamepad_RightShoulder);
	// [2026-11-01 T-068] Pad: DPad Left/Right (Down/Up stay reserved for
	// [2026-11-01 T-068] T-061 scent toggle and T-063 steer-to-camera).
	Map(WingBuffetAction, EKeys::Gamepad_DPad_Left);
	Map(TailSlamAction, EKeys::Gamepad_DPad_Right);
	// [2026-11-03 T-065] Camera cycle: V on keyboard, R3 (right-stick press)
	// on gamepad (adopted set: head chase / wide scenic / first-person).
	Map(CameraCycleAction, EKeys::V);
	Map(CameraCycleAction, EKeys::Gamepad_RightThumbstick);
	// [2026-11-03 T-072] Flow pause: Esc on keyboard, menu (Start) on
	// gamepad. Routed through the game mode's machine (never direct).
	PauseAction = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean);
	Map(PauseAction, EKeys::Escape);
	Map(PauseAction, EKeys::Gamepad_Special_Right);
	// [2026-11-03 T-073] Help overlay: H on keyboard, Back (Select) on
	// gamepad. Non-modal by design (overlay never pauses the game).
	HelpAction = MakeAction(TEXT("IA_Help"), EInputActionValueType::Boolean);
	Map(HelpAction, EKeys::H);
	Map(HelpAction, EKeys::Gamepad_Special_Left);
}

void ADragonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// [2026-10-28 T-033] First: guarantee the actions exist, THEN
	// bind them (the original order bound nulls — see header).
	EnsureInputActions();
	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	// [2026-10-28 T-033] Bind defensively: a null action stores a
	// binding that can never fire, so skip nulls and surface the
	// core Move/Look failure instead of swallowing it silently.
	if (MoveAction == nullptr || LookAction == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("Dragon input binding skipped: Move/Look action is null (Move=%s, Look=%s)"),
			   MoveAction != nullptr ? TEXT("set") : TEXT("null"),
			   LookAction != nullptr ? TEXT("set") : TEXT("null"));
	}
	if (MoveAction != nullptr)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleMove);
	}
	if (PadMoveAction != nullptr)
	{
		// [2026-11-03 T-065] Left stick: same movement + surge-gate edges as
		// WASD, but tagged as gamepad so stick-back pitches instead of
		// flaring (adopted maps diverge by device).
		EIC->BindAction(PadMoveAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandlePadMove);
	}
	if (LookAction != nullptr)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleLook);
	}
	if (PadLookAction != nullptr)
	{
		// [2026-11-03 T-065] Right stick: the camera stick, with its own
		// pitch-invert choice (separate from the mouse).
		EIC->BindAction(PadLookAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandlePadLook);
	}
	if (CameraCycleAction != nullptr)
	{
		EIC->BindAction(CameraCycleAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleCameraCycle);
	}
	if (PauseAction != nullptr)
	{
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &ADragonCharacter::HandlePause);
	}
	if (HelpAction != nullptr)
	{
		EIC->BindAction(HelpAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleHelp);
	}
	if (FlapAction != nullptr)
	{
		EIC->BindAction(FlapAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleFlap);
		// [2026-11-03 T-075] Hold edges feed prototype rise; shipped ignores holds.
		EIC->BindAction(FlapAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleRiseHeld);
		EIC->BindAction(FlapAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleRiseEnd);
		EIC->BindAction(FlapAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleRiseEnd);
	}
	if (MoveAction != nullptr)
	{
		// [2026-11-03 T-075] Press edge feeds W-tap Surge; movement itself
		// stays on the Triggered HandleMove binding above.
		EIC->BindAction(MoveAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleMoveStarted);
		// [2026-11-03 T-062] Release clears the remembered axis (S-flare /
		// A/D-turn in HandleMove): a released key stops flaring/turning.
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleMoveEnd);
		EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleMoveEnd);
	}
	if (PadMoveAction != nullptr)
	{
		// [2026-11-03 T-065] Stick flick shares the Surge gate (forward push
		// past half deflection in the momentum band); release clears the
		// remembered axis the same way.
		EIC->BindAction(PadMoveAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleMoveStarted);
		EIC->BindAction(PadMoveAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleMoveEnd);
		EIC->BindAction(PadMoveAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleMoveEnd);
	}
	if (FreeLookAction != nullptr)
	{
		EIC->BindAction(FreeLookAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleFreeLook);
		EIC->BindAction(FreeLookAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleFreeLookEnd);
		EIC->BindAction(FreeLookAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleFreeLookEnd);
	}
	if (SinkAction != nullptr)
	{
		EIC->BindAction(SinkAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleSink);
		EIC->BindAction(SinkAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleSinkEnd);
		EIC->BindAction(SinkAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleSinkEnd);
	}
	if (SteerLatchAction != nullptr)
	{
		EIC->BindAction(SteerLatchAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleSteerLatch);
	}
	if (ToggleFlightAction != nullptr)
	{
		EIC->BindAction(ToggleFlightAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleToggleFlight);
	}
	if (DiveAction != nullptr)
	{
		EIC->BindAction(DiveAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleDive);
		// [2026-10-28 T-030] Release paths: without these, bDiving stayed true
		// after Shift/trigger release and the dragon kept accelerating.
		EIC->BindAction(DiveAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleDiveEnd);
		EIC->BindAction(DiveAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleDiveEnd);
	}
	if (TrueSightAction != nullptr)
	{
		EIC->BindAction(TrueSightAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleTrueSight);
	}
	if (ScentToggleAction != nullptr)
	{
		EIC->BindAction(ScentToggleAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleToggleScent);
	}
	if (TintAction != nullptr)
	{
		EIC->BindAction(TintAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleToggleTintPicker);
	}
	// [2026-10-28 T-015] Fire mirrors Dive's binding shape: Triggered
	// while held, Completed/Canceled on release so the breath can
	// never latch on after button-up (T-030).
	if (FireAction != nullptr)
	{
		EIC->BindAction(FireAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleFire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &ADragonCharacter::HandleFireEnd);
		EIC->BindAction(FireAction, ETriggerEvent::Canceled, this, &ADragonCharacter::HandleFireEnd);
	}
	if (RoarAction != nullptr)
	{
		EIC->BindAction(RoarAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleRoar);
	}
	if (EatAction != nullptr)
	{
		EIC->BindAction(EatAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleEat);
		EIC->BindAction(WingBuffetAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleWingBuffet);
		EIC->BindAction(TailSlamAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleTailSlam);
	}
}

void ADragonCharacter::HandleMove(const FInputActionValue& Value)
{
	// [2026-11-03 T-065] Keyboard path: WASD, tagged keyboard so S flares.
	HandleMoveAxis(Value.Get<FVector2D>(), false);
}

void ADragonCharacter::HandlePadMove(const FInputActionValue& Value)
{
	// [2026-11-03 T-065] Gamepad path: left stick, tagged gamepad so
	// stick-back pitches instead of flaring (adopted maps diverge).
	HandleMoveAxis(Value.Get<FVector2D>(), true);
}

void ADragonCharacter::HandleMoveAxis(const FVector2D& Axis, bool bFromGamepad)
{
	// [2026-11-03 T-062] Every move sample is remembered for the flight model:
	// S-held (Y < -0.5) flares/bleeds speed in the momentum band and A/D (X)
	// yaws the nose there (bank/turn), while the precision band keeps every
	// axis as thrust via the integrator's pending input vector.
	// [2026-11-03 T-065] The device tag rides with the sample: only a
	// keyboard S flares; pad stick deflection pitches the nose in the
	// momentum band instead (see UpdatePrototypeFlight).
	LastMoveAxis = Axis;
	bLastMoveFromGamepad = bFromGamepad;
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}
	// [2026-11-03 T-075] Any movement input cancels latched steer-to-look
	// immediately (owner clarification, T-063).
	if (bSteerLatch)
	{
		bSteerLatch = false;
		UE_LOG(LogUnderTheMountain, Log, TEXT("Steer-to-look canceled by movement input"));
	}
	const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
}

void ADragonCharacter::HandleLook(const FInputActionValue& Value)
{
	// [2026-11-03 T-065] Mouse path: pitch follows the mouse invert choice.
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y * (InvertMousePitch() ? -1.0f : 1.0f));
}

void ADragonCharacter::HandlePadLook(const FInputActionValue& Value)
{
	// [2026-11-03 T-065] Right stick: the camera stick, with its own pitch-
	// invert choice (separate from the mouse).
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y * (InvertPadCameraPitch() ? -1.0f : 1.0f));
}

void ADragonCharacter::HandleFlap()
{
	// [2026-11-03 T-075] Course program owns its inputs; pilot flap would
	// spend the legs' stamina out from under the measurements.
	if (bCourseFlying)
	{
		return;
	}
	if (!Stats->CanFlap())
	{
		return;
	}
	// [2026-11-03 T-073] First flap dismisses the flap prompt (attempt
	// with stamina counts as performed — the teaching is the key).
	MarkOnboardingDone(EOnboardingVerb::Flap);
	if (!IsFlying() && !IsPrototypeFlying())
	{
		// [2026-11-03 T-075] Prototype schemes take off into the custom
		// chassis; shipped flight is untouched when the scheme is 0.
		if (UDragonMovementComponent::GetFlightScheme() != 0)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Custom,
													UDragonMovementComponent::DragonFlightCustomMode);
			ProtoNose = GetControlRotation();
		}
		else
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		}
	}
	// [2026-11-03 T-075] Prototype takeoff/ascent: one full 20-point reserve
	// buys the 900 cm/s up-forward launch (spike economy, provisional for
	// the course); shipped keeps the 3-cost hop below.
	if (IsPrototypeFlying() || UDragonMovementComponent::GetFlightScheme() != 0)
	{
		if (Stats->Stamina < 20.0f)
		{
			UE_LOG(LogUnderTheMountain, Log,
				   TEXT("Ascent refused: needs one 20-point reserve (stamina %.1f)"), Stats->Stamina);
			return;
		}
		if (Stats->TryConsumeStamina(20.0f))
		{
			if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement()))
			{
				Proto->ApplyAscent();
			}
		}
		return;
	}
	if (Stats->TryConsumeStamina(Stats->FlapStaminaCost))
	{
		FVector Vel = GetVelocity();
		Vel.Z = FlapUpwardSpeed;
		LaunchCharacter(Vel, false, true);
	}
}

void ADragonCharacter::HandleToggleFlight()
{
	// [2026-11-03 T-075] Course program owns its inputs (assist is scripted).
	if (bCourseFlying)
	{
		return;
	}
	// [2026-11-03 T-075] F is Landing Assist while prototype-flying (adopted
	// map): forced precision + brake + descent cap; walk/fall clears it.
	if (IsPrototypeFlying())
	{
		if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement()))
		{
			Proto->bLandingAssist = !Proto->bLandingAssist;
			UE_LOG(LogUnderTheMountain, Log, TEXT("Landing Assist %s"),
				   Proto->bLandingAssist ? TEXT("ON") : TEXT("OFF"));
		}
		return;
	}
	const bool bWasFlying = IsFlying() || IsPrototypeFlying();
	if (!bWasFlying && UDragonMovementComponent::GetFlightScheme() != 0)
	{
		// [2026-11-03 T-062] Adopted map: grounded F is Tail Slam (takeoff is
		// Space/Ascent). Legacy scheme 0 keeps the magic walk/fly toggle.
		HandleTailSlam();
		return;
	}
	// [2026-11-03 T-075] Legacy (scheme 0) keeps the magic walk/fly toggle.
	GetCharacterMovement()->SetMovementMode(bWasFlying ? MOVE_Walking : MOVE_Flying);
	// [2026-11-01 T-068] Leaving the ground plays the takeoff leap once;
	// [2026-11-01 T-068] locomotion (Fly/Hover by speed) takes over after.
	if (!bWasFlying)
	{
		PlayOneShot(TakeoffAnim);
	}
}

void ADragonCharacter::HandleDive(const FInputActionValue& Value)
{
	bDiving = Value.Get<bool>();
}

void ADragonCharacter::HandleDiveEnd()
{
	// [2026-10-28 T-030] See SetupPlayerInputComponent: release clears dive.
	bDiving = false;
}

void ADragonCharacter::HandleTrueSight()
{
	// [2026-10-28 T-013] Forward-only: the component owns the Seeing
	// Stone rules (refusal while stolen) and logs refusals itself.
	const bool bActive = Scent->ToggleTrueSight();
	UE_LOG(LogUnderTheMountain, Log, TEXT("True-sight toggled: %s"),
		   bActive ? TEXT("ACTIVE (scent dimmed, treasure markers on)") : TEXT("off"));
}

void ADragonCharacter::HandleToggleScent()
{
	// [2026-11-03 T-061] Forward-only like true-sight: the component
	// owns the ribbon hide/show (never destroy/rebuild) and markers
	// stay on the T layer regardless of this state.
	const bool bVisible = Scent->ToggleScentVisible();
	UE_LOG(LogUnderTheMountain, Log, TEXT("Scent ribbons toggled: %s"),
		   bVisible ? TEXT("shown") : TEXT("HIDDEN (true-sight markers unaffected)"));
	// [2026-11-03 T-073] First scent toggle dismisses the scent prompt.
	MarkOnboardingDone(EOnboardingVerb::Scent);
}

void ADragonCharacter::GatherCombatTargets(TArray<UThiefBrain*>& OutBrains, TArray<FVector>& OutLocations)
{
	// [2026-10-28 T-015] Parallel arrays for the combat seam: brains[i]
	// stands at locations[i]. A thief whose brain does not exist yet
	// (spawned, BeginPlay pending) is skipped, not dereferenced.
	OutBrains.Reset();
	OutLocations.Reset();
	if (GetWorld() == nullptr)
	{
		return;
	}
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AThiefCharacter::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		AThiefCharacter* Thief = Cast<AThiefCharacter>(Actor);
		if (Thief == nullptr)
		{
			continue;
		}
		UThiefBrain* Brain = Thief->GetBrain();
		if (Brain == nullptr)
		{
			continue;
		}
		OutBrains.Add(Brain);
		OutLocations.Add(Thief->GetActorLocation());
	}
}

void ADragonCharacter::GatherGoatTargets(TArray<AGoatActor*>& OutGoats,
										 TArray<FVector>& OutLocations)
{
	// [2026-11-01 T-078] Parallel arrays for TickFire's goat seam,
	// mirroring GatherCombatTargets: gathered only while breathing.
	OutGoats.Reset();
	OutLocations.Reset();
	if (GetWorld() == nullptr)
	{
		return;
	}
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGoatActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		AGoatActor* Goat = Cast<AGoatActor>(Actor);
		if (Goat == nullptr)
		{
			continue;
		}
		OutGoats.Add(Goat);
		OutLocations.Add(Goat->GetActorLocation());
	}
}

void ADragonCharacter::GatherBurnableTargets(TArray<ABurnablePropActor*>& OutProps,
											 TArray<FVector>& OutLocations)
{
	// [2026-11-01 T-055] Parallel arrays for TickFire's prop seam.
	OutProps.Reset();
	OutLocations.Reset();
	if (GetWorld() == nullptr)
	{
		return;
	}
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABurnablePropActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		ABurnablePropActor* Prop = Cast<ABurnablePropActor>(Actor);
		if (Prop == nullptr)
		{
			continue;
		}
		OutProps.Add(Prop);
		OutLocations.Add(Prop->GetActorLocation());
	}
}

void ADragonCharacter::ApplyBurningAreaDamage(float DeltaSeconds)
{
	// [2026-11-01 T-055] A burning prop is a small fire: anything
	// alive inside its BurnAreaRadius takes the breath's DPS (the
	// same seams fire uses, corpses skipped by those seams).
	// Targets are gathered here, not only while firing - a prop
	// keeps burning after the breath stops.
	if (Combat == nullptr || GetWorld() == nullptr || DeltaSeconds <= 0.0f)
	{
		return;
	}
	TArray<UThiefBrain*> Brains;
	TArray<FVector> BrainLocations;
	GatherCombatTargets(Brains, BrainLocations);
	TArray<AGoatActor*> Goats;
	TArray<FVector> GoatLocations;
	GatherGoatTargets(Goats, GoatLocations);
	TArray<ABurnablePropActor*> Props;
	TArray<FVector> PropLocations;
	GatherBurnableTargets(Props, PropLocations);
	for (int32 PropIndex = 0; PropIndex < Props.Num(); ++PropIndex)
	{
		ABurnablePropActor* Prop = Props[PropIndex];
		if (Prop == nullptr || Prop->GetBurnState() != EBurnState::Burning)
		{
			continue;
		}
		const FVector Centre = PropLocations[PropIndex];
		const float Radius = Prop->BurnAreaRadius;
		for (int32 Index = 0; Index < Brains.Num() && Index < BrainLocations.Num(); ++Index)
		{
			if (Brains[Index] != nullptr && Brains[Index]->State() != EThiefState::Dead &&
				FVector::Dist(BrainLocations[Index], Centre) <= Radius)
			{
				Brains[Index]->ApplyDamage(Combat->FireDps * DeltaSeconds, BrainLocations[Index]);
			}
		}
		for (int32 Index = 0; Index < Goats.Num() && Index < GoatLocations.Num(); ++Index)
		{
			if (Goats[Index] != nullptr && !Goats[Index]->IsDead() &&
				FVector::Dist(GoatLocations[Index], Centre) <= Radius)
			{
				Goats[Index]->ApplyGoatDamage(Combat->FireDps * DeltaSeconds);
			}
		}
	}
}

void ADragonCharacter::MaybeSpawnGroundScorch(const FVector& Origin, const FVector& Forward)
{
	// [2026-11-01 T-055] Ground scorch: while the breath points at
	// the ground (aim below horizontal), a 0.25 s-throttled trace
	// accumulates burn time per spot; 1.0 s on one spot leaves a
	// disc. Dedup 200 cm, cap 64 per session - the marks are
	// persistent, so unbounded spawning is unbounded actors.
	GroundScorchTimer -= GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	if (Forward.Z > -0.25f || GroundScorchTimer > 0.0f || GetWorld() == nullptr)
	{
		return;
	}
	GroundScorchTimer = 0.25f;
	FHitResult Hit;
	const FVector End = Origin + Forward * Combat->FireRange;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility))
	{
		return;
	}
	for (const FVector& Existing : ScorchLocations)
	{
		if (FVector::Dist2D(Existing, Hit.Location) < 200.0f)
		{
			GroundScorchAccum += 0.25f;
			if (GroundScorchAccum >= 1.0f && ScorchLocations.Num() < 64)
			{
				// Already marked nearby: nothing new to spawn.
				GroundScorchAccum = 0.0f;
			}
			return;
		}
	}
	GroundScorchAccum += 0.25f;
	if (GroundScorchAccum < 1.0f)
	{
		return;
	}
	GroundScorchAccum = 0.0f;
	if (ScorchLocations.Num() >= 64)
	{
		return;
	}
	const FVector Spot = Hit.Location + FVector(0.0f, 0.0f, 4.0f);
	if (AScorchMarkActor* Scorch = GetWorld()->SpawnActor<AScorchMarkActor>(Spot, FRotator::ZeroRotator))
	{
		Scorch->SetDiameter(500.0f);
		ScorchLocations.Add(Spot);
		UE_LOG(LogUnderTheMountain, Log, TEXT("Scorch mark left at %s"), *Spot.ToString());
	}
}

void ADragonCharacter::HandleIntroChoice(int32 BranchIndex)
{
	if (UWorld* World = GetWorld())
	{
		if (UIntroSubsystem* Intro = World->GetSubsystem<UIntroSubsystem>())
		{
			Intro->Choose(BranchIndex == 1 ? EIntroBranch::Chase : EIntroBranch::LetGo);
		}
	}
}

void ADragonCharacter::TickIntro(float DeltaSeconds)
{
	// [2026-11-01 T-018] Intro glue: state machine in the
	// subsystem, world work here (camera, gnome spawn/watch,
	// widget show/dismiss). Every transition is one-shot latched
	// so per-tick calls cannot double-spawn or double-show.
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	UIntroSubsystem* Intro = World->GetSubsystem<UIntroSubsystem>();
	if (Intro == nullptr)
	{
		return;
	}
	if (CVarIntroEnabled.GetValueOnGameThread() == 0)
	{
		if (Intro->GetState() != EIntroState::FreePlay)
		{
			Intro->SkipToFreePlay();
		}
		return;
	}
	Intro->TickIntro(DeltaSeconds);
	switch (Intro->GetState())
	{
	case EIntroState::WakeUp:
		// [2026-11-01 T-018] The restrained camera move: boom
		// eases from a close 240 to the play 700 over the wake.
		// (T-068's Sleep/Wake clips take over the body when they
		// land; the camera move is theirs to keep.)
		if (CameraBoom != nullptr)
		{
			const float P = Intro->GetWakeProgress();
			CameraBoom->TargetArmLength = FMath::Lerp(240.0f, 700.0f, P * P * (3.0f - 2.0f * P));
		}
		break;
	case EIntroState::GnomeSteals:
		if (!bIntroGnomeSpawned)
		{
			bIntroGnomeSpawned = true;
			UThiefDataAsset* BurglarData = LoadObject<UThiefDataAsset>(
				nullptr, TEXT("/Game/Gameplay/Thieves/DA_Thief_Burglar.DA_Thief_Burglar"));
			// [2026-11-03 T-066] Spawned beside the hoard prize chest (12500, 500):
			// the brain makes for the richest treasure in reach, so the steal
			// lands in seconds, not after a cross-lair trek (first version
			// spawned at the chamber edge and the gnome walked to C3).
			const FVector SpawnPoint(12200.0f, 100.0f, 300.0f);
			AThiefCharacter* Gnome = World->SpawnActorDeferred<AThiefCharacter>(
				AThiefCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, SpawnPoint));
			if (Gnome != nullptr)
			{
				Gnome->ThiefData = BurglarData;
				Gnome->ExitLocation = FVector(18700.0f, 0.0f, 160.0f);
				Gnome->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnPoint));
				IntroGnome = Gnome;
				Intro->SetGnomeName(Gnome->GetFName());
				UE_LOG(LogUnderTheMountain, Log, TEXT("Intro: gnome spawned in the hoard"));
			}
		}
		if (AThiefCharacter* Gnome = IntroGnome.Get())
		{
			if (UThiefBrain* Brain = Gnome->GetBrain())
			{
				if (Brain->CarriedItems().Num() > 0 || Brain->State() == EThiefState::Flee)
				{
					Intro->ReportGnomeStole();
				}
			}
		}
		break;
	case EIntroState::Choice:
		// [2026-11-01 T-019] Gamepad parity for the choice: A
		// (bottom face) = Chase, B (right face) = Let go, polled
		// here so a pad-only player is never stuck at a
		// mouse-only prompt. Mouse clicks remain the other path.
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
			{
				HandleIntroChoice(1);
				break;
			}
			if (PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
			{
				HandleIntroChoice(0);
				break;
			}
		}
		if (!bIntroChoiceShown)
		{
			bIntroChoiceShown = true;
			// Freeze the gnome mid-getaway while the player
			// decides (its tick, not the world's).
			if (AThiefCharacter* Gnome = IntroGnome.Get())
			{
				Gnome->SetActorTickEnabled(false);
			}
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				if (PC->IsLocalController())
				{
					IntroWidget = CreateWidget<UIntroChoiceWidget>(PC, UIntroChoiceWidget::StaticClass());
					if (IntroWidget != nullptr)
					{
						IntroWidget->BindToPawn(this);
						IntroWidget->AddToViewport(20);
						PC->SetShowMouseCursor(true);
					}
				}
			}
		}
		break;
	case EIntroState::FreePlay:
		if (bIntroChoiceShown)
		{
			bIntroChoiceShown = false;
			if (AThiefCharacter* Gnome = IntroGnome.Get())
			{
				Gnome->SetActorTickEnabled(true);
			}
			if (IntroWidget != nullptr)
			{
				IntroWidget->RemoveFromParent();
				IntroWidget = nullptr;
			}
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->SetShowMouseCursor(false);
			}
		}
		break;
	default:
		break;
	}
}

void ADragonCharacter::AuditGameLoop(float DeltaSeconds)
{
	// [2026-11-01 T-016] The pawn is the only actor guaranteed to
	// tick from second zero, so it drives the loop: session start,
	// event accounting, waves, outcome. Subsystems own the rules;
	// this function only observes and forwards, at 0.5 s cadence
	// (events are second-scale; per-frame audits would waste the
	// actor scans).
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	UNotorietySubsystem* Notoriety = World->GetSubsystem<UNotorietySubsystem>();
	UHoardSubsystem* Hoard = World->GetSubsystem<UHoardSubsystem>();
	UWaveSubsystem* Waves = World->GetSubsystem<UWaveSubsystem>();
	UTreasureSubsystem* TreasureSub = World->GetSubsystem<UTreasureSubsystem>();
	UXPSubsystem* XP = World->GetSubsystem<UXPSubsystem>();
	if (Notoriety == nullptr || Hoard == nullptr || Waves == nullptr || TreasureSub == nullptr ||
		TreasureSub->Registry() == nullptr || XP == nullptr)
	{
		return;
	}
	Hoard->RecordSessionStart(TreasureSub->Registry()->TotalGold());
	Waves->TickWave(DeltaSeconds);

	// [2026-11-03 T-083] Wave-gold XP window: when the wave number
	// advances, the finished wave's net hoard delta is awarded (a
	// straggler escaping in the gap lands in the next window —
	// attribution fuzz, documented not hidden); the final wave
	// awards once at session over instead of on an advance that
	// never comes.
	const int64 CurrentHoardNow = Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold());
	if (Waves->IsSessionOver())
	{
		if (!bXPFinalWaveAwarded)
		{
			bXPFinalWaveAwarded = true;
			XP->ClaimWaveGold(Waves->GetCurrentWave(), CurrentHoardNow - XPWaveHoardStart);
		}
	}
	else if (Waves->GetCurrentWave() != XPWaveTracked)
	{
		// Wave 0 is the pre-run intermission, not a scored wave: its
		// "window" opens on the starting hoard, so there is nothing to
		// award — just take the baseline for wave 1 and move on.
		if (XPWaveTracked != 0)
		{
			XP->ClaimWaveGold(XPWaveTracked, CurrentHoardNow - XPWaveHoardStart);
		}
		XPWaveTracked = Waves->GetCurrentWave();
		XPWaveHoardStart = CurrentHoardNow;
	}

	// [2026-11-03 T-086] Hoard-mound visual: find by name-or-label
	// prefix (box() suffixes placed labels prefix_NNN, so exact match
	// is wrong; GetActorNameOrLabel also survives cooking, where
	// editor labels are stripped but object names persist). Base scale
	// is captured on first sight (box() bakes size into actor scale);
	// applies + logs only on step change.
	if (!IsValid(HoardMoundActor.Get()))
	{
		TArray<AActor*> MeshActors;
		UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), MeshActors);
		for (AActor* Actor : MeshActors)
		{
			if (Actor != nullptr && Actor->GetActorNameOrLabel().StartsWith(TEXT("HoardMound")))
			{
				HoardMoundActor = Cast<AStaticMeshActor>(Actor);
				if (HoardMoundActor != nullptr)
				{
					HoardMoundBaseScale = HoardMoundActor->GetActorScale3D();
				}
				break;
			}
		}
	}
	if (HoardMoundActor != nullptr)
	{
		const UHoardThresholdsAsset* HT = Hoard->Thresholds();
		const float WarnF = HT != nullptr ? HT->MoundWarningFraction : 0.75f;
		const float WinF = HT != nullptr ? HT->WinFraction : 0.5f;
		const float LoseF = HT != nullptr ? HT->LoseFraction : 0.2f;
		const int32 Step = UHoardSubsystem::HoardMoundStep(CurrentHoardNow, Hoard->GetStartingHoard(),
														   WarnF, WinF, LoseF);
		if (Step != HoardMoundStep)
		{
			HoardMoundStep = Step;
			const float StepScale = UHoardSubsystem::HoardMoundScaleForStep(Step);
			HoardMoundActor->SetActorScale3D(HoardMoundBaseScale * StepScale);
			UE_LOG(LogUnderTheMountain, Log,
				   TEXT("Hoard mound step %d/3 (%lld of %lld gold, scale %.1f)"), Step,
				   CurrentHoardNow, Hoard->GetStartingHoard(), StepScale);
		}
	}

	LoopAuditTimer += DeltaSeconds;
	if (LoopAuditTimer < 0.5f)
	{
		return;
	}
	LoopAuditTimer = 0.0f;

	TArray<AActor*> ThiefActors;
	UGameplayStatics::GetAllActorsOfClass(World, AThiefCharacter::StaticClass(), ThiefActors);
	for (AActor* Actor : ThiefActors)
	{
		AThiefCharacter* Thief = Cast<AThiefCharacter>(Actor);
		UThiefBrain* Brain = Thief ? Thief->GetBrain() : nullptr;
		if (Brain == nullptr)
		{
			continue;
		}
		const FName Key = FName(*FString::Printf(TEXT("%s|%d"), *Thief->GetName(),
												 static_cast<int32>(Brain->State())));
		if (Brain->ReachedExit() && !CountedThiefEvents.Contains(Thief->GetFName()))
		{
			CountedThiefEvents.Add(Thief->GetFName());
			const bool bWithLoot = Brain->CarriedItems().Num() > 0;
			// [2026-11-01 T-018] The intro gnome's escape is
			// charged by the intro table, not the generic rates:
			// suppressed here, replaced when the subsystem says
			// so (failed chase +5; let-go already charged).
			bool bSuppressed = false;
			if (UIntroSubsystem* Intro = World->GetSubsystem<UIntroSubsystem>())
			{
				int32 Replacement = 0;
				if (Intro->ShouldSuppressEscapeGain(Thief->GetFName(), Replacement))
				{
					bSuppressed = true;
					if (Replacement > 0)
					{
						Notoriety->AddPoints(Replacement, TEXT("intro: gnome got away"));
					}
				}
			}
			if (!bSuppressed)
			{
				Notoriety->AddEscapePoints(bWithLoot);
				// [2026-11-03 T-083] Lootless escapes earn XP (GAMEPLAY.md
				// section 8): the scare-off reward, claimed by actor name
				// so it lands once. Intro-suppressed escapes stay in the
				// intro economy (no XP on top of it).
				if (!bWithLoot)
				{
					XP->ClaimScare(Thief->GetFName(), true);
				}
			}
			int64 Stolen = 0;
			for (const FName& ItemId : Brain->CarriedItems())
			{
				if (const FTreasureEntry* Entry = TreasureSub->Registry()->Find(ItemId))
				{
					Stolen += Entry->GoldValue;
				}
			}
			if (Stolen > 0)
			{
				Hoard->RecordTheft(Stolen, *Thief->GetName());
			}
		}
		if (Brain->State() == EThiefState::Dead && !CountedThiefEvents.Contains(Key))
		{
			CountedThiefEvents.Add(Key);
			Notoriety->AddPublicKillPoints(Thief->GetActorLocation());
			// [2026-11-03 T-083] Inside kills earn XP (GAMEPLAY.md
			// section 8): claimed by stable actor name, so a later
			// eat reprocessing the same corpse claims nothing.
			XP->ClaimKill(Thief->GetFName(),
						  UNotorietySubsystem::IsInsideLair(Thief->GetActorLocation()));
		}
	}

	TArray<ABurnablePropActor*> Props;
	TArray<FVector> PropLocations;
	GatherBurnableTargets(Props, PropLocations);
	for (ABurnablePropActor* Prop : Props)
	{
		if (Prop != nullptr && Prop->GetBurnState() != EBurnState::Intact &&
			!CountedPropBurns.Contains(Prop->GetFName()))
		{
			CountedPropBurns.Add(Prop->GetFName());
			Notoriety->AddStructureHitPoints();
		}
	}

	// [2026-11-01 T-016] Outcome: dragon down loses immediately;
	// final-wave survival scores the hoard. Logged once; the
	// session-end screen is T-072's, this is the state it reads.
	if (!bOutcomeLogged)
	{
		const bool bDragonDown = Stats != nullptr && Stats->HP <= 0.0f;
		// [2026-11-01 T-016] Fractions come from DA_HoardThresholds
		// when the asset exists (its class defaults otherwise) -
		// never literals, per the section 8 AT note.
		const UHoardThresholdsAsset* Thresholds = Hoard->Thresholds();
		const float WinFraction = Thresholds ? Thresholds->WinFraction : 0.5f;
		const float LoseFraction = Thresholds ? Thresholds->LoseFraction : 0.2f;
		const EHoardOutcome Outcome = UHoardSubsystem::ClassifyOutcome(
			Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold()), Hoard->GetStartingHoard(),
			WinFraction, LoseFraction, Waves->IsSessionOver(), bDragonDown);
		if (Outcome != EHoardOutcome::InProgress)
		{
			bOutcomeLogged = true;
			UE_LOG(LogUnderTheMountain, Log,
				   TEXT("Session outcome: %d (hoard %lld of %lld, stars %d, wave %d)"),
				   static_cast<int32>(Outcome), Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold()),
				   Hoard->GetStartingHoard(), Notoriety->GetStars(), Waves->GetCurrentWave());
			// [2026-11-03 T-072] Feed the flow shell: the end screen
			// shows this outcome (PyrrhicWin still counts as won —
			// the hoard numbers on the screen tell the cost).
			if (AUnderTheMountainGameMode* Mode =
					World->GetAuthGameMode<AUnderTheMountainGameMode>())
			{
				const EGameFlowOutcome FlowOutcome =
					(Outcome == EHoardOutcome::Win || Outcome == EHoardOutcome::PyrrhicWin)
						? EGameFlowOutcome::Won
						: EGameFlowOutcome::Lost;
				Mode->RequestSessionEnd(FlowOutcome,
										Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold()),
										Hoard->GetStartingHoard(), Notoriety->GetStars(),
										Waves->GetCurrentWave());
			}
		}
	}
}

void ADragonCharacter::HandleFire(const FInputActionValue& Value)
{
	// [2026-10-28 T-015] Latch only — Tick owns starting/ticking the
	// breath so stamina gating and target gathering happen once.
	bWantsFire = Value.Get<bool>();
	if (bWantsFire)
	{
		// [2026-11-03 T-073] First trigger pull dismisses the fire
		// prompt (attempt counts — the teaching is the key).
		MarkOnboardingDone(EOnboardingVerb::Fire);
	}
}

void ADragonCharacter::HandleFireEnd()
{
	// [2026-10-28 T-015] Release path (Completed/Canceled): clear the
	// latch and stop the breath now, not next tick.
	bWantsFire = false;
	if (Combat != nullptr)
	{
		Combat->StopFire();
	}
}

void ADragonCharacter::PlayOneShot(UAnimSequence* Clip)
{
	// [2026-11-01 T-068] Null clip = no-op (un-imported checkout): the verb
	// [2026-11-01 T-068] still happened, only its presentation is absent.
	if (Clip == nullptr)
	{
		return;
	}
	if (USkeletalMeshComponent* Mesh = GetMesh())
	{
		Mesh->PlayAnimation(Clip, false);
		Mesh->SetPlayRate(1.0f);
		OneShotTimeRemaining = Clip->GetPlayLength();
	}
}

void ADragonCharacter::HandleWingBuffet()
{
	if (Combat == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Wing buffet ignored: no combat component"));
		return;
	}
	TArray<UThiefBrain*> Brains;
	TArray<FVector> Locations;
	GatherCombatTargets(Brains, Locations);
	const float Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	int32 NumDazed = 0;
	const bool bBuffeted = Combat->TryWingBuffet(Now, Stats, GetActorLocation(), GetActorForwardVector(),
												 Brains, Locations, NumDazed);
	if (bBuffeted)
	{
		// [2026-11-01 T-068] Grounded buffet mantles like the roar clip;
		// [2026-11-01 T-068] airborne it must not interrupt flight locomotion.
		if (!IsFlying())
		{
			PlayOneShot(RoarAnim);
		}
		UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon wing buffet: dazed %d"), NumDazed);
	}
}

void ADragonCharacter::HandleTailSlam()
{
	if (Combat == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Tail slam ignored: no combat component"));
		return;
	}
	TArray<UThiefBrain*> Brains;
	TArray<FVector> Locations;
	GatherCombatTargets(Brains, Locations);
	const float Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	int32 NumHit = 0;
	const bool bSlammed = Combat->TryTailSlam(Now, Stats, GetActorLocation(), GetActorForwardVector(),
											  Brains, Locations, NumHit);
	if (bSlammed)
	{
		UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon tail slam: hit %d"), NumHit);
	}
}

void ADragonCharacter::HandleRoar()
{
	// [2026-10-28 T-015] The pawn supplies world facts (time, origin,
	// targets); TryRoar owns cost/cooldown and logs any refusal.
	if (Combat == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Roar ignored: no combat component"));
		return;
	}
	TArray<UThiefBrain*> Brains;
	TArray<FVector> Locations;
	GatherCombatTargets(Brains, Locations);
	const float Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bRoared = Combat->TryRoar(Now, Stats, GetActorLocation(), Brains, Locations);
	if (bRoared)
	{
		// [2026-11-03 T-073] First roar that actually fires dismisses
		// the roar prompt (a cooldown-blocked press keeps teaching).
		MarkOnboardingDone(EOnboardingVerb::Roar);
	}
	// [2026-11-01 T-068] A roar that actually fired is now visible, not
	// [2026-11-01 T-068] just audible-in-the-log: mantle + head-throw clip.
	if (bRoared)
	{
		PlayOneShot(RoarAnim);
	}
	// [2026-11-01 T-016] A roar that scares thieves while the
	// dragon is OUTSIDE the lair is a witnessed act (+4, once per
	// roar); inside, nobody outside hears about it.
	if (bRoared && GetWorld() != nullptr &&
		!UNotorietySubsystem::IsInsideLair(GetActorLocation()))
	{
		if (UNotorietySubsystem* Notoriety = GetWorld()->GetSubsystem<UNotorietySubsystem>())
		{
			Notoriety->AddRoarOutsidePoints();
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon roar: %s (%d thieves in world)"),
		   bRoared ? TEXT("roared") : TEXT("blocked — see combat warning above"), Brains.Num());
}

void ADragonCharacter::HandleEat()
{
	// [2026-10-28 T-015] The pawn picks the meal (it owns positions):
	// the single nearest eatable within Combat->EatRange — a dead
	// thief's corpse or an uneaten goat, whichever is closer. The
	// component then owns the channel and the food values.
	if (Combat == nullptr || GetWorld() == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("Eat ignored: no combat component or world"));
		return;
	}
	const FVector Origin = GetActorLocation();
	const float RangeSq = Combat->EatRange * Combat->EatRange;
	float BestDistSq = RangeSq;
	UThiefBrain* BestBrain = nullptr;
	AActor* BestThiefActor = nullptr;
	AGoatActor* BestGoat = nullptr;

	TArray<AActor*> ThiefActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AThiefCharacter::StaticClass(), ThiefActors);
	for (AActor* Actor : ThiefActors)
	{
		AThiefCharacter* Thief = Cast<AThiefCharacter>(Actor);
		if (Thief == nullptr)
		{
			continue;
		}
		UThiefBrain* Brain = Thief->GetBrain();
		if (Brain == nullptr || Brain->State() != EThiefState::Dead)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Thief->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestBrain = Brain;
			BestThiefActor = Thief;
			BestGoat = nullptr;
		}
	}
	TArray<AActor*> GoatActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGoatActor::StaticClass(), GoatActors);
	for (AActor* Actor : GoatActors)
	{
		AGoatActor* Goat = Cast<AGoatActor>(Actor);
		if (Goat == nullptr || Goat->IsEaten())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Goat->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestBrain = nullptr;
			BestThiefActor = nullptr;
			BestGoat = Goat;
		}
	}

	bool bStarted = false;
	if (BestBrain != nullptr)
	{
		bStarted = Combat->StartEatThief(BestBrain);
		if (bStarted)
		{
			// [2026-10-28 T-015] Remember the corpse: UThiefBrain has
			// no "consumed" flag, so Tick hides this actor on AteThief.
			PendingEatActor = BestThiefActor;
		}
	}
	else if (BestGoat != nullptr)
	{
		bStarted = Combat->StartEatGoat(BestGoat);
	}
	if (bStarted)
	{
		UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon eating..."));
		// [2026-11-03 T-073] First real meal dismisses the eat prompt
		// (a whiff that starts nothing keeps teaching).
		MarkOnboardingDone(EOnboardingVerb::Eat);
	}
	else
	{
		// [2026-10-28 T-015] Surfaced, never silent: a failed Eat press
		// always says why the dragon went hungry.
		UE_LOG(LogUnderTheMountain, Log, TEXT("Eat: nothing edible in reach"));
	}
}

void ADragonCharacter::HandleToggleTintPicker()
{
	// [2026-10-28 T-027] Lazy-create the code-built picker for the
	// local player only; remote/PIE-server pawns never see UI.
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr || !PC->IsLocalController())
	{
		return;
	}
	if (TintWidget == nullptr)
	{
		TintWidget = CreateWidget<UDragonTintWidget>(PC, UDragonTintWidget::StaticClass());
		if (TintWidget == nullptr)
		{
			UE_LOG(LogUnderTheMountain, Warning, TEXT("Tint picker widget failed to create"));
			return;
		}
		TintWidget->BindToAppearance(Appearance);
	}
	bTintPickerOpen = !bTintPickerOpen;
	if (bTintPickerOpen)
	{
		TintWidget->InitializeFromCurrent();
		TintWidget->AddToViewport(10);
		PC->SetShowMouseCursor(true);
		// [2026-11-03 T-073] First picker open dismisses the tint prompt.
		MarkOnboardingDone(EOnboardingVerb::Tint);
	}
	else
	{
		TintWidget->RemoveFromParent();
		PC->SetShowMouseCursor(false);
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Scale tint picker: %s"),
		   bTintPickerOpen ? TEXT("open (C to close)") : TEXT("closed"));
}

void ADragonCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	// [2026-10-28 T-011] By the time Landed fires, CMC has zeroed the
	// velocity that hit the ground, so the impact speed is the abs of
	// the value Tick stored last frame (LastVelocityZ). The curve itself
	// (threshold/min/max) belongs to Stats so it is testable headless.
	const float ImpactSpeed = FMath::Abs(LastVelocityZ);
	const float Damage = Stats->LandingDamageForImpactSpeed(ImpactSpeed);
	if (Damage > 0.0f)
	{
		Stats->ApplyHPDamage(Damage);
		UE_LOG(LogUnderTheMountain, Log,
			   TEXT("Hard landing: impact %.1f cm/s, HP damage %.1f (HP now %.1f)"), ImpactSpeed, Damage,
			   Stats->HP);
	}
	// [2026-11-01 T-077] Whatever was under the dragon takes the
	// crush half of the landing. This runs even when the dragon
	// itself took no damage: the hard-landing threshold (800) guards
	// the dragon's HP, the crush threshold (300) guards victims —
	// conflating them would make most real landings harmless to the
	// thief standing exactly where the dragon came down.
	ApplyLandingCrush(GetActorLocation(), ImpactSpeed);
	// [2026-11-01 T-068] Touchdown reads: flare/settle clip on any landing,
	// [2026-11-01 T-068] the flinch instead when the landing drew blood.
	PlayOneShot(Damage > 0.0f && HitAnim != nullptr ? HitAnim : LandAnim);
}

void ADragonCharacter::ApplyLandingCrush(const FVector& Centre, float ImpactSpeed)
{
	if (Combat == nullptr || GetWorld() == nullptr)
	{
		return;
	}
	const float Damage = UDragonCombatComponent::CrushDamageForImpact(
		ImpactSpeed, Combat->CrushDamage, Combat->CrushMinImpactSpeed);
	if (Damage <= 0.0f)
	{
		return;
	}
	// [2026-11-01 T-077] Footprint is a ground-plane radius: a thief
	// on the ledge above the landing point is not crushed by a
	// landing below it, however close the 3D distance reads.
	TArray<AActor*> Thieves;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AThiefCharacter::StaticClass(), Thieves);
	for (AActor* Actor : Thieves)
	{
		if (FVector::Dist2D(Centre, Actor->GetActorLocation()) <= Combat->CrushRadius)
		{
			ApplyCrushVictim(Actor, ImpactSpeed, TEXT("landing"));
		}
	}
	TArray<AActor*> Goats;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGoatActor::StaticClass(), Goats);
	for (AActor* Actor : Goats)
	{
		if (FVector::Dist2D(Centre, Actor->GetActorLocation()) <= Combat->CrushRadius)
		{
			ApplyCrushVictim(Actor, ImpactSpeed, TEXT("landing"));
		}
	}
}

void ADragonCharacter::ApplyCrushVictim(AActor* Victim, float ImpactSpeed, const TCHAR* Source)
{
	if (Victim == nullptr || Combat == nullptr)
	{
		return;
	}
	const float Damage = UDragonCombatComponent::CrushDamageForImpact(
		ImpactSpeed, Combat->CrushDamage, Combat->CrushMinImpactSpeed);
	if (Damage <= 0.0f)
	{
		return;
	}
	// [2026-11-01 T-077] Damage routes through the same seams fire
	// uses (brain for thieves, ApplyGoatDamage for goats); both
	// ignore corpses, so a crush can never double-count a kill.
	// The log line names the crush, the source and the victim —
	// this is the PIE evidence line the task asks for.
	if (AThiefCharacter* Thief = Cast<AThiefCharacter>(Victim))
	{
		Thief->ApplyThiefDamage(Damage);
		UE_LOG(LogUnderTheMountain, Log, TEXT("Crush (%s): dragon crushed thief %s for %.1f at %.1f cm/s"),
			   Source, *Thief->GetName(), Damage, FMath::Abs(ImpactSpeed));
	}
	else if (AGoatActor* Goat = Cast<AGoatActor>(Victim))
	{
		Goat->ApplyGoatDamage(Damage);
		UE_LOG(LogUnderTheMountain, Log, TEXT("Crush (%s): dragon crushed goat %s for %.1f at %.1f cm/s"),
			   Source, *Goat->GetName(), Damage, FMath::Abs(ImpactSpeed));
	}
}

void ADragonCharacter::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other,
								 UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
								 FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	// [2026-11-01 T-077] Run-over: grounded contact only (airborne
	// impacts are Landed's landing crush), at charge speed, against
	// a crushable actor, debounced so grinding contact is one crush
	// per half-second rather than per physics step.
	if (Combat == nullptr || Other == nullptr || GetWorld() == nullptr ||
		GetCharacterMovement() == nullptr || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	if (!Other->IsA<AThiefCharacter>() && !Other->IsA<AGoatActor>())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastCrushTimeSeconds < 0.5f)
	{
		return;
	}
	const float GroundSpeed = FVector(GetVelocity().X, GetVelocity().Y, 0.0f).Size();
	if (UDragonCombatComponent::CrushDamageForImpact(GroundSpeed, Combat->CrushDamage,
													 Combat->CrushMinImpactSpeed) <= 0.0f)
	{
		return;
	}
	LastCrushTimeSeconds = Now;
	ApplyCrushVictim(Other, GroundSpeed, TEXT("run-over"));
}

bool ADragonCharacter::IsPrototypeFlying() const
{
	// [2026-11-03 T-075] Custom-mode flight counts only while a prototype
	// scheme is selected; scheme 0 keeps shipped semantics everywhere.
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	return Move != nullptr && Move->MovementMode == MOVE_Custom && UDragonMovementComponent::GetFlightScheme() != 0;
}

void ADragonCharacter::UpdatePrototypeFlight(float DeltaSeconds)
{
	// [2026-11-03 T-075] Slice 2b: mode enter/exit, brake feed, stamina
	// classification, CSV rows. Input remap (Surge/Ascent/RMB/F-assist)
	// lands in the next slice; Space still flaps (Ascent impulse) via
	// HandleFlap and Shift still brakes via bDiving.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move == nullptr)
	{
		return;
	}
	const int32 Scheme = UDragonMovementComponent::GetFlightScheme();
	const bool bInCustom = Move->MovementMode == MOVE_Custom;
	if (Scheme != 0 && IsFlying() && !bInCustom)
	{
		Move->SetMovementMode(MOVE_Custom, UDragonMovementComponent::DragonFlightCustomMode);
		ProtoNose = GetControlRotation();
		UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon prototype flight engaged (scheme %d)"), Scheme);
	}
	else if (Scheme == 0 && bInCustom)
	{
		Move->SetMovementMode(MOVE_Flying);
	}
	if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(Move))
	{
		// [2026-11-03 T-075] Course program owns the feeds while bCourseFlying
		// (below keeps drain + CSV running for the legs).
		if (!bCourseFlying)
		{
			// [2026-11-03 T-062] Shift (bDiving) brakes; S-held (remembered
			// Y < -0.5, the adopted flare) brakes the same way. Bleeding speed
			// IS the flare, and the Brake rule (forced precision) covers both.
			// [2026-11-03 T-065] Keyboard S only: pad stick-back pitches
			// instead of flaring (device tag on the sample).
			Proto->bProtoBraking = bDiving || (!bLastMoveFromGamepad && LastMoveAxis.Y < -0.5f);
			Proto->bProtoRise = bRiseHeld;
			Proto->bProtoSink = bSinkHeld;
			if (!IsPrototypeFlying())
			{
				Proto->bLandingAssist = false;
				Proto->bProtoAutoThrust = false;
			}
		}
		if (IsPrototypeFlying())
		{
			// [2026-11-03 T-075] Nose follows control rotation except while
			// free-looking (frozen); latched, it turns toward the view at
			// the airspeed-limited yaw rate (T-063 steer-to-look). The course
			// program owns the nose while bCourseFlying.
			if (!bFreeLook && !bCourseFlying)
			{
				// [2026-11-03 T-062] A/D yaws at the airspeed-limited rate in
				// the momentum band (adopted bank/turn): the controller turns
				// so camera and nose agree, and the nose follows below. The
				// precision band keeps A/D as strafe thrust (integrator
				// PendingInput), so no branch is needed there.
				if (Controller != nullptr && FMath::Abs(LastMoveAxis.X) > 0.1f &&
					UDragonMovementComponent::PrecisionAuthorityWeight(
						Move->Velocity.Size(), Proto->FlightParams) < 0.5f)
				{
					// [2026-11-03 T-103] Trap slow widens turns: yaw scales
					// with the thrust factor.
					AddControllerYawInput(LastMoveAxis.X *
										  UDragonMovementComponent::YawRateForAirspeed(
											  Move->Velocity.Size(), Proto->FlightParams) *
										  Proto->TrapSlowFactor * DeltaSeconds);
				}
				if (bSteerLatch)
				{
					// [2026-11-03 T-103] Latch turns widen under trap slow too.
					const float MaxTurn = UDragonMovementComponent::YawRateForAirspeed(
											  Move->Velocity.Size(), Proto->FlightParams) *
										  Proto->TrapSlowFactor;
					ProtoNose = FMath::RInterpConstantTo(ProtoNose, GetControlRotation(), DeltaSeconds, MaxTurn);
				}
				else
				{
					// [2026-11-03 T-065] Pad left-stick aims the nose in the
					// momentum band (adopted flight grammar): forward stick
					// pitches down toward a dive, back stick pitches up to
					// climb. Keyboard keeps mouse-aim (S never pitches).
					// Runs before the nose-follow below so the nose picks
					// up the pitched view the same tick.
					if (bLastMoveFromGamepad && Controller != nullptr && FMath::Abs(LastMoveAxis.Y) > 0.2f &&
						UDragonMovementComponent::PrecisionAuthorityWeight(
							Move->Velocity.Size(), Proto->FlightParams) < 0.5f)
					{
						AddControllerPitchInput(LastMoveAxis.Y *
												UDragonMovementComponent::YawRateForAirspeed(
													Move->Velocity.Size(), Proto->FlightParams) *
												(InvertPadFlightPitch() ? -1.0f : 1.0f) * DeltaSeconds);
					}
					ProtoNose = GetControlRotation();
				}
			}
			Proto->bProtoAutoThrust = bSteerLatch;
			// [2026-11-03 T-075] Prototype stamina classification (spike
			// economy: dive posture costs 0, precision/hover band 8/s, glide
			// 1/s). Zero stamina keeps glide/brake/land (mercy rule) and
			// burns HP only on draining actions — mirroring the shipped block.
			const float Airspeed = Move->Velocity.Size();
			const float AuthorityW = UDragonMovementComponent::PrecisionAuthorityWeight(
				Airspeed, Proto->FlightParams);
			// [2026-11-03 T-064] Dive is the shared posture rule now (one
			// definition for drain, naming, and tests).
			const bool bDivePosture = UDragonMovementComponent::IsDivePosture(
				GetControlRotation().Pitch, Airspeed, Proto->FlightParams);
			const bool bHoverBand = AuthorityW > 0.5f || Move->Velocity.Size2D() < Stats->HoverSpeedThreshold;
			const float ProtoDrainRate = bDivePosture ? 0.0f
													  : (bHoverBand ? Stats->HoverDrainPerSecond : Stats->GlideDrainPerSecond);
			Stats->DrainForFlight(DeltaSeconds, ProtoDrainRate);
			if (Stats->Stamina <= 0.0f && Stats->ShouldForceFallAtZeroStamina(ProtoDrainRate))
			{
				Stats->DrainHPForExhaustion(DeltaSeconds);
				Move->SetMovementMode(MOVE_Falling);
			}
			if (GetWorld() != nullptr)
			{
				Proto->CaptureFlightCsvRow(GetWorld()->GetTimeSeconds(), Stats->Stamina);
			}
		}
	}
}

void ADragonCharacter::FlightCsv()
{
	// [2026-11-03 T-075] Dev/capture: dump the rows the course judges.
	if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement()))
	{
		Proto->DumpFlightCsvToLog();
	}
}

FRotator ADragonCharacter::GetProtoNose() const
{
	return ProtoNose;
}

void ADragonCharacter::HandleMoveStarted(const FInputActionValue& Value)
{
	// [2026-11-03 T-075] W-tap Surge (adopted KB map): a fresh forward press
	// in the momentum band buys +800 cm/s for one 20-point reserve (1/s
	// precision-band W stays thrust; shipped has no tap.
	if (!IsPrototypeFlying())
	{
		return;
	}
	if (bCourseFlying)
	{
		return;
	}
	if (Value.Get<FVector2D>().Y < 0.5f)
	{
		return;
	}
	UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement());
	if (Proto == nullptr || GetWorld() == nullptr)
	{
		return;
	}
	const float Airspeed = Proto->Velocity.Size();
	if (UDragonMovementComponent::PrecisionAuthorityWeight(Airspeed, Proto->FlightParams) > 0.5f)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastSurgeTime < 1.0f || Stats->Stamina < 20.0f)
	{
		return;
	}
	if (Stats->TryConsumeStamina(20.0f))
	{
		LastSurgeTime = Now;
		Proto->ApplySurge();
		UE_LOG(LogUnderTheMountain, Log, TEXT("Surge: +%.0f cm/s (stamina %.1f)"),
			   Proto->FlightParams.SurgeDeltaV, Stats->Stamina);
	}
}

void ADragonCharacter::HandleMoveEnd()
{
	// [2026-11-03 T-062] Stick release clears the remembered axis so a
	// released S stops flaring and a released A/D stops turning.
	LastMoveAxis = FVector2D::ZeroVector;
}

void ADragonCharacter::HandleRiseHeld(const FInputActionValue& Value)
{
	// [2026-11-03 T-075] Space-hold rise feed (blend band only; the press
	// edge stays the Ascent impulse in HandleFlap). Shipped ignores holds.
	(void)Value;
	bRiseHeld = true;
}

void ADragonCharacter::HandleRiseEnd()
{
	bRiseHeld = false;
}

void ADragonCharacter::HandleFreeLook(const FInputActionValue& Value)
{
	// [2026-11-03 T-063] RMB hold = free look (nose frozen, camera
	// roams); double-click toggles latched steer-to-look. Shipped since
	// T-062 (Scheme C default).
	(void)Value;
	if (UDragonMovementComponent::GetFlightScheme() == 0)
	{
		return;
	}
	const float Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Now - LastRMBReleaseTime < 0.35f)
	{
		bSteerLatch = !bSteerLatch;
		bFreeLook = false;
		UE_LOG(LogUnderTheMountain, Log, TEXT("Steer-to-look %s"),
			   bSteerLatch ? TEXT("LATCHED") : TEXT("released"));
	}
	else
	{
		bFreeLook = true;
	}
}

void ADragonCharacter::HandleFreeLookEnd()
{
	// [2026-11-03 T-075] Camera returns to the nose on release so the next
	// tick's nose-follow has nothing to snap.
	bFreeLook = false;
	if (GetWorld() != nullptr)
	{
		LastRMBReleaseTime = GetWorld()->GetTimeSeconds();
	}
	if (!bSteerLatch && Controller != nullptr)
	{
		Controller->SetControlRotation(ProtoNose);
	}
}

void ADragonCharacter::HandleSteerLatch()
{
	// [2026-11-03 T-063] Gamepad D-pad Up equivalent of double-click RMB:
	// toggles the latch (a second press releases, like the double-click).
	// Shipped on the T-062 hybrid; IsPrototypeFlying is true in MOVE_Custom.
	if (IsPrototypeFlying())
	{
		bSteerLatch = !bSteerLatch;
		UE_LOG(LogUnderTheMountain, Log, TEXT("Steer-to-camera %s (pad)"),
			   bSteerLatch ? TEXT("LATCHED") : TEXT("released"));
	}
}

void ADragonCharacter::HandleSink(const FInputActionValue& Value)
{
	// [2026-11-03 T-075] Ctrl-hold sink feed (blend band only).
	(void)Value;
	bSinkHeld = true;
}

void ADragonCharacter::HandleSinkEnd()
{
	bSinkHeld = false;
}

void ADragonCharacter::TickFlightCourse(float DeltaSeconds)
{
	// [2026-11-03 T-075] Scripted T-075 legs in the LIVE world (real ticks,
	// real Lair geometry): takeoff, climb, cruise, 360 turn, brake, hover,
	// assisted landing. Arms only when the course cvar AND a prototype scheme
	// are set — default-off, so shipped players never meet it. Each leg logs
	// one CourseLeg summary line; per-tick rows ride the normal CSV capture;
	// the run ends by quitting the game. Replaces the deleted bare-world
	// CourseBot: manually-ticked worlds never forwarded actor ticks, so the
	// lab measured nothing (see T-075 loop notes 2026-11-03).
	if (CoursePhase < 0)
	{
		if (CVarFlightCourse.GetValueOnGameThread() != 0 && UDragonMovementComponent::GetFlightScheme() != 0 && Stats != nullptr)
		{
			Stats->Stamina = Stats->MaxStamina;
			bCourseFlying = true;
			CoursePhase = 0;
			CoursePhaseTime = 0.0f;
			CourseTotalTime = 0.0f;
			CourseLog(TEXT("FlightCourse armed: full pool, scheme %d (legs: climb, turn, brake, hover, land)"),
					  UDragonMovementComponent::GetFlightScheme());
		}
		else
		{
			return;
		}
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(Move);
	if (Move == nullptr || Proto == nullptr)
	{
		return;
	}
	CoursePhaseTime += DeltaSeconds;
	CourseTotalTime += DeltaSeconds;
	if (CourseTotalTime > 150.0f)
	{
		CourseLog(TEXT("FlightCourse watchdog: quitting"));
		CoursePhase = 99;
		QuitCourseGame();
		return;
	}
	const float BaseYaw = GetControlRotation().Yaw;
	switch (CoursePhase)
	{
	case 0: // Takeoff: Ascent launch on the same 20-reserve gate as HandleFlap.
		if (CoursePhaseTime <= DeltaSeconds * 1.5f && !IsPrototypeFlying())
		{
			Move->SetMovementMode(MOVE_Custom, UDragonMovementComponent::DragonFlightCustomMode);
			ProtoNose = GetControlRotation();
			if (Stats->Stamina >= 20.0f && Stats->TryConsumeStamina(20.0f))
			{
				Proto->ApplyAscent();
			}
			CourseMark = GetActorLocation();
		}
		ProtoNose = FRotator(60.0f, BaseYaw, 0.0f);
		if (CoursePhaseTime >= 3.0f)
		{
			CourseLog(TEXT("CourseLeg liftoff: alt=%.0fcm"),
					  GetActorLocation().Z - CourseMark.Z);
			// [2026-11-03 T-075] Transit: the 30 m chambers cannot host the
			// cruise/turn legs, so climb/cruise/turn/brake/hover fly in open
			// sky above the peak (shaft top 5200 + margin). Teleports are
			// logged, never silent; chimney/cave geometry stays owner-flown.
			SetActorLocation(FVector(19000.0f, 0.0f, 8000.0f), false, nullptr, ETeleportType::TeleportPhysics);
			Move->Velocity = FVector::ZeroVector;
			CourseLog(TEXT("CourseLeg transit: open sky above peak"));
			CoursePhase = 1;
			CoursePhaseTime = 0.0f;
			CourseMark = GetActorLocation();
			// [2026-11-03 T-075] Climb entry needs energy: a second reserve
			// buys a Surge the nose-up cruise then converts to altitude.
			if (Stats->Stamina >= 20.0f && Stats->TryConsumeStamina(20.0f))
			{
				Proto->ApplySurge();
				CourseLog(TEXT("CourseLeg climb: entry surge (stamina %.1f)"), Stats->Stamina);
			}
			RestoreCoursePool(TEXT("liftoff"));
		}
		break;
	case 1: // Climb: sustained nose-up cruise (chimney proxy: climb rate).
		ProtoNose = FRotator(30.0f, BaseYaw, 0.0f);
		if (CoursePhaseTime >= 5.0f)
		{
			CourseLog(TEXT("CourseLeg climb: +%.0fcm in 5s (%.0fcm/s)"),
					  GetActorLocation().Z - CourseMark.Z,
					  (GetActorLocation().Z - CourseMark.Z) / 5.0f);
			CoursePhase = 2;
			CoursePhaseTime = 0.0f;
			CourseMark = GetActorLocation();
			RestoreCoursePool(TEXT("climb"));
			if (Stats->Stamina >= 20.0f && Stats->TryConsumeStamina(20.0f))
			{
				Proto->ApplySurge();
				CourseLog(TEXT("CourseLeg cruise: surged (stamina %.1f)"), Stats->Stamina);
			}
		}
		break;
	case 2: // Cruise: level nose, build speed for the turn.
		ProtoNose = FRotator(0.0f, BaseYaw, 0.0f);
		if (CoursePhaseTime >= 4.0f)
		{
			CoursePhase = 3;
			CoursePhaseTime = 0.0f;
			CourseTurnAccum = 0.0f;
			RestoreCoursePool(TEXT("cruise"));
			CourseBaseYaw = BaseYaw;
			CourseMinXY = GetActorLocation();
			CourseMaxXY = GetActorLocation();
		}
		break;
	case 3: // Turn: full 360 at the airspeed-limited yaw rate; measure diameter.
	{
		const float TurnRate = UDragonMovementComponent::YawRateForAirspeed(
			Move->Velocity.Size(), Proto->FlightParams);
		CourseTurnAccum += TurnRate * DeltaSeconds;
		ProtoNose = FRotator(0.0f, CourseBaseYaw + CourseTurnAccum, 0.0f);
		const FVector P = GetActorLocation();
		CourseMinXY.X = FMath::Min(CourseMinXY.X, P.X);
		CourseMinXY.Y = FMath::Min(CourseMinXY.Y, P.Y);
		CourseMaxXY.X = FMath::Max(CourseMaxXY.X, P.X);
		CourseMaxXY.Y = FMath::Max(CourseMaxXY.Y, P.Y);
		if (CourseTurnAccum >= 360.0f)
		{
			const float Diameter = FMath::Max(CourseMaxXY.X - CourseMinXY.X, CourseMaxXY.Y - CourseMinXY.Y);
			CourseLog(TEXT("CourseLeg turn: diameter=%.0fcm in %.1fs at cruise (T-066 evidence)"),
					  Diameter, CoursePhaseTime);
			CoursePhase = 4;
			CoursePhaseTime = 0.0f;
			CourseMark = GetActorLocation();
			RestoreCoursePool(TEXT("turn"));
			Proto->bProtoBraking = true;
		}
		break;
	}
	case 4: // Brake: full brake to rest; measure stop distance.
		ProtoNose = FRotator(0.0f, BaseYaw, 0.0f);
		if (Move->Velocity.Size() < 60.0f || CoursePhaseTime >= 6.0f)
		{
			Proto->bProtoBraking = false;
			CourseLog(TEXT("CourseLeg brake: stop in %.0fcm over %.1fs"),
					  (GetActorLocation() - CourseMark).Size2D(), CoursePhaseTime);
			CoursePhase = 5;
			CoursePhaseTime = 0.0f;
			CourseMark = GetActorLocation();
			RestoreCoursePool(TEXT("brake"));
		}
		break;
	case 5: // Hover: dead stick; measure drift (chimney/cave enabler).
		if (CoursePhaseTime >= 5.0f)
		{
			CourseLog(TEXT("CourseLeg hover: drift=%.0fcm over 5s (stamina %.1f)"),
					  (GetActorLocation() - CourseMark).Size(), Stats->Stamina);
			// [2026-11-03 T-075] Transit: assisted landing flies onto the Entry
			// floor by the PlayerStart disc (guaranteed clear spawn ground —
			// the hoard center proved unwedgeable: SeeingStone + chests at
			// (19000,0) wedged two land legs at z=438 with velocity running
			// but position frozen). T-066 owes the hoard a clear touchdown
			// disc for the same reason.
			SetActorLocation(FVector(0.0f, 0.0f, 900.0f), false, nullptr, ETeleportType::TeleportPhysics);
			Move->Velocity = FVector::ZeroVector;
			CourseLog(TEXT("CourseLeg transit: entry approach (0,0,900)"));
			RestoreCoursePool(TEXT("hover"));
			CoursePhase = 6;
			CoursePhaseTime = 0.0f;
			Proto->bLandingAssist = true;
			// [2026-11-03 T-075] Precision hover-hold ignores nose pitch, so
			// the leg commands descent the same way Ctrl does.
			Proto->bProtoSink = true;
			CourseLog(TEXT("CourseLeg land: assist ON, descending"));
		}
		break;
	case 6: // Assisted landing: slight nose-down until walk, or timeout.
		ProtoNose = FRotator(-10.0f, BaseYaw, 0.0f);
		CourseTouchSpeed = Move->Velocity.Size();
		if (Move->MovementMode == MOVE_Walking || CoursePhaseTime >= 12.0f)
		{
			Proto->bLandingAssist = false;
			Proto->bProtoSink = false;
			CourseLog(TEXT("CourseLeg land: %s touchdown at %.0fcm/s, HP %.1f (assist %s)"),
					  Move->MovementMode == MOVE_Walking ? TEXT("clean") : TEXT("TIMEOUT no"),
					  CourseTouchSpeed, Stats->HP,
					  Move->MovementMode == MOVE_Walking ? TEXT("held") : TEXT("beat by timeout"));
			CoursePhase = 7;
			CoursePhaseTime = 0.0f;
		}
		break;
	case 7: // Done: dump CSV to file (headless runs keep no UE log), summarize, quit.
		Proto->DumpFlightCsvToFile(FPaths::ProjectSavedDir() / TEXT("CourseFly") / FString::Printf(TEXT("scheme%d.csv"), UDragonMovementComponent::GetFlightScheme()));
		CourseLog(TEXT("FlightCourse COMPLETE (%d rows in Saved/CourseFly)"), Proto->FlightCsvRows.Num());
		CoursePhase = 99;
		QuitCourseGame();
		break;
	default:
		break;
	}
}

void ADragonCharacter::QuitCourseGame()
{
	// [2026-11-03 T-075] Ends a scripted course run: player path first,
	// hard exit when headless/controller-less.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ConsoleCommand(TEXT("quit"));
	}
	else
	{
		FPlatformMisc::RequestExit(0);
	}
}

void ADragonCharacter::RestoreCoursePool(const TCHAR* LegName)
{
	// [2026-11-03 T-075] Handling isolation: each leg starts on a full pool
	// so exhaustion can never gate a handling measurement. Every restore is
	// logged with the finished leg's spend, and per-tick stamina still rides
	// the CSV — nothing about the economy is hidden, it is controlled.
	if (Stats != nullptr)
	{
		CourseLog(TEXT("CourseLeg %s: spent %.1f stamina, pool restored"),
				  LegName, Stats->MaxStamina - Stats->Stamina);
		Stats->Stamina = Stats->MaxStamina;
	}
}

void ADragonCharacter::CourseLog(const TCHAR* Fmt, ...)
{
	// [2026-11-03 T-075] Dual evidence channel: the UE log (normal runs) AND
	// raw stdout (headless -game runs in this pipeline never materialise a
	// log file — proven over four silent 35 s exits). fflush so a quit or
	// crash cannot eat buffered lines.
	TCHAR Buf[1024];
	GET_TYPED_VARARGS(TCHAR, Buf, UE_ARRAY_COUNT(Buf), UE_ARRAY_COUNT(Buf) - 1, Fmt, Fmt);
	UE_LOG(LogUnderTheMountain, Log, TEXT("%s"), Buf);
	printf("COURSE %s\n", TCHAR_TO_UTF8(Buf));
	fflush(stdout);
}

void ADragonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// [2026-11-01 T-052] Weather follows the cvar: a console or
	// -ExecCmds change lands here within a frame and applies once
	// (ApplyWeather only runs on an actual change).
	const int32 DesiredWeather = FMath::Clamp(CVarWeatherState.GetValueOnGameThread(), 0, 1);
	if (DesiredWeather != CurrentWeatherState)
	{
		ApplyWeather(DesiredWeather);
	}
	// [2026-11-01 T-019] HUD: create once for the local player,
	// then feed it the subsystems' numbers every tick. This is
	// the live readout the T-016 debug line prototyped; the debug
	// line stays (it also works headless in logs' stead).
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			if (HUDWidget == nullptr)
			{
				HUDWidget = CreateWidget<UDragonHUDWidget>(PC, UDragonHUDWidget::StaticClass());
				if (HUDWidget != nullptr)
				{
					HUDWidget->AddToViewport(5);
				}
			}
			if (HUDWidget != nullptr && GetWorld() != nullptr)
			{
				UNotorietySubsystem* Notoriety = GetWorld()->GetSubsystem<UNotorietySubsystem>();
				UHoardSubsystem* Hoard = GetWorld()->GetSubsystem<UHoardSubsystem>();
				UWaveSubsystem* Waves = GetWorld()->GetSubsystem<UWaveSubsystem>();
				UTreasureSubsystem* TreasureSub = GetWorld()->GetSubsystem<UTreasureSubsystem>();
				UXPSubsystem* HUDXP = GetWorld()->GetSubsystem<UXPSubsystem>();
				if (Notoriety && Hoard && Waves && TreasureSub && TreasureSub->Registry() && Stats && HUDXP)
				{
					static const TCHAR* WaveWords[] = {TEXT("Intermission"), TEXT("Spawning"),
													   TEXT("Active"), TEXT("Session over")};
					const int32 WaveIndex =
						FMath::Clamp(static_cast<int32>(Waves->GetWaveState()), 0, 3);
					// [2026-11-03 T-064] Flight-state line: one classification,
					// caller-worded like WaveStateText. Only flight modes show
					// a line — grounded (and falling) pass empty and the
					// widget collapses it.
					bool bFlightDive = false;
					bool bFlightBrake = false;
					bool bFlightHover = false;
					FlightStateFlags(bFlightDive, bFlightBrake, bFlightHover);
					const FString FlightStateText = (IsPrototypeFlying() || IsFlying())
														? UDragonHUDWidget::FormatFlightState(bFlightDive, bFlightBrake, bFlightHover)
														: FString();
					HUDWidget->Refresh(Stats->HP, Stats->MaxHP, Stats->Stamina, Stats->MaxStamina,
									   Notoriety->GetStars(), Notoriety->GetPoints(),
									   Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold()),
									   Hoard->GetStartingHoard(), Waves->GetCurrentWave(),
									   HUDXP->GetTotalXP(), WaveWords[WaveIndex], FlightStateText);
				}
			}
		}
	}
	// [2026-11-01 T-018] Intro sequence (camera, gnome, choice).
	// [2026-11-03 T-072] The run starts at the title screen: the intro
	// waits for StartPlay (HasRunStarted) instead of playing behind
	// the title. The cvar path below is unchanged (skip still skips).
	if (UWorld* IntroWorld = GetWorld())
	{
		if (const AUnderTheMountainGameMode* Mode =
				IntroWorld->GetAuthGameMode<AUnderTheMountainGameMode>())
		{
			if (Mode->HasRunStarted())
			{
				TickIntro(DeltaSeconds);
			}
		}
		else
		{
			TickIntro(DeltaSeconds);
		}
	}
	// [2026-11-01 T-016] Game loop: waves, notoriety/hoard
	// accounting and outcome classification, driven from here
	// (the pawn is the one actor that always ticks).
	AuditGameLoop(DeltaSeconds);
	// [2026-11-03 T-079] Wall-impact pricing: the integrator records
	// the hardest substep hit; the pawn prices it here on the shipped
	// landing curve (same rule the pure test pins — source tagged
	// collision in the log; T-074 owns the enemy-hit event and any
	// future threshold divergence).
	if (Stats != nullptr)
	{
		if (UDragonMovementComponent* Proto = Cast<UDragonMovementComponent>(GetCharacterMovement()))
		{
			const UDragonMovementComponent::FWallImpact Hit = Proto->ConsumeWallImpact();
			if (Hit.bPending)
			{
				const UDragonMovementComponent::FWallImpactResult Wall =
					UDragonMovementComponent::ResolveWallImpact(Hit.IntoSpeed, *Stats);
				if (Wall.DamageHP > 0.0f)
				{
					Stats->ApplyHPDamage(Wall.DamageHP);
					UE_LOG(LogUnderTheMountain, Log,
						   TEXT("Wall impact: %.0f cm/s into the surface, %d HP (collision source)"),
						   Hit.IntoSpeed, FMath::RoundToInt(Wall.DamageHP));
				}
			}
		}
	}
	// [2026-11-03 T-073] First-use prompts tick beside the intro (both
	// gate on run-started + intro-done inside).
	TickOnboarding(DeltaSeconds);
	// [2026-11-03 T-075] Prototype chassis housekeeping (no-op on scheme 0).
	UpdatePrototypeFlight(DeltaSeconds);
	// [2026-11-03 T-075] Scripted course legs (arms only on utm.flight.course).
	TickFlightCourse(DeltaSeconds);
	// [2026-10-28 T-014] Trap-slow expiry: restore the captured speeds
	// in the same tick the clock runs out.
	// [2026-11-03 T-103] Clock + restore live in TickTrapSlow (testable
	// without a world); Tick just pumps it.
	TickTrapSlow(DeltaSeconds);
	// [2026-11-03 T-065] Camera rig follows every tick (walk and flight):
	// head-anchored pivot, precision-shortened arm, first-person mesh hide.
	ApplyCameraRig(DeltaSeconds);
	// [2026-10-28 T-015] Combat, before flight work: the component never
	// ticks itself, so the pawn drives fire + eat here. Targets are
	// gathered only while fire is wanted (TickEat needs none), so an
	// idle dragon pays no actor scan. Fire origin sits at the nose
	// (150cm ahead, 60cm up); aim follows the control rotation, with
	// actor forward as the no-controller fallback.
	if (Combat != nullptr)
	{
		TArray<UThiefBrain*> CombatBrains;
		TArray<FVector> CombatLocations;
		TArray<AGoatActor*> CombatGoats;
		TArray<FVector> CombatGoatLocations;
		TArray<ABurnablePropActor*> CombatProps;
		TArray<FVector> CombatPropLocations;
		if (bWantsFire)
		{
			GatherCombatTargets(CombatBrains, CombatLocations);
			GatherGoatTargets(CombatGoats, CombatGoatLocations);
			GatherBurnableTargets(CombatProps, CombatPropLocations);
			if (!Combat->IsFiring())
			{
				Combat->TryStartFire(Stats);
			}
		}
		if (Combat->IsFiring())
		{
			const FVector FireOrigin =
				GetActorLocation() + GetActorForwardVector() * 150.0f + FVector(0.0f, 0.0f, 60.0f);
			const FVector FireForward =
				Controller != nullptr ? GetControlRotation().Vector() : GetActorForwardVector();
			Combat->TickFire(DeltaSeconds, Stats, FireOrigin, FireForward, CombatBrains,
							 CombatLocations, CombatGoats, CombatGoatLocations, CombatProps,
							 CombatPropLocations);
			MaybeSpawnGroundScorch(FireOrigin, FireForward);
		}
		// [2026-11-01 T-055] Burning props keep hurting after the
		// breath stops, so this runs every tick, firing or not.
		ApplyBurningAreaDamage(DeltaSeconds);
		// [2026-11-01 T-051] Breath presentation follows IsFiring
		// exactly: the pooled Niagara activates once per breath (no
		// per-frame spawns) and the point light flickers around a
		// base intensity while lit, so terrain and thieves in the
		// cone catch firelight. utm.fire.forcevisuals (below) can
		// hold the visuals on for captures/measurements without
		// touching damage, which stays Combat's math alone.
		const bool bBreathVisible = Combat->IsFiring() || ForceFireVisuals();
		if (FireBreathFX != nullptr)
		{
			if (bBreathVisible && !FireBreathFX->IsActive())
			{
				FireBreathFX->Activate(true);
			}
			else if (!bBreathVisible && FireBreathFX->IsActive())
			{
				FireBreathFX->Deactivate();
			}
		}
		if (FireLight != nullptr)
		{
			FireLight->SetVisibility(bBreathVisible);
			if (bBreathVisible)
			{
				const float Flicker =
					0.85f + 0.3f * FMath::Sin(GetWorld() ? GetWorld()->GetTimeSeconds() * 37.0f : 0.0f);
				FireLight->SetIntensity(9000.0f * Flicker);
			}
		}
		// [2026-10-28 T-015] The component exposes no IsEating(): TickEat
		// is called every tick and its result is the state (None = no
		// meal). AteThief is the corpse-consumed signal — hide the
		// remembered actor here; AteGoat needs nothing (Consume hid it).
		const EEatResult EatResult = Combat->TickEat(DeltaSeconds, Stats);
		if (EatResult == EEatResult::AteThief)
		{
			if (PendingEatActor != nullptr)
			{
				PendingEatActor->SetActorHiddenInGame(true);
				PendingEatActor = nullptr;
			}
			UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon ate a thief corpse"));
		}
		else if (EatResult == EEatResult::AteGoat)
		{
			UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon ate a goat"));
			// [2026-11-01 T-016] Livestock eaten at a settlement is
			// witnessed (+3); eaten inside the lair gains nothing
			// (the same inside/outside rule as kills).
			if (GetWorld() != nullptr &&
				!UNotorietySubsystem::IsInsideLair(GetActorLocation()))
			{
				if (UNotorietySubsystem* Notoriety = GetWorld()->GetSubsystem<UNotorietySubsystem>())
				{
					Notoriety->AddLivestockEatPoints();
				}
			}
		}
	}
	// [2026-11-01 T-068] Animation driver. Locomotion states come from
	// [2026-11-01 T-068] UDragonAnimInstance (Idle/Walk/Fly/Glide/Hover/Dive),
	// [2026-11-01 T-068] each with its own clip now (Glide is no longer slowed
	// [2026-11-01 T-068] Fly; Dive reuses the held Glide pose near-frozen).
	// [2026-11-01 T-068] Play rate follows the wingbeat curves EVERY tick, not
	// [2026-11-01 T-068] just on transitions, so a climb visibly works harder.
	// [2026-11-01 T-068] One-shots (takeoff/land/roar/hit) suppress locomotion
	// [2026-11-01 T-068] until OneShotTimeRemaining expires. A null clip for a
	// [2026-11-01 T-068] state falls back to FlyAnim, then bind pose — an
	// [2026-11-01 T-068] un-imported checkout flies exactly like T-043 did.
	if (USkeletalMeshComponent* Mesh = GetMesh())
	{
		const FVector AnimVelocity = GetCharacterMovement()->Velocity;
		const float AnimHS = AnimVelocity.Size2D();
		// [2026-11-02 T-058] Airborne means Flying OR Falling: exhaustion
		// [2026-11-02 T-058] (stamina 0, Tick below) and ledge walk-offs set
		// [2026-11-02 T-058] MOVE_Falling, and selecting ground states there
		// [2026-11-02 T-058] played the Walk clip in mid-air — folded swept
		// [2026-11-02 T-058] wings + stepping legs on a falling dragon (owner
		// [2026-11-02 T-058] screenshots 2026-11-02, stamina ~5). A dragon in
		// [2026-11-02 T-058] the air uses flight poses, full stop.
		const bool bAirborne =
			IsFlying() || IsPrototypeFlying() || GetCharacterMovement()->MovementMode == MOVE_Falling;
		if (OneShotTimeRemaining > 0.0f)
		{
			OneShotTimeRemaining -= DeltaSeconds;
			if (OneShotTimeRemaining <= 0.0f)
			{
				// [2026-11-01 T-068] Force locomotion re-entry next tick.
				CurrentAnimState = static_cast<EDragonAnimState>(255);
			}
		}
		else
		{
			const EDragonAnimState DesiredState =
				SelectAnimState(bAirborne, bDiving, AnimHS, AnimVelocity.Z);
			if (DesiredState != CurrentAnimState)
			{
				UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon anim state -> %d (flying=%d diving=%d)"),
					   static_cast<int32>(DesiredState), IsFlying() ? 1 : 0, bDiving ? 1 : 0);
				CurrentAnimState = DesiredState;
				UAnimSequence* Clip = nullptr;
				switch (DesiredState)
				{
				case EDragonAnimState::Idle:
					Clip = IdleAnim;
					break;
				case EDragonAnimState::Walk:
					Clip = WalkAnim;
					break;
				case EDragonAnimState::Glide:
				case EDragonAnimState::Dive:
					Clip = GlideAnim != nullptr ? GlideAnim : FlyAnim;
					break;
				case EDragonAnimState::Hover:
					Clip = HoverAnim != nullptr ? HoverAnim : FlyAnim;
					break;
				default:
					Clip = FlyAnim;
					break;
				}
				// [2026-11-04 T-113] Dual channel like CourseLog (T-075):
				// [2026-11-04 T-113] headless -game runs never materialise a
				// [2026-11-04 T-113] log file, so state + clip proof needs
				// [2026-11-04 T-113] stdout. A null clip here means bind pose.
				printf("ANIMSTATE state=%d flying=%d diving=%d clip=%s\n", static_cast<int32>(DesiredState),
					   IsFlying() ? 1 : 0, bDiving ? 1 : 0,
					   Clip != nullptr ? TCHAR_TO_UTF8(*Clip->GetName()) : "BIND");
				fflush(stdout);
				if (Clip != nullptr)
				{
					Mesh->PlayAnimation(Clip, true);
				}
				else
				{
					Mesh->Stop();
				}
			}
			Mesh->SetPlayRate(
				UDragonAnimInstance::ComputeFlapPlayRate(CurrentAnimState, AnimHS, AnimVelocity.Z));
		}
		// [2026-11-01 T-068] Visual body tilt: pitch with the flight path,
		// [2026-11-01 T-068] bank into turns. Mesh-only (base yaw -90 from the
		// [2026-11-01 T-068] constructor); the capsule and movement never tilt,
		// [2026-11-01 T-068] so collision truth is untouched. Smoothed so a
		// [2026-11-01 T-068] velocity step can never snap the body.
		{
			const float Yaw = GetActorRotation().Yaw;
			const float YawRate = DeltaSeconds > 0.0f
									  ? FMath::FindDeltaAngleDegrees(LastActorYaw, Yaw) / DeltaSeconds
									  : 0.0f;
			LastActorYaw = Yaw;
			const float TargetBank =
				bAirborne ? UDragonAnimInstance::ComputeBankDegrees(YawRate, AnimHS) : 0.0f;
			const float TargetPitch =
				bAirborne ? UDragonAnimInstance::ComputeMeshPitchDegrees(AnimHS, AnimVelocity.Z) : 0.0f;
			const float Blend = FMath::Clamp(DeltaSeconds * 5.0f, 0.0f, 1.0f);
			MeshBankDegrees = FMath::Lerp(MeshBankDegrees, TargetBank, Blend);
			MeshPitchDegrees = FMath::Lerp(MeshPitchDegrees, TargetPitch, Blend);
			Mesh->SetRelativeRotation(FRotator(MeshPitchDegrees, -90.0f, MeshBankDegrees));
		}
	}
	// [2026-10-28 T-011] Snapshot vertical velocity before any flight work
	// this frame can change it; Landed() reads this when the ground wins.
	LastVelocityZ = GetCharacterMovement()->Velocity.Z;

	// [2026-10-28 T-011] Flight mode name + drain rate for this tick, in
	// one place so the debug readout below can never disagree with what
	// was actually charged. GAMEPLAY.md section 2: dive 0/s (altitude
	// pays), hover 8/s (priced out), glide 0/s (unpowered travel).
	// [2026-11-03 T-062] This legacy block runs only on scheme 0 now: the
	// hybrid (default) drains in UpdatePrototypeFlight above.
	const FVector Velocity = GetCharacterMovement()->Velocity;
	const float HorizontalSpeed = Velocity.Size2D();
	const bool bHovering = IsFlying() && !bDiving && HorizontalSpeed < Stats->HoverSpeedThreshold;
	const float FlightDrainRate =
		bDiving ? 0.0f : (bHovering ? Stats->HoverDrainPerSecond : Stats->GlideDrainPerSecond);
	const TCHAR* FlightModeName = TEXT("Ground");
	if (IsFlying())
	{
		FlightModeName = bDiving ? TEXT("Dive") : (bHovering ? TEXT("Hover") : TEXT("Glide"));
		// [2026-10-28 T-011] DrainForFlight clamps at 0, so calling it at
		// empty is harmless; the exhaustion branch below owns empty.
		Stats->DrainForFlight(DeltaSeconds, FlightDrainRate);
		if (bDiving)
		{
			const FVector DiveDir = GetControlRotation().Vector();
			GetCharacterMovement()->Velocity += DiveDir * DiveAcceleration * DeltaSeconds;
		}
		if (Stats->Stamina <= 0.0f && Stats->ShouldForceFallAtZeroStamina(FlightDrainRate))
		{
			// [2026-10-28 T-036] Only a stamina-spending action has an
			// exhaustion consequence. A dive reaches this point with a
			// zero rate, so it keeps flying without HP burn; glide and
			// hover have positive rates and therefore burn HP before
			// flight ends. Checked after the stamina drain so the tick
			// that empties the pool is also the first tick that pays.
			Stats->DrainHPForExhaustion(DeltaSeconds);
			// [2026-10-27 T-003] Exhausted dragon glides down, not falls out of
			// control: force walking mode so landing logic takes over.
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	}
	else if (IsPrototypeFlying())
	{
		// [2026-11-03 T-064] Hybrid naming for the readout only — drain ran
		// in UpdatePrototypeFlight above, so this branch never spends.
		// Player terms, matching the HUD line: a dive names its recovery,
		// level cruise names how to enter one.
		bool bNameDive = false;
		bool bNameBrake = false;
		bool bNameHover = false;
		FlightStateFlags(bNameDive, bNameBrake, bNameHover);
		FlightModeName = bNameDive ? TEXT("Dive - pull up or Brake")
								   : (bNameBrake ? TEXT("Braking")
												 : (bNameHover ? TEXT("Hover - precision hold") : TEXT("Glide - nose down to dive")));
	}
	else if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		FlightModeName = TEXT("Falling");
	}
	else if (GetCharacterMovement()->MovementMode == MOVE_Walking)
	{
		Stats->RegenOnGround(DeltaSeconds);
	}

	// [2026-10-28 T-011] Flight debug readout for PT-STAM feel work: one
	// stable-key line (replaced in place, never scrolling) shown while
	// airborne. GEngine is null in headless automation runs, hence the
	// guard — the -nullrhi test path must never dereference it.
	if (GEngine)
	{
		// [2026-11-01 T-016] Game-state line, always on (T-019 owns
		// the real HUD; this is the debug readout it will replace):
		// stars/points, hoard of starting, wave and its state.
		if (UWorld* World = GetWorld())
		{
			UNotorietySubsystem* Notoriety = World->GetSubsystem<UNotorietySubsystem>();
			UHoardSubsystem* Hoard = World->GetSubsystem<UHoardSubsystem>();
			UWaveSubsystem* Waves = World->GetSubsystem<UWaveSubsystem>();
			UTreasureSubsystem* TreasureSub = World->GetSubsystem<UTreasureSubsystem>();
			if (Notoriety && Hoard && Waves && TreasureSub && TreasureSub->Registry())
			{
				GEngine->AddOnScreenDebugMessage(
					11012, 0.25f, FColor::Cyan,
					FString::Printf(TEXT("Notoriety %d star (%d pts) | Hoard %lld/%lld | Wave %d state %d | Scent %s"),
									Notoriety->GetStars(), Notoriety->GetPoints(),
									Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold()),
									Hoard->GetStartingHoard(), Waves->GetCurrentWave(),
									static_cast<int32>(Waves->GetWaveState()),
									// [2026-11-03 T-061] The N-key scent-layer
									// state, readable without opening the log.
									Scent->IsScentVisible() ? TEXT("on") : TEXT("OFF")));
			}
		}
	}
	if (GEngine && (IsFlying() || IsPrototypeFlying() || GetCharacterMovement()->MovementMode == MOVE_Falling))
	{
		const FVector ShownVelocity = GetVelocity();
		GEngine->AddOnScreenDebugMessage(
			11011, 0.1f, FColor::Yellow,
			FString::Printf(TEXT("Vel %s | speed %.0f | vSpeed %.0f | %s | stamina %.1f | HP %.1f"),
							*ShownVelocity.ToString(), ShownVelocity.Size(), ShownVelocity.Z, FlightModeName,
							Stats->Stamina, Stats->HP));
	}
}
