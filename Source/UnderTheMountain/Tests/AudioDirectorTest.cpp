// [2026-11-04 T-071] Audio seam pure-logic tests (no assets, no world audio).
#include "Misc/AutomationTest.h"
#include "Audio/AudioDirector.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioDirectorTest, "UnderTheMountain.Audio.Director",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAudioDirectorTest::RunTest(const FString& P)
{
	for (FName Verb : {TEXT("Roar"), TEXT("Fire"), TEXT("Eat"), TEXT("Land"), TEXT("Flap"), TEXT("WingBuffet"), TEXT("TailSlam"), TEXT("Takeoff"), TEXT("Claw"), TEXT("Crush")})
		TestFalse(*FString::Printf(TEXT("verb %s has a cue"), *Verb.ToString()), FAudioDirectorRules::CueForDragonVerb(Verb).IsNone());
	TestTrue(TEXT("unknown verb fails closed"), FAudioDirectorRules::CueForDragonVerb(TEXT("Sing")).IsNone());
	TestEqual(TEXT("soft landing silent"), FAudioDirectorRules::LandingGainForImpact(100.f), 0.f);
	TestEqual(TEXT("hard landing full"), FAudioDirectorRules::LandingGainForImpact(2200.f), 1.f);
	TestTrue(TEXT("mid landing scaled"), FAudioDirectorRules::LandingGainForImpact(1250.f) > 0.4f && FAudioDirectorRules::LandingGainForImpact(1250.f) < 0.6f);
	TestEqual(TEXT("master volume scales"), FAudioDirectorRules::EffectiveGain(1.f, 0.5f), 0.5f);
	TestEqual(TEXT("gain clamped"), FAudioDirectorRules::EffectiveGain(2.f, 2.f), 1.f);
	TestTrue(TEXT("title wins"), FAudioDirectorRules::TrackForState(true, true, true) == EAudioTrackSlot::Title);
	TestTrue(TEXT("combat over lair"), FAudioDirectorRules::TrackForState(false, true, true) == EAudioTrackSlot::Combat);
	TestTrue(TEXT("lair"), FAudioDirectorRules::TrackForState(false, false, true) == EAudioTrackSlot::Lair);
	TestTrue(TEXT("exterior"), FAudioDirectorRules::TrackForState(false, false, false) == EAudioTrackSlot::Exterior);
	UAudioDirectorComponent* D = NewObject<UAudioDirectorComponent>();
	D->SetMasterVolume(2.f);
	TestEqual(TEXT("volume clamped high"), D->GetMasterVolume(), 1.f);
	D->SetMasterVolume(-1.f);
	TestEqual(TEXT("volume clamped low"), D->GetMasterVolume(), 0.f);
	D->SetMasterVolume(0.5f);
	TestTrue(TEXT("land cue selected"), D->PlayDragonVerb(TEXT("Land"), 2200.f) == FName(TEXT("SFX_DragonLand")));
	TestEqual(TEXT("land gain through master"), D->LastGain(), 0.5f);
	TestTrue(TEXT("roar cue selected"), D->PlayDragonVerb(TEXT("Roar")) == FName(TEXT("SFX_DragonRoar")));
	D->SetMusicState(false, false, true);
	TestTrue(TEXT("component lair slot"), D->CurrentTrackSlot() == EAudioTrackSlot::Lair);
	TestTrue(TEXT("component lair ambience"), D->CurrentAmbience() == EAudioAmbience::Lair);
	D->SetMusicState(false, false, false);
	TestTrue(TEXT("component exterior ambience"), D->CurrentAmbience() == EAudioAmbience::Exterior);
	return true;
}
#endif
