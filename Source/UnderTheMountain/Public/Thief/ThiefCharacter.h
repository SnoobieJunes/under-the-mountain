// Copyright Under the Mountain contributors.
// Purpose: the in-world thief — a greybox ACharacter whose UThiefBrain
// decides where to go (treasure, then exit) while this actor only
// walks, mirrors the brain's speeds, and forwards roar/damage events.
// Created 2026-10-28 (T-014).
// Invariants: all steal/flee decisions live in UThiefBrain; this actor
// never reads the treasure registry directly, and the placeholder body
// is visual-only so capsule collision stays the single movement truth.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ThiefCharacter.generated.h"

class UStaticMeshComponent;
class UThiefBrain;
class UThiefDataAsset;

UCLASS()
class UNDERTHEMOUNTAIN_API AThiefCharacter : public ACharacter
{
	GENERATED_BODY()

  public:
	AThiefCharacter();

	// [2026-10-28 T-014] Authored thief type (burglar/adventurer/dwarf
	// variants are data, not subclasses). Read once at BeginPlay.
	UPROPERTY(EditAnywhere, Category = "Thief")
	TObjectPtr<UThiefDataAsset> ThiefData;

	// [2026-10-28 T-014] World location the thief runs to in
	// Flee/Scared — authored per spawn so cave mouths stay level data.
	UPROPERTY(EditAnywhere, Category = "Thief")
	FVector ExitLocation = FVector::ZeroVector;

	// [2026-10-28 T-014] The decision-maker, created in BeginPlay once
	// the world's treasure registry can be injected into it.
	UPROPERTY(VisibleAnywhere, Category = "Thief")
	TObjectPtr<UThiefBrain> Brain;

	// [2026-10-28 T-014] Greybox body (engine sphere) until real thief
	// meshes exist; scaled from ThiefData.BodyScale at BeginPlay.
	UPROPERTY(VisibleAnywhere, Category = "Thief")
	TObjectPtr<UStaticMeshComponent> Body;

	UThiefBrain* GetBrain() const;

	// [2026-10-28 T-014] Dragon-roar hook for combat code: forwards to
	// the brain, which owns the drop/scare rules. No-op (with the
	// BeginPlay warnings as evidence) before the brain exists.
	void OnRoarHeard();

	// [2026-10-28 T-014] Dragon-attack hook: forwards to the brain and
	// reports whether the blow killed the thief.
	bool ApplyThiefDamage(float Damage);

  protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

  private:
	// [2026-10-28 T-014] Latches after the escape is logged so the
	// "reached exit" line prints exactly once.
	bool bLoggedExit = false;
};
