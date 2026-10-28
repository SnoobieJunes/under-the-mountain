// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Dragon walking-skeleton movement: ACharacter gives us
// walking + flying movement modes; flap/dive are velocity work on top, so
// T-011 feel tuning changes numbers, not architecture.
// [2026-10-28 T-011] The feel tuning that line promised: drain rate is
// chosen per tick from what the dragon is actually doing (dive/glide/
// hover), exhaustion burns HP, Landed() pays for hard impacts, and a
// debug readout makes velocity/drain visible while tuning in PIE.

#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonStatsComponent.h"
#include "UnderTheMountain.h"

#include "Camera/CameraComponent.h"
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

ADragonCharacter::ADragonCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Stats = CreateDefaultSubobject<UDragonStatsComponent>(TEXT("Stats"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);

	// [2026-10-27 T-003] Placeholder body: capsule mesh hidden in favour of
	// the capsule itself scaled long — reads as a creature silhouette in
	// greybox without pretending to be the T-010 dragon.
	GetMesh()->SetVisibility(false);

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderBody->SetRelativeScale3D(FVector(1.0f, 2.2f, 1.0f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		PlaceholderBody->SetStaticMesh(SphereMesh.Object);
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxFlySpeed = 1800.0f;
	Move->MaxWalkSpeed = 600.0f;
	Move->BrakingDecelerationFlying = 300.0f;
	bUseControllerRotationYaw = false;
	Move->bOrientRotationToMovement = true;
}

bool ADragonCharacter::IsFlying() const
{
	return GetCharacterMovement()->MovementMode == MOVE_Flying;
}

void ADragonCharacter::BeginPlay()
{
	Super::BeginPlay();
	BuildInputActions();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->AddMappingContext(MappingContext, 0);
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
	// Gamepad: left stick move, right stick look, bottom face flap,
	// top face toggle flight, right trigger dive.
	Map(MoveAction, EKeys::Gamepad_Left2D);
	Map(LookAction, EKeys::Gamepad_Right2D);
	Map(FlapAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(ToggleFlightAction, EKeys::Gamepad_FaceButton_Top);
	Map(DiveAction, EKeys::Gamepad_RightTrigger);
}

void ADragonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleMove);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleLook);
	EIC->BindAction(FlapAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleFlap);
	EIC->BindAction(ToggleFlightAction, ETriggerEvent::Started, this, &ADragonCharacter::HandleToggleFlight);
	EIC->BindAction(DiveAction, ETriggerEvent::Triggered, this, &ADragonCharacter::HandleDive);
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
		if (Stats->Stamina <= 0.0f)
		{
			// [2026-10-28 T-011] GAMEPLAY.md section 2 zero-stamina rule:
			// HP burns at 2/s while a *draining* action continues — a
			// free dive (rate 0) deliberately skips the burn. Checked
			// after the stamina drain so the tick that empties the pool
			// is also the first tick that pays in HP.
			if (FlightDrainRate > 0.0f)
			{
				Stats->DrainHPForExhaustion(DeltaSeconds);
			}
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
