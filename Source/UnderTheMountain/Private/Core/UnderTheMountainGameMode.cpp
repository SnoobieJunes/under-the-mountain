// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Constructor-only game mode: DefaultPawnClass is the
// dragon; everything else is engine defaults until a task needs more.
// [2026-11-03 T-072] The task that needed more: the game-flow shell.
// BeginPlay opens on the title screen (world paused) unless the
// autostart cvar or a restart "?autostart=1" URL option says to skip
// straight into the run (headless captures, scripted courses).

#include "Core/UnderTheMountainGameMode.h"

#include "Core/GameFlowMachine.h"
#include "UnderTheMountain.h"
#include "Dragon/DragonCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GameFlowWidget.h"

// [2026-11-03 T-072] Headless/scripted runs skip the title screen and
// start playing immediately (paused title would freeze captures and
// PLAY-state-only proves like the thief steal loop). Humans always
// get the title (default 0).
TAutoConsoleVariable<int32> CVarFlowAutostart(
	TEXT("utm.gameflow.autostart"), 0,
	TEXT("1 = skip the title screen and start the run immediately (automation)."));

AUnderTheMountainGameMode::AUnderTheMountainGameMode()
{
	DefaultPawnClass = ADragonCharacter::StaticClass();
	// [2026-11-03 T-072] One autostart check per frame while the title
	// shows (see Tick): ticks even while the title screen pauses the
	// world, so a late -ExecCmds cvar still skips the title.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AUnderTheMountainGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// [2026-11-03 T-072] Autostart is re-checked while the title shows:
	// -ExecCmds cvars land after BeginPlay AND after the first tick, so
	// a one-shot check can consume itself before the cvar arrives. One
	// branch per frame at title; zero cost once the run starts.
	if (FlowMachine != nullptr &&
		FlowMachine->GetState() == UGameFlowMachine::EState::Title &&
		CVarFlowAutostart.GetValueOnGameThread() != 0)
	{
		StartRun();
	}
}

void AUnderTheMountainGameMode::BeginPlay()
{
	Super::BeginPlay();

	FlowMachine = NewObject<UGameFlowMachine>(this);
	FlowWidget = CreateWidget<UGameFlowWidget>(GetWorld(), UGameFlowWidget::StaticClass());
	if (FlowWidget != nullptr)
	{
		FlowWidget->AddToViewport(100);
		FlowWidget->BindToGameMode(this);
	}

	// [2026-11-03 T-072] A restart reloads the level with ?autostart=1
	// so the fresh run lands straight in Playing (one click, standard);
	// the URL option is per-load, so normal launches still open titled.
	const bool bAutostart = CVarFlowAutostart.GetValueOnGameThread() != 0 ||
							GetWorld()->URL.HasOption(TEXT("autostart"));
	if (bAutostart)
	{
		StartRun();
	}
	else
	{
		ApplyFlowState();
	}
}

bool AUnderTheMountainGameMode::HasRunStarted() const
{
	return FlowMachine != nullptr && FlowMachine->GetState() !=
										 UGameFlowMachine::EState::Title;
}

UGameFlowMachine::EState AUnderTheMountainGameMode::GetFlowState() const
{
	return FlowMachine != nullptr ? FlowMachine->GetState()
								  : UGameFlowMachine::EState::Title;
}

APlayerController* AUnderTheMountainGameMode::GetFlowController() const
{
	return GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
}

void AUnderTheMountainGameMode::ApplyFlowState()
{
	if (FlowMachine == nullptr)
	{
		return;
	}
	const bool bMenu = !FlowMachine->IsPlaying();
	UGameplayStatics::SetGamePaused(GetWorld(), bMenu);
	if (APlayerController* PC = GetFlowController())
	{
		if (bMenu)
		{
			FInputModeUIOnly InputMode;
			if (FlowWidget != nullptr)
			{
				InputMode.SetWidgetToFocus(FlowWidget->TakeWidget());
			}
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		else
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("Game flow: %s"),
		   FlowMachine->GetState() == UGameFlowMachine::EState::Title	  ? TEXT("title (paused)")
		   : FlowMachine->GetState() == UGameFlowMachine::EState::Playing ? TEXT("playing")
		   : FlowMachine->GetState() == UGameFlowMachine::EState::Paused  ? TEXT("paused")
		   : FlowMachine->GetState() == UGameFlowMachine::EState::Won	  ? TEXT("won")
																		  : TEXT("lost"));
	if (FlowWidget == nullptr)
	{
		return;
	}
	switch (FlowMachine->GetState())
	{
	case UGameFlowMachine::EState::Title:
		FlowWidget->ShowTitle();
		break;
	case UGameFlowMachine::EState::Paused:
		FlowWidget->ShowPause();
		break;
	case UGameFlowMachine::EState::Won:
	case UGameFlowMachine::EState::Lost:
		FlowWidget->ShowEnd(FlowMachine->GetResult());
		break;
	case UGameFlowMachine::EState::Playing:
	default:
		FlowWidget->HideAll();
		break;
	}
}

void AUnderTheMountainGameMode::StartRun()
{
	if (FlowMachine != nullptr && FlowMachine->StartPlay())
	{
		ApplyFlowState();
	}
}

void AUnderTheMountainGameMode::TogglePauseRun()
{
	if (FlowMachine != nullptr && FlowMachine->TogglePause())
	{
		ApplyFlowState();
	}
}

void AUnderTheMountainGameMode::RestartRun()
{
	// [2026-11-03 T-072] Restart = reload the map: a fresh world means
	// fresh subsystems (notoriety, hoard, waves, registry), fresh
	// placed actors (thieves, treasures, burnables), a fresh pawn
	// (pools, position), and rebuilt scent visuals — every item on the
	// clean-run list resets because nothing run-specific survives a
	// load. Only SaveGame prefs persist (camera choice, tint: player
	// settings, not run state, by design). The ?autostart=1 URL option
	// lands the fresh world straight in Playing (see BeginPlay); the
	// machine transition below only guards legality (no reload from a
	// state that must not restart).
	if (FlowMachine != nullptr && FlowMachine->RestartRun())
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		UGameplayStatics::OpenLevel(this,
									FName(*UGameplayStatics::GetCurrentLevelName(GetWorld(), true)),
									true, TEXT("?autostart=1"));
	}
}

void AUnderTheMountainGameMode::QuitToTitle()
{
	if (FlowMachine == nullptr)
	{
		return;
	}
	FlowMachine->QuitToTitle();
	ApplyFlowState();
}

void AUnderTheMountainGameMode::QuitGame()
{
	// [2026-11-03 T-072] The engine quit console command ends a PIE
	// session and exits a packaged build alike — one path for both.
	if (APlayerController* PC = GetFlowController())
	{
		PC->ConsoleCommand(TEXT("quit"));
	}
}

void AUnderTheMountainGameMode::RequestSessionEnd(EGameFlowOutcome Outcome, int64 CurrentHoard,
												  int64 StartingHoard, int32 Stars, int32 Wave)
{
	if (FlowMachine == nullptr)
	{
		return;
	}
	FGameFlowResult Result;
	Result.Outcome = Outcome;
	Result.CurrentHoard = CurrentHoard;
	Result.StartingHoard = StartingHoard;
	Result.Stars = Stars;
	Result.FinalWave = Wave;
	if (FlowMachine->RequestEnd(Result))
	{
		ApplyFlowState();
	}
}
