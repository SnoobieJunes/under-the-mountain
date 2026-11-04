// Copyright Under the Mountain contributors.
// Purpose: the playable dragon (walking skeleton, T-003). Created 2026-10-27.
// [2026-10-28 T-041] The visible body is SK_Dragon_v6 on the inherited
// skeletal mesh (visible since T-032, set up in the constructor);
// [2026-11-03 T-110] v6 replaced v4 (same 1.9 m scale, same offsets);
// movement/collision stay on the capsule and movement component, so the
// mesh is display only. There is no placeholder body any more.
// [2026-10-28 T-011] Flight feel pass: per-mode stamina drain
// (dive/glide/hover), exhaustion HP burn, hard-landing damage on Landed,
// and an on-screen flight debug readout (all tuning in GAMEPLAY.md s2).
// Invariants: movement mode is the single source of "flying" truth.
// [2026-10-28 T-041] In production code paths the pawn mutates stamina
// and HP only through UDragonStatsComponent methods; the pools
// themselves are public UPROPERTYs (see that header) — not encapsulated.

#pragma once

#include "CoreMinimal.h"
#include "Dragon/DragonAnimInstance.h" // [2026-11-01 T-068] EDragonAnimState owner
#include "Notoriety/WitnessRules.h"	   // [2026-11-04 T-134] witness fact types
#include "GameFramework/Character.h"
#include "UI/OnboardingDefs.h" // [2026-11-03 T-073] EOnboardingVerb for the prompt flags
#include "DragonCharacter.generated.h"

class UDragonStatsComponent;
class UDragonCombatComponent;
class UDragonTargetingComponent;
class UDragonCarryComponent;
class UDragonMovementComponent;
class UThiefBrain;
class UScentComponent;
class UDragonAppearanceComponent;
class UDragonTintWidget;
class AGoatActor;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
class UCameraComponent;
class UAnimSequence;

// [2026-11-01 T-068] EDragonAnimState is owned by DragonAnimInstance.h
// [2026-11-01 T-068] (included above, before generated.h per UHT rules).

// [2026-11-03 T-065] Camera set (T-065): near chase above the wings
// (default), far scenic chase, first-person at the eyes. The pawn
// cycles the set and persists the choice; T-088's Settings screen
// surfaces the same value later. Global scope: UENUM cannot live inside
// the pawn class.
// [2026-11-04 T-111] Rework (T-111, owner spec): Eyes -> Near -> Far order
// [2026-11-04 T-111] opening on Near, tuned to the T-113 v6 rig. Indices are
// [2026-11-04 T-111] stable on purpose (Near 0 / Far 1 / Eyes 2): a T-065 save
// [2026-11-04 T-111] slot migrates to its closest match (close chase -> Near,
// [2026-11-04 T-111] wide -> Far, first-person -> Eyes) instead of stranding
// [2026-11-04 T-111] the player in a hidden-mesh first-person view.
UENUM(BlueprintType)
enum class EDragonCameraMode : uint8
{
	Near UMETA(DisplayName = "Near chase above the wings (default)"),
	Far UMETA(DisplayName = "Far scenic chase"),
	Eyes UMETA(DisplayName = "First-person at the dragon's eyes"),
};

UCLASS()
class UNDERTHEMOUNTAIN_API ADragonCharacter : public ACharacter
{
	GENERATED_BODY()

  public:
	ADragonCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonStatsComponent> Stats;

	// [2026-10-28 T-013] The nose: owns scent ribbons + true-sight
	// markers driven by the treasure registry. The pawn only forwards
	// the toggle input; all scent rules live in the component.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UScentComponent> Scent;

	// [2026-10-28 T-027] Scale tint owner: applies the player's RGB
	// choice to the mesh material and persists it across sessions.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonAppearanceComponent> Appearance;

	// [2026-10-28 T-015] Combat verbs (fire breath, roar, eat channel).
	// The component owns every combat rule and never ticks itself; this
	// pawn gathers targets in-world and calls its TickFire/TickEat.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonCombatComponent> Combat;

	// [2026-11-04 T-069] Smart lock-on: cadenced candidate gather +
	// the shared current target pickup/combat read (see its header).
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonTargetingComponent> Targeting;

	// [2026-11-04 T-070] Pickup/carry/drop/deposit over the treasure
	// registry (one item at a time; see its header).
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonCarryComponent> Carry;

	// [2026-11-04 T-069] The shared lock, readable without reaching
	// into the component: NAME_None / None category = no lock.
	FName GetLockedTargetId() const;
	bool GetLockedTargetCategory(int32& OutCategory) const;

	// [2026-11-04 T-102] Right-face multiplex, pure (Option A as
	// completed by T-076 in this batch — see BuildInputActions):
	// a press shorter than the hold threshold is a tap, anything
	// longer is a hold; a tap claws when a claw target is in the
	// arc and otherwise toggles tint, a hold is Eat/context on the
	// ground and sink in the air. Static so DragonInputTest pins
	// the timing/context table without a pawn.
	static bool IsRightFaceTap(float PressSeconds);
	static bool RightFaceTapIsClaw(bool bClawTargetInArc);
	// [2026-11-04 T-071] Audio seam: verbs call this, never PlaySound.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<class UAudioDirectorComponent> AudioDirector;

