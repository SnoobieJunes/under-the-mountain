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

// [2026-11-03 T-110] Proof-capture cvars (see the header): delayed
// one-shot HighResShot + optional delayed quit for scripted -game
// captures. Both default off.
TAutoConsoleVariable<float> CVarProofShotDelay(
	TEXT("utm.proof.shotdelay"), 0.0f,
	TEXT("Seconds after the first tick before taking one HighResShot (0 = off)."));
TAutoConsoleVariable<float> CVarProofQuitDelay(
	TEXT("utm.proof.quit"), 0.0f,
	TEXT("Seconds after the proof shot before quitting (0 = stay running)."));

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
	TickProofCapture();
}

void AUnderTheMountainGameMode::TickProofCapture()
{
	// [2026-11-03 T-110] One branch per frame while either cvar is armed;
	// zero cost otherwise (both default 0). The shot fires through the
	// player controller's console so it captures exactly the game view
	// (UI included — title/HUD visibility is part of the proof).
	const double Now = FPlatformTime::Seconds();
	if (ProofFirstTickTime < 0.0)
	{
		ProofFirstTickTime = Now;
	}
	if (!bProofShotTaken)
	{
		const float Delay = CVarProofShotDelay.GetValueOnGameThread();
		// [2026-11-04 T-020] Fps window runs alongside the shot countdown
		// [2026-11-04 T-020] (zero cost when the shot is off): skip the first
		// [2026-11-04 T-020] 10 s so first-launch shader compile does not
		// [2026-11-04 T-020] set the number. Buckets separate steady load
		// [2026-11-04 T-020] (most frames in the first two) from hitches.
		if (Delay > 0.0f)
		{
			if (ProofLastTickTime > 0.0 && Now - ProofFirstTickTime > 10.0)
			{
				const double Dt = Now - ProofLastTickTime;
				if (Dt > 0.0)
				{
					++ProofFrames;
					ProofFrameTotal += Dt;
					ProofFrameWorst = FMath::Max(ProofFrameWorst, Dt);
					const double Ms = Dt * 1000.0;
					if (Ms <= 16.6)
					{
						++ProofFrames60;
					}
					else if (Ms <= 33.3)
					{
						++ProofFrames30;
					}
					else
					{
						++ProofFramesSlow;
					}
				}
			}
			ProofLastTickTime = Now;
		}
		if (Delay > 0.0f && Now - ProofFirstTickTime >= Delay)
		{
			bProofShotTaken = true;
			ProofShotTime = Now;
			if (APlayerController* PC = GetFlowController())
			{
				PC->ConsoleCommand(TEXT("HighResShot 1"));
			}
			const double AvgMs = ProofFrames > 0 ? 1000.0 * ProofFrameTotal / static_cast<double>(ProofFrames) : 0.0;
			const double AvgFps = AvgMs > 0.0 ? 1000.0 / AvgMs : 0.0;
			UE_LOG(LogUnderTheMountain, Log,
				   TEXT("Proof capture: HighResShot fired %.1fs after first tick (state %d)"),
				   Now - ProofFirstTickTime, static_cast<int32>(GetFlowState()));
			UE_LOG(LogUnderTheMountain, Log,
				   TEXT("Proof fps: avg %.1f over %d frames (%.2f ms avg, %.2f ms worst)"),
				   AvgFps, ProofFrames, AvgMs, 1000.0 * ProofFrameWorst);
			// [2026-11-04 T-113] Dual channel like CourseLog (T-075):
			// [2026-11-04 T-113] headless -game runs never materialise a log
			// [2026-11-04 T-113] file, so the shot needs stdout proof too.
			printf("PROOF HighResShot fired %.1fs after first tick (state %d)\n", Now - ProofFirstTickTime,
				   static_cast<int32>(GetFlowState()));
			fflush(stdout);
			printf("PROOF fps avg=%.1f frames=%lld avgms=%.2f worstms=%.2f\n", AvgFps,
				   static_cast<long long>(ProofFrames), AvgMs, 1000.0 * ProofFrameWorst);
			fflush(stdout);
			printf("PROOF fpsbuckets at60=%lld at30=%lld slow=%lld\n",
				   static_cast<long long>(ProofFrames60), static_cast<long long>(ProofFrames30),
				   static_cast<long long>(ProofFramesSlow));
			fflush(stdout);
		}
		return;
	}
	const float QuitAfter = CVarProofQuitDelay.GetValueOnGameThread();
	if (QuitAfter > 0.0f && Now - ProofShotTime >= QuitAfter)
	{
		UE_LOG(LogUnderTheMountain, Log, TEXT("Proof capture: quitting %.1fs after the shot"), QuitAfter);
		// One-shot: disarm before quitting so an ignored quit (PIE) cannot loop.
		CVarProofQuitDelay->Set(0.0f);
		QuitGame();
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
