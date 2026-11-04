// [2026-11-04 T-071] See AudioDirector.h. No PlaySound anywhere else:
// this component is the only caller, and only when an asset is assigned.
#include "Audio/AudioDirector.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "UnderTheMountain.h"

float FAudioDirectorRules::LandingGainForImpact(float Speed)
{
	if (Speed <= 300.0f)
		return 0.0f;
	if (Speed >= 2200.0f)
		return 1.0f;
	return (Speed - 300.0f) / 1900.0f;
}
float FAudioDirectorRules::EffectiveGain(float Base, float Master)
{
	return FMath::Clamp(Base, 0.0f, 1.0f) * FMath::Clamp(Master, 0.0f, 1.0f);
}
EAudioTrackSlot FAudioDirectorRules::TrackForState(bool bTitle, bool bCombat, bool bLair)
{
	if (bTitle)
		return EAudioTrackSlot::Title;
	if (bCombat)
		return EAudioTrackSlot::Combat;
	return bLair ? EAudioTrackSlot::Lair : EAudioTrackSlot::Exterior;
}
FName FAudioDirectorRules::CueForDragonVerb(FName Verb)
{
	// [2026-11-04 T-071] Every dragon verb has a named cue; unknown => None.
	static const TMap<FName, FName> Cues = {
		{TEXT("Roar"), TEXT("SFX_DragonRoar")},
		{TEXT("Fire"), TEXT("SFX_DragonFire")},
		{TEXT("Eat"), TEXT("SFX_DragonEat")},
		{TEXT("Land"), TEXT("SFX_DragonLand")},
		{TEXT("Flap"), TEXT("SFX_DragonFlap")},
		{TEXT("WingBuffet"), TEXT("SFX_WingBuffet")},
		{TEXT("TailSlam"), TEXT("SFX_TailSlam")},
		{TEXT("Takeoff"), TEXT("SFX_DragonTakeoff")},
		{TEXT("Claw"), TEXT("SFX_DragonClaw")},
		{TEXT("Crush"), TEXT("SFX_DragonCrush")},
	};
	const FName* Found = Cues.Find(Verb);
	return Found ? *Found : NAME_None;
}

UAudioDirectorComponent::UAudioDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UAudioDirectorComponent::SetMasterVolume(float V)
{
	MasterVolume = FMath::Clamp(V, 0.0f, 1.0f);
}
void UAudioDirectorComponent::SetMusicState(bool bTitle, bool bCombat, bool bLair)
{
	CurrentSlot = FAudioDirectorRules::TrackForState(bTitle, bCombat, bLair);
	Ambience = bLair ? EAudioAmbience::Lair : EAudioAmbience::Exterior;
	USoundBase* Track = CurrentSlot == EAudioTrackSlot::Title	 ? TitleTrack
						: CurrentSlot == EAudioTrackSlot::Lair	 ? LairTrack
						: CurrentSlot == EAudioTrackSlot::Combat ? CombatTrack
																 : ExteriorTrack;
	if (Track == nullptr && !WarnedMissing.Contains(FName(*UEnum::GetValueAsString(CurrentSlot))))
	{
		WarnedMissing.Add(FName(*UEnum::GetValueAsString(CurrentSlot)));
		UE_LOG(LogUnderTheMountain, Warning, TEXT("AudioDirector: no track assigned for slot %s (editor-side, T-071)"), *UEnum::GetValueAsString(CurrentSlot));
	}
	// Actual crossfade/playback of the assigned track is editor/asset-side;
	// selection logic above is the tested seam.
}
FName UAudioDirectorComponent::PlayDragonVerb(FName Verb, float ImpactSpeed)
{
	LastCueName = FAudioDirectorRules::CueForDragonVerb(Verb);
	const float Base = (Verb == TEXT("Land")) ? FAudioDirectorRules::LandingGainForImpact(ImpactSpeed) : 1.0f;
	LastCueGain = FAudioDirectorRules::EffectiveGain(Base, MasterVolume);
	if (LastCueName == NAME_None)
		return NAME_None;
	if (USoundBase* Cue = SfxCues.FindRef(LastCueName))
	{
		if (AActor* Owner = GetOwner())
			UGameplayStatics::PlaySoundAtLocation(Owner, Cue, Owner->GetActorLocation(), LastCueGain);
	}
	else if (!WarnedMissing.Contains(LastCueName))
	{
		WarnedMissing.Add(LastCueName);
		UE_LOG(LogUnderTheMountain, Warning, TEXT("AudioDirector: cue %s requested, no asset assigned (editor-side, T-071)"), *LastCueName.ToString());
	}
	return LastCueName;
}