	// [2026-11-01 T-051] Fire-breath presentation: a pooled Niagara
	// stream (NS_FireBreath, authored by Scripts/create_combat_assets.py
	// via the editor - see that file) plus a flickering point light,
	// both driven from Combat->IsFiring() in Tick. Display only — the
	// cone test that damages thieves is UDragonCombatComponent::
	// IsInFireCone math. Replaces the T-015 orange cone mesh, which
	// read as geometry, not fire. The Niagara component is created
	// once and Activate/Deactivate-cycled (pooling: no per-shot
	// spawns); the light's intensity is the flicker, its visibility
	// follows the breath.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<class UNiagaraComponent> FireBreathFX;

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<class UPointLightComponent> FireLight;

	// [2026-11-01 T-052] Rain presentation for the weather system:
	// a pooled Niagara shower (NS_Rain) carried above the pawn so
	// rain follows the dragon instead of needing a world-sized
	// emitter. Activated only in the Rain weather state.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<class UNiagaraComponent> RainFX;

	// [2026-11-01 T-052] Weather switching from the PIE console:
	// `Weather` cycles Clear -> Rain, `SetWeather 0|1` jumps.
	// Both land in ApplyWeather; the utm.weather.state cvar is the
	// same state read by Tick (that is how -ExecCmds and headless
	// captures drive it).
	UFUNCTION(Exec)
	void Weather();

	UFUNCTION(Exec)
	void SetWeather(int32 State);

	// [2026-11-01 T-055] Dev/capture switch: ignite every burnable
	// prop within 30 m. Exists so the burn -> rubble -> scorch
	// chain can be proven from -ExecCmds and captures without
	// playing a full breath onto a crate first; it calls the same
	// IgniteNow the exposure threshold calls.
	UFUNCTION(Exec)
	void BurnTest();

	// [2026-11-01 T-018] Intro choice entry from the choice
	// widget: 0 = Let go, 1 = Chase. Forwards to the intro
	// subsystem; the widget is dismissed by the FreePlay
	// transition in Tick.
	void HandleIntroChoice(int32 BranchIndex);

	// [2026-11-01 T-052] Apply a weather state (0 Clear, 1 Rain) to
	// the world's sky actors (sun, sky light, fog - found by class
	// in the current world) and this pawn's rain FX, and log it.
	// Interior point lights are never touched; the lair's boxes
	// shadow the sun, so the interior reads unchanged.
	void ApplyWeather(int32 NewState);

	// [2026-11-01 T-052] Current weather state (0 Clear, 1 Rain);
	// -1 until the first ApplyWeather (BeginPlay) so the first Tick
	// always applies the cvar's initial value exactly once.
	int32 CurrentWeatherState = -1;

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UCameraComponent> Camera;

	// [2026-10-27 T-003] Flight tuning for the skeleton; T-011 owns feel.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float FlapUpwardSpeed = 650.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float DiveAcceleration = 2400.0f;

	bool IsFlying() const;

	// [2026-11-04 T-125] Read-only dive latch for the AnimInstance: ABP
	// transition tests need the same dive fact the single-node driver
	// passed to SelectAnimState (no setter — input owns the latch).
	bool IsDiving() const
	{
		return bDiving;
	}

	// [2026-11-03 T-075] Prototype flight (T-075): true while the pawn flies the
	// custom movement chassis (MOVE_Custom + non-zero utm.flight.scheme).
	bool IsPrototypeFlying() const;

	// [2026-11-03 T-075] Nose direction the prototype integrator steers onto
	// (free-look freezes it, steer-latch turns it at the yaw limit).
	FRotator GetProtoNose() const;

	// [2026-11-03 T-075] Per-tick prototype housekeeping: enter/exit the custom
	// mode, feed brake state, capture CSV rows. No-op when scheme is shipped (0).
	void UpdatePrototypeFlight(float DeltaSeconds);

	// [2026-11-03 T-075] Dev/capture: dump captured prototype CSV rows to the log.
	UFUNCTION(Exec)
	void FlightCsv();

	// [2026-10-28 T-014] Dwarf trap effect (GAMEPLAY.md section 5.2):
	// multiplies walk/fly speed by Factor for Seconds, then restores.
	// Implemented as a Tick countdown (not a timer handle) so pausing
	// the world pauses the trap; re-triggering refreshes, never stacks.
	void ApplyTrapSlow(float Factor, float Seconds);
	bool IsTrapSlowed() const
	{
		return TrapSlowTimeRemaining > 0.0f;
	}

	// [2026-11-03 T-103] Trap-slow clock + expiry, extracted from Tick so the
	// slow/restore contract is provable headlessly (Tick itself needs a
	// world). Counts down and restores the captured speeds and the hybrid
	// thrust factor exactly when the clock runs out.
	void TickTrapSlow(float DeltaSeconds);

