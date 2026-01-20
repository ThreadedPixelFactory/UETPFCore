// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "MenuSubsystem.h"
#include "LauncherGameInstance.h"
#include "ModuleLoaderSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

void UMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::Initialize - Menu subsystem initialized"));

	// Start in main menu state
	CurrentState = EMenuState::MainMenu;
	PreviousState = EMenuState::None;
}

void UMenuSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::Deinitialize - Menu subsystem shutdown"));
}

// ========================================
// Menu State Management
// ========================================

void UMenuSubsystem::TransitionToState(EMenuState NewState, bool bAnimate)
{
	if (NewState == CurrentState)
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuSubsystem::TransitionToState - Already in state %d"), (int32)NewState);
		return;
	}

	// Exit current state
	ExitState(CurrentState);

	// Add current state to history
	StateHistory.Add(CurrentState);
	if (StateHistory.Num() > MaxStateHistorySize)
	{
		StateHistory.RemoveAt(0);
	}

	// Update state
	PreviousState = CurrentState;
	CurrentState = NewState;

	// Enter new state
	EnterState(NewState, bAnimate);

	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::TransitionToState - Transitioned from %d to %d"), 
		(int32)PreviousState, (int32)CurrentState);
}

void UMenuSubsystem::NavigateBack()
{
	if (StateHistory.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuSubsystem::NavigateBack - No state history"));
		return;
	}

	// Get last state from history
	const EMenuState LastState = StateHistory.Last();
	StateHistory.RemoveAt(StateHistory.Num() - 1);

	// Transition without adding to history
	ExitState(CurrentState);
	PreviousState = CurrentState;
	CurrentState = LastState;
	EnterState(LastState, true);

	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::NavigateBack - Navigated back to state %d"), (int32)CurrentState);
}

// ========================================
// Menu Actions
// ========================================

void UMenuSubsystem::StartGame(const FString& ModuleName)
{
	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::StartGame - Starting module: %s"), *ModuleName);

	// Transition to loading state
	TransitionToState(EMenuState::Loading, true);

	// Load gameplay module
	if (ULauncherGameInstance* Launcher = Cast<ULauncherGameInstance>(GetGameInstance()))
	{
		Launcher->LoadGameplayModule(ModuleName);
	}
}

void UMenuSubsystem::OpenSettings()
{
	TransitionToState(EMenuState::Settings, true);
}

void UMenuSubsystem::ExitGame()
{
	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::ExitGame - Exiting game"));

	// Use platform-appropriate quit
	UKismetSystemLibrary::QuitGame(
		GetWorld(),
		nullptr,
		EQuitPreference::Quit,
		false
	);
}

// ========================================
// Internal Methods
// ========================================

void UMenuSubsystem::EnterState(EMenuState NewState, bool bAnimate)
{
	// Trigger camera transition
	if (bAnimate)
	{
		TriggerCameraTransition(NewState);
	}

	// Show appropriate widgets (handled by HUD or Widget system)
	// This would communicate with MainMenuHUD to show/hide widgets

	switch (NewState)
	{
	case EMenuState::MainMenu:
		UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::EnterState - Entering Main Menu"));
		break;

	case EMenuState::Settings:
		UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::EnterState - Entering Settings"));
		break;

	case EMenuState::ModuleSelect:
		UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::EnterState - Entering Module Selection"));
		break;

	case EMenuState::Loading:
		UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::EnterState - Entering Loading"));
		break;

	default:
		break;
	}
}

void UMenuSubsystem::ExitState(EMenuState OldState)
{
	// Hide widgets for old state
	// Cleanup any state-specific resources

	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::ExitState - Exiting state %d"), (int32)OldState);
}

void UMenuSubsystem::TriggerCameraTransition(EMenuState NewState)
{
	// This would trigger camera transitions based on state
	// For example:
	// - MainMenu: Orbit sky
	// - Settings: Pan to star constellation
	// - ModuleSelect: Follow particle trail
	
	// Implementation would query MainMenuGameMode for camera and trigger transitions
	// This can be done in Blueprint or expanded here

	UE_LOG(LogTemp, Log, TEXT("MenuSubsystem::TriggerCameraTransition - Camera transition for state %d"), (int32)NewState);
}
