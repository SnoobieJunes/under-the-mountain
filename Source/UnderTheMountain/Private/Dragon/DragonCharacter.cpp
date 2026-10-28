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
#include "Dragon/DragonStatsComponent.h"
#include "Scent/ScentComponent.h"
#include "UI/DragonTintWidget.h"
#include "UnderTheMountain.h"

#include "Camera/CameraComponent.h"
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

ADragonCharacter::ADragonCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Stats = CreateDefaultSubobject<UDragonStatsComponent>(TEXT("Stats"));
	Scent = CreateDefaultSubobject<UScentComponent>(TEXT("Scent"));
	Appearance = CreateDefaultSubobject<UDragonAppearanceComponent>(TEXT("Appearance"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);

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
	return MoveAction != nullptr && LookAction != nullptr && FlapAction != nullptr &&
		   ToggleFlightAction != nullptr && DiveAction != nullptr && TrueSightAction != nullptr &&
		   TintAction != nullptr && MappingContext != nullptr;
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
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon pawn spawned at %s"), *GetActorLocation().ToString());
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
	// Gamepad: left stick move, right stick look, bottom face flap,
	// top face toggle flight, right trigger dive.
	Map(MoveAction, EKeys::Gamepad_Left2D);
	Map(LookAction, EKeys::Gamepad_Right2D);
	Map(FlapAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(ToggleFlightAction, EKeys::Gamepad_FaceButton_Top);
	Map(DiveAction, EKeys::Gamepad_RightTrigger);
	Map(TrueSightAction, EKeys::Gamepad_FaceButton_Left);
	Map(TintAction, EKeys::Gamepad_FaceButton_Right);
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
	GetCharacterMovement()->SetMovementMode(IsFlying() ? MOVE_Walking : MOVE_Flying);
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
}

void ADragonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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
	// [2026-10-28 T-032] Walk cycle follows actual movement: play the
	// single-node anim while the pawn has ground/flight speed, stop
	// it (back to the standing bind pose) when it doesn't. Transition
	// -only calls so PlayAnimation never restarts every frame.
	if (WalkAnim != nullptr)
	{
		const bool bMoving = GetVelocity().Size2D() > 20.0f;
		if (bMoving && !bPlayingWalk)
		{
			GetMesh()->PlayAnimation(WalkAnim, true);
			bPlayingWalk = true;
		}
		else if (!bMoving && bPlayingWalk)
		{
			GetMesh()->Stop();
			bPlayingWalk = false;
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