	// [2026-11-03 T-064] Flight-state classification shared by the HUD feed
	// and the debug readout: dive is the shared posture rule, brake reads
	// the brake input, hover is the precision band. Grounded (or legacy
	// ground modes) leave all three false. Const: reads movement + input
	// state only, never writes pools.
	void FlightStateFlags(bool& bOutDiving, bool& bOutBraking, bool& bOutHovering) const;

	// [2026-11-03 T-065] Camera set: steps Near → Far → Eyes
	// [2026-11-04 T-111] (Eyes -> Near -> Far cyclic order, entered at the
	// [2026-11-04 T-111] Near default) and persists the choice (SaveGame slot
	// [2026-11-04 T-111] below). World-free apart from the slot file itself,
	// [2026-11-04 T-111] so the order + persistence round-trip is
	// [2026-11-04 T-111] headless-testable.
	void CycleCamera();
	// [2026-11-04 T-111] Pure rig decisions per mode (arm, mesh visibility,
	// [2026-11-04 T-111] boom collision) — the headless-testable half of
	// [2026-11-04 T-111] ApplyCameraRig. Pivots need live bones and ride the
	// [2026-11-04 T-111] -game capture set instead.
	static void CameraRigForMode(EDragonCameraMode Mode, float& OutArmLength, bool& bOutHideMesh,
								 bool& bOutCollide);
	EDragonCameraMode GetCameraMode() const;
	void SetCameraSaveSlotForTesting(const FString& SlotName);
	void LoadCameraChoice();
	void SaveCameraChoice() const;
	// [2026-11-04 T-111] Test hook: one rig tick headlessly. Arm/visibility/
	// [2026-11-04 T-111] collision converge without live bones; pivots ride
	// [2026-11-04 T-111] the -game capture set instead.
	void ApplyCameraRigForTesting(float DeltaSeconds)
	{
		ApplyCameraRig(DeltaSeconds);
	}

	// [2026-11-03 T-065] Pitch-invert choices (Settings values until T-088's
	// screen surfaces them; all default off = stick/mouse up is nose-up):
	// mouse flight pitch, gamepad flight pitch (left stick), gamepad camera
	// pitch (right stick). Static so tests pin the defaults.
	static bool InvertMousePitch();
	static bool InvertPadFlightPitch();
	static bool InvertPadCameraPitch();

	// [2026-10-28 T-033] Input contract: true when every
	// code-created action and the mapping context exist.
	// EnsureInputActions() guarantees this before SetupPlayerInput
	// Component binds anything (bindings store the action pointer,
	// so a null bound early can never fire).
	bool HasInputActions() const;
	// [2026-10-28 T-033] Build the actions if they don't exist yet.
	// Lazy because UObjects cannot be created inside the class
	// constructor (CDO creation asserts); called at the top of
	// SetupPlayerInputComponent and BeginPlay.
	void EnsureInputActions();

	// [2026-10-28 T-032] Walk cycle (T-029) played single-node while
	// the pawn actually moves; nullptr if the asset failed to load,
	// in which case the mesh holds its standing bind pose.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> WalkAnim;

	// [2026-10-28 T-043] Flight cycle played single-node while flying;
	// loaded in the constructor from SK_Dragon_v4_FlyAnim. A separate
	// import step creates that asset; if it is missing FlyAnim stays
	// null, BeginPlay logs one warning, and flight falls back to the
	// bind pose in Tick (errors surfaced, never silent).
	// [2026-11-04 T-113] Content is the newest owner-drop rig now, not v4:
	// [2026-11-04 T-113] T-113 re-imported all nine clip paths on the v6
	// [2026-11-04 T-113] skeleton (names stay v4-legacy so no path changes
	// [2026-11-04 T-113] were needed); the gate below passes them through.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> FlyAnim;

	// [2026-11-01 T-068] T-068 clip set (Scripts/create_dragon_anims.py +
	// [2026-11-01 T-068] import_dragon_anims.py). All null-safe: a missing
	// [2026-11-01 T-068] clip falls back to FlyAnim or bind pose in Tick, so
	// [2026-11-01 T-068] an un-imported checkout still flies like T-043 did.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> GlideAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> HoverAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> TakeoffAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> LandAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> RoarAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> HitAnim;

	// [2026-11-04 T-126] Remaining v1 clip families (T-068 remainder). All
	// [2026-11-04 T-126] null-safe like the T-068 set above: a family whose
	// [2026-11-04 T-126] Blender clip has not been authored/imported yet stays
	// [2026-11-04 T-126] null, PlayClipFamily no-ops, and the verb still works.
	// [2026-11-04 T-126] Triggers: HitHeavy rides Landed (heavy damage only);
	// [2026-11-04 T-126] BiteEat rides a started eat channel; TailWhip rides a
	// [2026-11-04 T-126] landed tail slam; Defeat/Sleep/Wake/LieDown/FirePose/
	// [2026-11-04 T-126] Claw have routing entries only until their gameplay
	// [2026-11-04 T-126] triggers land (T-074/T-084/T-076 respectively).
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> HitHeavyAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> DefeatAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> SleepIdleAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> WakeAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> LieDownAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> FirePoseAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> ClawAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> BiteEatAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> TailWhipAnim;
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> WingBuffetAnim;

