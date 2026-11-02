// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Dragon walking-skeleton movement: ACharacter gives us
// walking + flying movement modes; flap/dive are velocity work on top, so
// T-011 feel tuning changes numbers, not architecture.
// [2026-10-28 T-011] The feel tuning that line promised: drain rate is
// chosen per tick from what the dragon is actually doing (dive/glide/
// hover), exhaustion burns HP, Landed() pays for hard impacts, and a
// debug readout makes velocity/drain visible while tuning in PIE.

#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonAppearanceComponent.h"
#include "Dragon/DragonCombatComponent.h"
#include "Dragon/DragonStatsComponent.h"
#include "Livestock/GoatActor.h"
#include "Scent/ScentComponent.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefCharacter.h"
#include "Thief/ThiefDataAsset.h"
#include "UI/DragonTintWidget.h"
#include "UI/IntroChoiceWidget.h"
#include "UnderTheMountain.h"
#include "Hoard/HoardSubsystem.h"
#include "Intro/IntroSubsystem.h"
#include "Notoriety/NotorietySubsystem.h"
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
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
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
} // namespace

ADragonCharacter::ADragonCharacter()
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

	// [2026-10-28 T-032] The real dragon: SK_Dragon_v4 (T-029 rig/walk)
	// replaces the greybox ball as the visible body. Probed bounds are
	// 190 long (local Y, head at -Y in the source rig) x 48 wide x 72
	// tall (cm), origin at body centre — so scale 2 gives a 380cm
	// dragon whose feet (local z -72 after scaling) sit exactly on the
	// capsule floor at pawn z -96 when the mesh is at z -24. Yaw -90
	// turns the mesh's -Y head onto the pawn's +X forward; the PIE
	// screenshot in this task is the evidence that sign is right.
	// Collision stays on the capsule: the mesh is display only.
	GetMesh()->SetVisibility(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DragonMesh(
		TEXT("/Game/Meshy/Dragon/SK_Dragon_v4.SK_Dragon_v4"));
	if (DragonMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(DragonMesh.Object);
	}
	else
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("SK_Dragon_v4 not found; dragon pawn will be invisible (capsule only)"));
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
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (TrapSlowTimeRemaining <= 0.0f)
	{
		UnslowedWalkSpeed = Move->MaxWalkSpeed;
		UnslowedFlySpeed = Move->MaxFlySpeed;
		Move->MaxWalkSpeed *= Factor;
		Move->MaxFlySpeed *= Factor;
	}
	TrapSlowTimeRemaining = FMath::Max(TrapSlowTimeRemaining, Seconds);
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon trap-slowed: factor %.2f for %.1fs"), Factor, Seconds);
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
		   TintAction != nullptr && FireAction != nullptr && RoarAction != nullptr &&
		   EatAction != nullptr && WingBuffetAction != nullptr && TailSlamAction != nullptr &&
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
	if (FlyAnim == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("SK_Dragon_v4_FlyAnim not found; flight will use bind pose"));
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon pawn spawned at %s"), *GetActorLocation().ToString());
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
	TrueSightAction = MakeAction(TEXT("IA_TrueSight"), EInputActionValueType::Boolean);
	TintAction = MakeAction(TEXT("IA_TintPicker"), EInputActionValueType::Boolean);
	// [2026-10-28 T-015] Combat verbs: Boolean like Flap — Fire reads
	// the held value, Roar/Eat act on the press edge.
	FireAction = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	RoarAction = MakeAction(TEXT("IA_Roar"), EInputActionValueType::Boolean);
	EatAction = MakeAction(TEXT("IA_Eat"), EInputActionValueType::Boolean);
	// [2026-11-01 T-068] Buffet/slam verbs, press-edge like Roar.
	WingBuffetAction = MakeAction(TEXT("IA_WingBuffet"), EInputActionValueType::Boolean);
	TailSlamAction = MakeAction(TEXT("IA_TailSlam"), EInputActionValueType::Boolean);

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
	Map(TrueSightAction, EKeys::T);
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
	Map(MoveAction, EKeys::Gamepad_Left2D);
	Map(LookAction, EKeys::Gamepad_Right2D);
	Map(FlapAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(ToggleFlightAction, EKeys::Gamepad_FaceButton_Top);
	Map(DiveAction, EKeys::Gamepad_RightTrigger);
	Map(TrueSightAction, EKeys::Gamepad_FaceButton_Left);
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
	if (LookAction != nullptr)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleLook);
	}
	if (FlapAction != nullptr)
	{
		EIC->BindAction(FlapAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleFlap);
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
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}
	const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
}

void ADragonCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ADragonCharacter::HandleFlap()
{
	if (!Stats->CanFlap())
	{
		return;
	}
	if (!IsFlying())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
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
	const bool bWasFlying = IsFlying();
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
			// [2026-11-01 T-018] Spawned beside the hoard's prize
			// chest (18500,-500): the brain makes for the richest
			// treasure in reach, so the steal lands in seconds,
			// not after a cross-lair trek (first version spawned
			// at the chamber edge and the gnome walked to C3).
			const FVector SpawnPoint(18300.0f, -200.0f, 300.0f);
			AThiefCharacter* Gnome = World->SpawnActorDeferred<AThiefCharacter>(
				AThiefCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, SpawnPoint));
			if (Gnome != nullptr)
			{
				Gnome->ThiefData = BurglarData;
				Gnome->ExitLocation = FVector(-1300.0f, 0.0f, 160.0f);
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
	if (Notoriety == nullptr || Hoard == nullptr || Waves == nullptr || TreasureSub == nullptr ||
		TreasureSub->Registry() == nullptr)
	{
		return;
	}
	Hoard->RecordSessionStart(TreasureSub->Registry()->TotalGold());
	Waves->TickWave(DeltaSeconds);

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
		}
	}
}

void ADragonCharacter::HandleFire(const FInputActionValue& Value)
{
	// [2026-10-28 T-015] Latch only — Tick owns starting/ticking the
	// breath so stamina gating and target gathering happen once.
	bWantsFire = Value.Get<bool>();
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
	// [2026-11-01 T-018] Intro sequence (camera, gnome, choice).
	TickIntro(DeltaSeconds);
	// [2026-11-01 T-016] Game loop: waves, notoriety/hoard
	// accounting and outcome classification, driven from here
	// (the pawn is the one actor that always ticks).
	AuditGameLoop(DeltaSeconds);
	// [2026-10-28 T-014] Trap-slow expiry: restore the captured speeds
	// in the same tick the clock runs out.
	if (TrapSlowTimeRemaining > 0.0f)
	{
		TrapSlowTimeRemaining -= DeltaSeconds;
		if (TrapSlowTimeRemaining <= 0.0f)
		{
			UCharacterMovementComponent* Move = GetCharacterMovement();
			Move->MaxWalkSpeed = UnslowedWalkSpeed;
			Move->MaxFlySpeed = UnslowedFlySpeed;
			UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon trap slow expired; speeds restored"));
		}
	}
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
				SelectAnimState(IsFlying(), bDiving, AnimHS, AnimVelocity.Z);
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
				IsFlying() ? UDragonAnimInstance::ComputeBankDegrees(YawRate, AnimHS) : 0.0f;
			const float TargetPitch =
				IsFlying() ? UDragonAnimInstance::ComputeMeshPitchDegrees(AnimHS, AnimVelocity.Z) : 0.0f;
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
	// pays), hover 8/s (priced out), glide 1/s (the travel mode).
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
					FString::Printf(TEXT("Notoriety %d star (%d pts) | Hoard %lld/%lld | Wave %d state %d"),
									Notoriety->GetStars(), Notoriety->GetPoints(),
									Hoard->GetCurrentHoard(TreasureSub->Registry()->TotalGold()),
									Hoard->GetStartingHoard(), Waves->GetCurrentWave(),
									static_cast<int32>(Waves->GetWaveState())));
			}
		}
	}
	if (GEngine && (IsFlying() || GetCharacterMovement()->MovementMode == MOVE_Falling))
	{
		const FVector ShownVelocity = GetVelocity();
		GEngine->AddOnScreenDebugMessage(
			11011, 0.1f, FColor::Yellow,
			FString::Printf(TEXT("Vel %s | speed %.0f | vSpeed %.0f | %s | stamina %.1f | HP %.1f"),
							*ShownVelocity.ToString(), ShownVelocity.Size(), ShownVelocity.Z, FlightModeName,
							Stats->Stamina, Stats->HP));
	}
}
