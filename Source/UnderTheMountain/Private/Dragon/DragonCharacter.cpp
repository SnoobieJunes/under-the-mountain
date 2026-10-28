// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Dragon walking-skeleton movement: ACharacter gives us
// walking + flying movement modes; flap/dive are velocity work on top, so
// T-011 feel tuning changes numbers, not architecture.

#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonStatsComponent.h"
#include "UnderTheMountain.h"

#include "Camera/CameraComponent.h"
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

void ADragonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsFlying())
	{
		Stats->DrainForFlight(DeltaSeconds);
		if (bDiving)
		{
			const FVector DiveDir = GetControlRotation().Vector();
			GetCharacterMovement()->Velocity += DiveDir * DiveAcceleration * DeltaSeconds;
		}
		if (Stats->Stamina <= 0.0f)
		{
			// [2026-10-27 T-003] Exhausted dragon glides down, not falls out of
			// control: force walking mode so landing logic takes over.
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	}
	else if (GetCharacterMovement()->MovementMode == MOVE_Walking)
	{
		Stats->RegenOnGround(DeltaSeconds);
	}
}