	// [2026-11-04 T-126] Clip-family routing entry: returns the loaded clip
	// [2026-11-04 T-126] for a family (null when not yet imported) and plays
	// [2026-11-04 T-126] it through PlayOneShot (ABP slot or single-node).
	UAnimSequence* ClipForFamily(EDragonClipFamily Family) const;
	void PlayClipFamily(EDragonClipFamily Family);

	// [2026-11-01 T-068] Play a one-shot clip (takeoff/land/roar/hit) over
	// [2026-11-01 T-068] locomotion for its length, then locomotion resumes.
	// [2026-11-01 T-068] This is the single-node stand-in for montage slots;
	// [2026-11-01 T-068] ABP_Dragon montages replace it without call-site churn.
	// [2026-11-04 T-125] Dual-mode now (T-125 agent lane): in ABP mode the
	// call-site-identical PlayOneShot plays through the graph's slots as
	// a dynamic montage; without ABP_Dragon on disk the shipped
	// single-node play runs. bUsingABP is set once in the constructor.
	void PlayOneShot(UAnimSequence* Clip);

	// [2026-11-04 T-125] ABP montage slot names (must match the Slot nodes
	// in ABP_Dragon — see docs/ABP-DRAGON-BUILDSHEET.md).
	static inline const FName AnimSlotFullBody = FName(TEXT("FullBody"));
	static inline const FName AnimSlotUpperBody = FName(TEXT("UpperBody"));

	bool IsUsingABP() const
	{
		return bUsingABP;
	}

	// [2026-11-01 T-068] Wing buffet (X) and tail slam (Z): rules live in
	// [2026-11-01 T-068] UDragonCombatComponent; the pawn gathers targets.
	void HandleWingBuffet();
	void HandleTailSlam();
	// [2026-11-04 T-076] Claw (Q / right-face tap with a target in arc).
	void HandleClaw();
	// [2026-11-04 T-070] Pickup-or-drop toggle (P / right-face hold
	// context): carrying drops (deposits inside the lair), empty
	// claws pick up the locked treasure or the nearest in range.
	void HandlePickupOrDrop();
	// [2026-11-04 T-102] Pad trigger verbs: RT Surge (press edge,
	// shares W-tap's gate), LT Brake (held latch into bBraking).
	void HandleSurge();
	void HandleBrake(const struct FInputActionValue& Value);
	void HandleBrakeEnd();
	// [2026-11-04 T-102] Right-face multiplex handlers (see the
	// pure IsRightFaceTap/RightFaceTapIsClaw decisions above).
	void HandleRightFaceStarted();
	void HandleRightFaceEnd();
	// [2026-11-04 T-069] L3/Tab: cycle (or acquire) the smart lock.
	void HandleSmartLock();

	// [2026-11-01 T-068] Pure animation-state selection, kept as the pawn's
	// [2026-11-01 T-068] public seam; delegates to
	// [2026-11-01 T-068] UDragonAnimInstance::SelectLocomotionState (see that
	// [2026-11-01 T-068] header for the full rule incl. Hover/Dive).
	static EDragonAnimState SelectAnimState(bool bFlying, bool bDiving, float HorizontalSpeed,
											float VerticalSpeed);

  protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// [2026-10-28 T-011] Hard-landing damage: ACharacter::Landed fires
	// after the movement component has already zeroed vertical velocity,
	// so the impact speed comes from LastVelocityZ (stored every Tick),
	// never from a live read in here.
	virtual void Landed(const FHitResult& Hit) override;

