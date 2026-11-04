// Copyright Under the Mountain contributors.
// Purpose: [2026-11-04 T-071] the one audio seam. Verbs/world code call
// UAudioDirectorComponent, never PlaySound/PlaySoundAtLocation directly.
// Data-driven slots: a track slot names a music state; the actual
// USoundBase assets are assigned in the editor/BP (Content/Audio/) — code
// only decides WHICH slot/cue and at what gain, which is pure and tested.
// Master volume is the T-088 hook: SetMasterVolume clamps [0,1] and every
// cue/track gain multiplies through it. With no asset assigned a request
// is logged once per cue, never silent, and never crashes.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioDirector.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EAudioTrackSlot : uint8
{
	Title,
	Lair,
	Exterior,
	Combat
};
UENUM(BlueprintType)
enum class EAudioAmbience : uint8
{
	Lair,
	Exterior
};

struct UNDERTHEMOUNTAIN_API FAudioDirectorRules
{
	// [2026-11-04 T-071] Landing cue gain scales with impact speed: silent
	// below the soft threshold, full at/above the hard-landing max (2200,
	// mirroring DragonStats landing curve), linear between.
	static float LandingGainForImpact(float ImpactSpeed);
	static float EffectiveGain(float BaseGain, float MasterVolume);
	static EAudioTrackSlot TrackForState(bool bTitle, bool bInCombat, bool bInsideLair);
	static FName CueForDragonVerb(FName Verb); // Roar/Fire/Eat/Land/Flap/Buffet/TailSlam/Takeoff -> cue name or NAME_None
};

UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UAudioDirectorComponent : public UActorComponent
{
	GENERATED_BODY()
  public:
	UAudioDirectorComponent();
	// [2026-11-04 T-071] Data-driven slots, assigned editor-side.
	UPROPERTY(EditAnywhere, Category = "Audio|Music")
	TObjectPtr<USoundBase> TitleTrack;
	UPROPERTY(EditAnywhere, Category = "Audio|Music")
	TObjectPtr<USoundBase> LairTrack;
	UPROPERTY(EditAnywhere, Category = "Audio|Music")
	TObjectPtr<USoundBase> ExteriorTrack;
	UPROPERTY(EditAnywhere, Category = "Audio|Music")
	TObjectPtr<USoundBase> CombatTrack;
	UPROPERTY(EditAnywhere, Category = "Audio|SFX")
	TMap<FName, TObjectPtr<USoundBase>> SfxCues;
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float MasterVolume = 1.0f;

	void SetMasterVolume(float Volume); // [2026-11-04 T-071] T-088 hook
	float GetMasterVolume() const
	{
		return MasterVolume;
	}
	void SetMusicState(bool bTitle, bool bInCombat, bool bInsideLair);
	EAudioTrackSlot CurrentTrackSlot() const
	{
		return CurrentSlot;
	}
	void SetAmbience(EAudioAmbience A)
	{
		Ambience = A;
	}
	EAudioAmbience CurrentAmbience() const
	{
		return Ambience;
	}
	// Verb seam: returns the cue that WOULD play (tests pin this).
	FName PlayDragonVerb(FName Verb, float ImpactSpeed = 0.0f);
	FName LastCue() const
	{
		return LastCueName;
	}
	float LastGain() const
	{
		return LastCueGain;
	}

  private:
	EAudioTrackSlot CurrentSlot = EAudioTrackSlot::Title;
	EAudioAmbience Ambience = EAudioAmbience::Lair;
	FName LastCueName = NAME_None;
	float LastCueGain = 0.0f;
	TSet<FName> WarnedMissing;
};
