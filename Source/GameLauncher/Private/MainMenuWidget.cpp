// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "MainMenuWidget.h"
#include "MenuSubsystem.h"
#include "Camera/CameraActor.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Play show animation
	PlayShowAnimation();

	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::NativeConstruct - Main menu widget constructed"));
}

void UMainMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::NativeDestruct - Main menu widget destroyed"));
}

// ========================================
// Menu Actions
// ========================================

void UMainMenuWidget::OnStartGameClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::OnStartGameClicked - Starting story template module"));

	// Trigger motion design event
	OnMenuItemClicked(TEXT("StartGame"));

	// Start game through menu subsystem
	if (UMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->StartGame(TEXT("SinglePlayerStoryTemplate"));
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::OnSettingsClicked - Opening settings"));

	// Trigger motion design event
	OnMenuItemClicked(TEXT("Settings"));

	// Open settings through menu subsystem
	if (UMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenSettings();
	}
}

void UMainMenuWidget::OnModuleSelectClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::OnModuleSelectClicked - Opening module selection"));

	// Trigger motion design event
	OnMenuItemClicked(TEXT("ModuleSelect"));

	// Transition to module select state
	if (UMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->TransitionToState(EMenuState::ModuleSelect, true);
	}
}

void UMainMenuWidget::OnExitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::OnExitClicked - Exiting game"));

	// Trigger motion design event
	OnMenuItemClicked(TEXT("Exit"));

	// Exit through menu subsystem
	if (UMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->ExitGame();
	}
}

// ========================================
// Camera Integration
// ========================================

ACameraActor* UMainMenuWidget::GetMenuCamera() const
{
	if (AMainMenuGameMode* GameMode = Cast<AMainMenuGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GameMode->GetMenuCamera();
	}
	return nullptr;
}

void UMainMenuWidget::TriggerCameraTransition(const FString& TransitionName)
{
	// This can be expanded to support named camera transitions
	// For now, just log the request
	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget::TriggerCameraTransition - Transition: %s"), *TransitionName);

	// Blueprint can override this to implement specific camera movements
	// based on transition name
}

// ========================================
// Protected Methods
// ========================================

UMenuSubsystem* UMainMenuWidget::GetMenuSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UMenuSubsystem>();
	}
	return nullptr;
}
