// Copyright Under the Mountain contributors.
// [2026-11-01 T-018] UIntroSubsystem implementation.

#include "Intro/IntroSubsystem.h"
#include "Notoriety/NotorietySubsystem.h"
#include "UnderTheMountain.h"

namespace
{
// [2026-11-01 T-018] State timings: 2 s asleep (the world
// settles), 2.5 s wake (the camera move's duration), 20 s to
// decide before the game decides for you (Let Go).
constexpr float SleepSeconds = 2.0f;
constexpr float WakeSeconds = 2.5f;
constexpr float ChoiceTimeoutSeconds = 20.0f;
} // namespace

int32 UIntroSubsystem::LetGoPoints()
{
	return 5;
}

int32 UIntroSubsystem::FailedChasePoints()
{
	return 5;
}

EIntroState UIntroSubsystem::GetState() const
{
	return State;
}

EIntroBranch UIntroSubsystem::GetBranch() const
{
	return Branch;
}

float UIntroSubsystem::GetWakeProgress() const
{
	if (State == EIntroState::Sleeping)
	{
		return 0.0f;
	}
	if (State == EIntroState::WakeUp)
	{
		return FMath::Clamp(StateTime / WakeSeconds, 0.0f, 1.0f);
	}
	return 1.0f;
}

void UIntroSubsystem::TickIntro(float DeltaSeconds)
{
	StateTime += DeltaSeconds;
	switch (State)
	{
	case EIntroState::Sleeping:
		if (StateTime >= SleepSeconds)
		{
			State = EIntroState::WakeUp;
			StateTime = 0.0f;
			UE_LOG(LogUnderTheMountain, Log, TEXT("Intro: waking"));
		}
		break;
	case EIntroState::WakeUp:
		if (StateTime >= WakeSeconds)
		{
			State = EIntroState::GnomeSteals;
			StateTime = 0.0f;
			UE_LOG(LogUnderTheMountain, Log, TEXT("Intro: something stirs in the hoard"));
		}
		break;
	case EIntroState::Choice:
		// [2026-11-01 T-018] The anti-soft-lock: an unanswered
		// choice resolves as Let Go, it can never wait forever.
		if (StateTime >= ChoiceTimeoutSeconds)
		{
			Choose(EIntroBranch::LetGo);
		}
		break;
	default:
		break;
	}
}

void UIntroSubsystem::ReportGnomeStole()
{
	if (State != EIntroState::GnomeSteals)
	{
		return;
	}
	State = EIntroState::Choice;
	StateTime = 0.0f;
	UE_LOG(LogUnderTheMountain, Log, TEXT("Intro: the gnome has your gold - choose"));
}

void UIntroSubsystem::Choose(EIntroBranch InBranch)
{
	if (State != EIntroState::Choice)
	{
		return;
	}
	Branch = InBranch;
	State = EIntroState::FreePlay;
	StateTime = 0.0f;
	if (Branch == EIntroBranch::LetGo)
	{
		// [2026-11-01 T-018] Let go is charged now, flat +5 (it
		// talks); ShouldSuppressEscapeGain stops the generic audit
		// charging +8-with-loot on top when the gnome walks out.
		bEscapeCharged = true;
		if (UWorld* World = GetWorld())
		{
			if (UNotorietySubsystem* Notoriety = World->GetSubsystem<UNotorietySubsystem>())
			{
				Notoriety->AddPoints(LetGoPoints(), TEXT("intro: let the gnome go"));
			}
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Intro: branch %s - free play"),
		   Branch == EIntroBranch::LetGo ? TEXT("LetGo") : TEXT("Chase"));
}

void UIntroSubsystem::SkipToFreePlay()
{
	State = EIntroState::FreePlay;
	StateTime = 0.0f;
}

void UIntroSubsystem::SetGnomeName(FName InName)
{
	GnomeName = InName;
}

bool UIntroSubsystem::ShouldSuppressEscapeGain(FName ThiefName, int32& OutReplacementPoints) const
{
	OutReplacementPoints = 0;
	if (ThiefName != GnomeName || Branch == EIntroBranch::None)
	{
		return false;
	}
	if (Branch == EIntroBranch::LetGo)
	{
		// Already charged at the choice; the escape itself is free.
		return true;
	}
	// Chase that failed: the gnome gets out - the intro table
	// charges the flat +5, not the generic +8-with-loot.
	OutReplacementPoints = FailedChasePoints();
	return true;
}