	// [2026-11-01 T-077] Run-over crush: physical contact while
	// grounded at charge speed routes into ApplyCrushVictim for the
	// contacted actor only. Airborne contact is Landed's job.
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
						   bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse,
						   const FHitResult& Hit) override;

  private:
	// [2026-10-27 T-003] Enhanced Input actions are instantiated in code for
	// the skeleton: one C++ source of truth, no hand-authored .uassets to
	// drift. Migration to data assets is a later decision, not an accident.
	// [2026-10-28 T-035] Every UObject member below is UPROPERTY-tracked:
	// without reflection the GC cannot see these TObjectPtrs from the
	// pawn, so the actions/widget survived only via external Enhanced
	// Input/widget-tree references (T-004 sweep finding, high).
	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY()
	TObjectPtr<UInputAction> FlapAction;
	UPROPERTY()
	TObjectPtr<UInputAction> ToggleFlightAction;
	UPROPERTY()
	TObjectPtr<UInputAction> DiveAction;
	// [2026-11-03 T-075] Prototype-only actions (T-075/T-063): RMB free-look,
	// Ctrl sink, D-pad Up steer latch. Shipped leaves them unbound-equivalent
	// (handlers gate on the scheme), so shipped behavior is unchanged.
	UPROPERTY()
	TObjectPtr<UInputAction> FreeLookAction;
	UPROPERTY()
	TObjectPtr<UInputAction> SinkAction;
	UPROPERTY()
	TObjectPtr<UInputAction> SteerLatchAction;
	UPROPERTY()
	TObjectPtr<UInputAction> TrueSightAction;
	// [2026-11-03 T-061] Scent-layer show/hide (N / D-pad Down) — the
	// ribbons only; true-sight markers answer to T alone.
	UPROPERTY()
	TObjectPtr<UInputAction> ScentToggleAction;
	UPROPERTY()
	TObjectPtr<UInputAction> TintAction;
	// [2026-10-28 T-015] Combat actions, code-created like the rest:
	// [2026-11-03 T-065] Split look/move: the mouse and the right stick
	// need separate pitch-invert choices, and the left stick needs its
	// own flight-pitch feed — one shared action cannot tag the device.
	UPROPERTY()
	TObjectPtr<UInputAction> PadLookAction;
	UPROPERTY()
	TObjectPtr<UInputAction> PadMoveAction;
	// [2026-11-03 T-065] Camera cycle (V / gamepad R3): steps the adopted
	// set and persists the choice.
	UPROPERTY()
	TObjectPtr<UInputAction> CameraCycleAction;
	// [2026-11-03 T-072] Flow pause (Esc / gamepad menu): routes
	// through the game mode's machine, never pauses directly.
	UPROPERTY()
	TObjectPtr<UInputAction> PauseAction;
	// [2026-11-03 T-073] Help overlay (H / gamepad Back): lists every
	// shipped binding from the single onboarding table.
	UPROPERTY()
	TObjectPtr<UInputAction> HelpAction;
	// Fire is held (Triggered/Completed), Roar and Eat are presses.
	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;
	UPROPERTY()
	TObjectPtr<UInputAction> RoarAction;
	UPROPERTY()
	TObjectPtr<UInputAction> EatAction;
	// [2026-11-01 T-068] Wing buffet (X / DPad Left) and tail slam (Z / DPad Right).
	UPROPERTY()
	TObjectPtr<UInputAction> WingBuffetAction;
	UPROPERTY()
	TObjectPtr<UInputAction> TailSlamAction;
	// [2026-11-04 T-076] Claw (Q; gamepad reaches it through the
	// right-face multiplex below, never a second pad binding).
	UPROPERTY()
	TObjectPtr<UInputAction> ClawAction;
	// [2026-11-04 T-070] Pickup/drop (P; gamepad reaches it through
	// the right-face hold multiplex — context interact — below).
	UPROPERTY()
	TObjectPtr<UInputAction> PickupAction;
	// [2026-11-04 T-102] Adopted pad triggers: RT Surge / LT Brake.
	// Brake replaces DiveAction's pad/shift role (dive is a posture
	// since T-064); DiveAction itself is kept unbound for compat.
	UPROPERTY()
	TObjectPtr<UInputAction> SurgeAction;
	UPROPERTY()
	TObjectPtr<UInputAction> BrakeAction;
	// [2026-11-04 T-102] The ONE action bound to gamepad right face:
	// tap/hold/context are discriminated in the pawn (see
	// HandleRightFace*), so no verb is ever double-bound to it.
	UPROPERTY()
	TObjectPtr<UInputAction> RightFaceAction;
	// [2026-11-04 T-069] Smart-lock cycle (Tab / L3, the FLIGHT-SPIKE
	// reservation): acquires or steps the targeting lock.
	UPROPERTY()
	TObjectPtr<UInputAction> SmartLockAction;
	UPROPERTY()
	TObjectPtr<UDragonTintWidget> TintWidget;
	bool bTintPickerOpen = false;
	// [2026-11-03 T-073] Onboarding: first-use prompts + help overlay.
	// The widget is hit-test invisible / non-modal (teaches, never
	// blocks); VerbDone persists per verb so prompts never repeat.
	UPROPERTY()
	TObjectPtr<class UOnboardingWidget> OnboardingWidget;
	// [2026-11-03 T-073] Save slot for onboarding progress
	// ("OnboardingProgressV1", fresh slot per format like the
	// camera/tint saves). Plain data member: tests never write it.
	FString OnboardingSaveSlotName = TEXT("OnboardingProgressV1");
	bool OnboardingDone[static_cast<int32>(EOnboardingVerb::Count)] = {};
	EOnboardingVerb CurrentPrompt = EOnboardingVerb::Count;
	float OnboardingTimer = 0.0f;

	// [2026-11-01 T-018] The intro choice widget, created when the
	// gnome steals and dismissed at FreePlay.
	UPROPERTY()
	TObjectPtr<class UIntroChoiceWidget> IntroWidget;

	// [2026-11-01 T-019] The HUD, created lazily on the first Tick
	// with a local controller (possession lands after BeginPlay).
	UPROPERTY()
	TObjectPtr<class UDragonHUDWidget> HUDWidget;
	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext;

	void BuildInputActions();
	void HandleMove(const struct FInputActionValue& Value);
	void HandlePadMove(const struct FInputActionValue& Value);
	void HandleLook(const struct FInputActionValue& Value);
	void HandlePadLook(const struct FInputActionValue& Value);
	void HandleCameraCycle();
	void HandleFlap();
	void HandleToggleFlight();
	void HandleDive(const struct FInputActionValue& Value);
	// [2026-10-28 T-030] Dive release: Triggered alone never fires on key-up,
	// so bDiving latched forever; Completed/Canceled clear it.
	void HandleDiveEnd();

	// [2026-11-03 T-075] Prototype inputs (T-075/T-063): W-tap Surge, Space-hold
	// rise, RMB free-look + double-click steer latch, D-pad Up steer latch,
	// Ctrl sink. Shipped since T-062 (Scheme C default); legacy scheme 0
	// keeps the old map.
	void HandleMoveStarted(const struct FInputActionValue& Value);
	// [2026-11-03 T-062] Release edge for the remembered move axis (S-flare /
	// A/D-turn below): a released key must stop flaring/turning.
	void HandleMoveEnd();
	// [2026-11-03 T-065] Device-tagged move core: keyboard S flares while
	// pad stick-back pitches instead (adopted maps diverge by device).
	void HandleMoveAxis(const FVector2D& Axis, bool bFromGamepad);
	void HandleRiseHeld(const struct FInputActionValue& Value);
	void HandleRiseEnd();
	// [2026-11-03 T-072] Pause routing by flow state: Playing pauses,
	// Paused resumes, Won/Lost back out to title, Title ignores.
	void HandlePause();
	// [2026-11-03 T-073] Help overlay toggle (H / gamepad Back).
	void HandleHelp();
	// [2026-11-03 T-073] Marks a verb performed (prompt dismissed
	// forever, progress saved). Attempt-based like the teaching
	// itself: pressing the key with the verb available counts.
	void MarkOnboardingDone(EOnboardingVerb Verb);
	// [2026-11-03 T-073] 1 Hz prompt evaluation while a prompt can
	// still show: run started, intro done, onboarding incomplete.
	void TickOnboarding(float DeltaSeconds);
	void LoadOnboardingProgress();
	void HandleFreeLook(const struct FInputActionValue& Value);
	void HandleFreeLookEnd();
	void HandleSteerLatch();
	void HandleSink(const struct FInputActionValue& Value);
	void HandleSinkEnd();

	// [2026-11-03 T-075] Prototype nose + latch state (see UpdatePrototypeFlight:
	// nose follows control rotation except while free-looking; the latch turns it
	// toward the view at the airspeed-limited yaw rate).
	FRotator ProtoNose = FRotator::ZeroRotator;
	bool bFreeLook = false;
	bool bSteerLatch = false;
	bool bRiseHeld = false;
	bool bSinkHeld = false;
	float LastRMBReleaseTime = -10.0f;
	float LastSurgeTime = -10.0f;
	// [2026-11-03 T-062] Last move-axis sample (see HandleMove): S-held
	// (Y < -0.5) flares in the momentum band, A/D (X) yaws the nose there.
	FVector2D LastMoveAxis = FVector2D::ZeroVector;
	// [2026-11-03 T-065] Device tag for the sample above: pad stick-back
	// is NOT an S-flare (it pitches the nose instead), and pad stick
	// deflection pitches in the momentum band. Set by every move sample.
	bool bLastMoveFromGamepad = false;

	// [2026-11-03 T-065] Camera rig state: active set member (persisted),
	// pivots recomputed every tick from mesh bones (null-safe: a missing
	// bone keeps the last pivot). Eyes hides the pawn mesh (head interior
	// would fill the frame); fire/rain FX are capsule-attached and stay
	// visible. Boom collision is on in the chase modes, off in Eyes.
	// [2026-11-04 T-111] Default is Near (owner spec).
	EDragonCameraMode CameraMode = EDragonCameraMode::Near;
	FString CameraSaveSlotName = TEXT("DragonCameraChoice");
	bool bMeshHiddenForCamera = false;
	// [2026-11-04 T-111] One-shot latch for the utm.camera.mode capture
	// [2026-11-04 T-111] forcing (see ApplyCameraRig).
	bool bCameraModeForceApplied = false;
	// [2026-11-04 T-020] One-shot latch for the utm.proof.spawn / utm.proof.yaw
	// [2026-11-04 T-020] capture forcing (see ApplyCameraRig).
	bool bProofSpawnApplied = false;

	// [2026-11-03 T-075] Scripted course runner (utm.flight.course): the program
	// owns nose/brake/assist while bCourseFlying; UpdatePrototypeFlight skips its
	// feeds but keeps drain + CSV. Off by default; shipped never sees it.
	bool bCourseFlying = false;
	int32 CoursePhase = -1;
	float CoursePhaseTime = 0.0f;
	float CourseTotalTime = 0.0f;
	float CourseTurnAccum = 0.0f;
	float CourseBaseYaw = 0.0f;
	FVector CourseMark = FVector::ZeroVector;
	FVector CourseMinXY = FVector::ZeroVector;
	FVector CourseMaxXY = FVector::ZeroVector;
	float CourseTouchSpeed = 0.0f;
	void TickFlightCourse(float DeltaSeconds);
	void QuitCourseGame();
	void RestoreCoursePool(const TCHAR* LegName);
	void CourseLog(const TCHAR* Fmt, ...);

	// [2026-11-03 T-065] Camera rig per-tick pivot/arm/visibility (persistence
	// lives in the public Load/SaveCameraChoice above so tests drive it).
	void ApplyCameraRig(float DeltaSeconds);
	// [2026-10-28 T-013] Seeing Stone toggle (key T / gamepad left face).
	void HandleTrueSight();
	// [2026-11-03 T-061] Scent-ribbon show/hide (key N / gamepad D-pad Down).
	void HandleToggleScent();
	// [2026-10-28 T-027] Scale-colour picker toggle (key C / gamepad right face).
	void HandleToggleTintPicker();
	// [2026-10-28 T-015] Fire breath hold (LMB / gamepad left trigger):
	// Triggered latches bWantsFire from the value; Completed/Canceled
	// clear it and stop the breath (Triggered alone never fires on
	// release — the T-030 dive lesson).
	void HandleFire(const struct FInputActionValue& Value);
	void HandleFireEnd();
	// [2026-10-28 T-015] Roar press (R / gamepad left shoulder).
	void HandleRoar();
	// [2026-10-28 T-015] Eat press (E / gamepad right shoulder): picks
	// the nearest eatable in Combat->EatRange and starts the channel.
	void HandleEat();
	// [2026-10-28 T-015] The in-world half of the combat seam: every
	// thief actor's brain + location as the parallel arrays
	// UDragonCombatComponent consumes. Gathered only when a verb
	// needs targets, never as idle per-frame bookkeeping.
	void GatherCombatTargets(TArray<UThiefBrain*>& OutBrains, TArray<FVector>& OutLocations);

	// [2026-11-04 T-134] Witness facts for one act location: villager
	// positions + a real LOS trace each, and settlement areas derived
	// from villager homes (a villager's home cluster IS the settlement
	// footprint for charging purposes). Fed to FWitnessRules through
	// the Notoriety witnessed verbs; called only on kill/eat events.
	void GatherWitnessFacts(const FVector& ActLocation, TArray<FSettlementArea>& OutSettlements,
							TArray<FVillagerWitness>& OutVillagers);

	// [2026-11-01 T-078] The goat half of the fire seam: every goat
	// actor + location as parallel arrays, gathered only while fire
	// is wanted (TickFire burns goats through the same cone).
	void GatherGoatTargets(TArray<AGoatActor*>& OutGoats, TArray<FVector>& OutLocations);

	// [2026-11-01 T-055] The burnable-prop half of the fire seam,
	// plus the two things burning props cause: area damage around
	// lit props, and ground scorch while the breath is held on the
	// ground. Scorch dedups by proximity and caps per session so a
	// long firefight cannot spawn discs without bound.
	void GatherBurnableTargets(TArray<class ABurnablePropActor*>& OutProps,
							   TArray<FVector>& OutLocations);
	void ApplyBurningAreaDamage(float DeltaSeconds);
	void MaybeSpawnGroundScorch(const FVector& Origin, const FVector& Forward);

	// [2026-11-01 T-016] The game-loop audit (0.5 s cadence from
	// Tick): session start, escape/death/ignition accounting into
	// Notoriety + Hoard, wave driving, and outcome classification.
	void AuditGameLoop(float DeltaSeconds);

	// [2026-11-01 T-018] Intro driver from Tick: subsystem state
	// machine plus the world work (camera ease, gnome spawn and
	// watch, choice widget show/dismiss).
	void TickIntro(float DeltaSeconds);

	// [2026-11-01 T-055] Scorch bookkeeping: disc locations this
	// session (dedup radius checked against these) and the
	// accumulator/throttle for ground hits while firing downward.
	TArray<FVector> ScorchLocations;

	// [2026-11-01 T-018] Intro glue state: the spawned gnome, and
	// the one-shot latches for spawning it and showing/dismissing
	// the choice widget exactly once per transition.
	TWeakObjectPtr<class AThiefCharacter> IntroGnome;
	bool bIntroGnomeSpawned = false;
	bool bIntroChoiceShown = false;

	// [2026-11-01 T-016] Game-loop audit bookkeeping: thieves and
	// props already accounted for (by actor name - unique per
	// spawn), so an escape/death/ignition is counted exactly once.
	TSet<FName> CountedThiefEvents;
	TSet<FName> CountedPropBurns;
	float LoopAuditTimer = 0.0f;
	bool bOutcomeLogged = false;
	// [2026-11-03 T-086] Hoard-mound visual: cached actor + base scale
	// (captured on first sight — box() bakes size into actor scale),
	// last applied step (-1 forces the first apply + log).
	UPROPERTY()
	TObjectPtr<class AStaticMeshActor> HoardMoundActor;
	FVector HoardMoundBaseScale = FVector::OneVector;
	int32 HoardMoundStep = -1;
	// [2026-11-03 T-083] Wave-gold XP bookkeeping: the wave whose
	// hoard window is open, and the hoard total when it opened. When
	// the wave number advances, the finished wave's net gold is
	// awarded (idempotent per wave inside the subsystem); the final
	// wave awards at session over.
	int32 XPWaveTracked = 0;
	int64 XPWaveHoardStart = 0;
	bool bXPFinalWaveAwarded = false;
	float GroundScorchAccum = 0.0f;
	float GroundScorchTimer = 0.0f;

	// [2026-11-01 T-077] Crush application, pawn side: the landing
	// path damages every thief/goat within Combat->CrushRadius of
	// Centre; the run-over path damages only the contacted actor.
	// Both ask UDragonCombatComponent::CrushDamageForImpact first —
	// zero damage means no victim is touched and nothing is logged
	// as a crush. Every actual crush logs one line naming it, which
	// is the PIE evidence the task requires.
	void ApplyLandingCrush(const FVector& Centre, float ImpactSpeed);
	void ApplyCrushVictim(AActor* Victim, float ImpactSpeed, const TCHAR* Source);

	// [2026-11-01 T-077] World-time of the last run-over crush.
	// NotifyHit fires repeatedly while two capsules grind together;
	// this 0.5 s debounce (checked in NotifyHit) makes one pass one
	// crush per half-second instead of per-physics-step damage.
	float LastCrushTimeSeconds = -1000.0f;
	// [2026-11-04 T-102] LT/Shift brake latch (prototype integrator
	// reads it beside bDiving in UpdatePrototypeFlight).
	bool bBraking = false;
	// [2026-11-04 T-102] Right-face multiplex press state: world
	// time the press started, whether the button is still down,
	// and whether the hold verb already fired for this press.
	float RightFacePressTime = -1000.0f;
	bool bRightFaceDown = false;
	bool bRightFaceHoldFired = false;
	// [2026-11-04 T-070] The carried treasure actor (presentation
	// follow only — registry truth lives in UDragonCarryComponent).
	UPROPERTY()
	TObjectPtr<AActor> CarriedTreasureActor = nullptr;
	// [2026-11-04 T-076/T-102] Claw helpers: one gather for the
	// pending window, one arc probe for the right-face tap choice.
	void GatherClawTargets(TArray<UThiefBrain*>& OutBrains, TArray<FVector>& OutLocations,
						   TArray<FVector>& OutForwards);
	bool HasClawTargetInArc();
	void TickRightFaceHold();
	void TickCarryFollow();

	bool bDiving = false;

	// [2026-10-28 T-015] True while the fire input is held; Tick turns
	// it into TryStartFire + TickFire so input stays a latch, not logic.
	bool bWantsFire = false;

	// [2026-10-28 T-015] Corpse actor being eaten (thief meals only —
	// a goat hides itself in Consume). UPROPERTY TObjectPtr so the GC
	// keeps it reachable across the channel (the T-035 lesson); Tick
	// hides and clears it when TickEat reports AteThief.
	UPROPERTY()
	TObjectPtr<AActor> PendingEatActor = nullptr;

	// [2026-10-28 T-043] Last state Tick applied to the single-node
	// mesh, so Tick only calls Play/Stop on SelectAnimState
	// transitions (replaces the T-032 bPlayingWalk bool).
	// [2026-11-04 T-125] Single-node mode only; ABP mode leaves this
	// untouched (the graph owns locomotion playback).
	EDragonAnimState CurrentAnimState = EDragonAnimState::Idle;

	// [2026-11-04 T-125] True when ABP_Dragon was on disk at construction:
	// mesh runs AnimationBlueprint mode, locomotion/tilt driver blocks
	// are skipped, one-shots play through the graph's slots. False keeps
	// the shipped single-node path byte-identical. Never flips at runtime.
	bool bUsingABP = false;

	FName OneShotSlotFor(UAnimSequence* Clip) const;

	// [2026-11-01 T-068] One-shot playback clock: while > 0 the locomotion
	// [2026-11-01 T-068] driver is suppressed; at expiry Tick forces a
	// [2026-11-01 T-068] locomotion re-entry by invalidating CurrentAnimState.
	float OneShotTimeRemaining = 0.0f;

	// [2026-11-01 T-068] Smoothed visual mesh tilt (degrees) + last yaw, for
	// [2026-11-01 T-068] bank-from-turn-rate. Display only: capsule untouched.
	float MeshBankDegrees = 0.0f;
	float MeshPitchDegrees = 0.0f;
	float LastActorYaw = 0.0f;

	// [2026-10-28 T-011] Last Tick's vertical velocity (cm/s, signed).
	// Captured before Tick's flight logic runs so Landed() can recover
	// the impact speed the movement component has already discarded.
	float LastVelocityZ = 0.0f;

	// [2026-10-28 T-014] Trap-slow countdown + the un-slowed speeds
	// captured on first application (restored exactly at expiry).
	float TrapSlowTimeRemaining = 0.0f;
	float UnslowedWalkSpeed = 0.0f;
	float UnslowedFlySpeed = 0.0f;
};
