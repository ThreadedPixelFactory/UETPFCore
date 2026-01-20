// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "StoryPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AStoryPlayerController::AStoryPlayerController()
{
	// Gameplay uses normal input mode (not UI only)
	bShowMouseCursor = false;
}

void AStoryPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Set input mode to game only
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	// Hide mouse cursor for gameplay
	bShowMouseCursor = false;

	// Add input mapping context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (GameplayMappingContext)
		{
			Subsystem->AddMappingContext(GameplayMappingContext, GameplayMappingPriority);
		}
	}

	// Setup camera
	SetupGameplayCamera();

	UE_LOG(LogTemp, Log, TEXT("StoryPlayerController::BeginPlay - Gameplay controller initialized"));
}

void AStoryPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input setup will be done via IMC in Blueprint
	// This allows for data-driven input configuration

	UE_LOG(LogTemp, Log, TEXT("StoryPlayerController::SetupInputComponent - Input component ready"));
}

void AStoryPlayerController::SetupGameplayCamera()
{
	// Camera setup for gameplay
	// This will depend on player character implementation
	// Template implementation - customize for your game
	
	// For now, just ensure we have a valid view target
	if (GetPawn())
	{
		SetViewTargetWithBlend(GetPawn(), 0.5f);
		UE_LOG(LogTemp, Log, TEXT("StoryPlayerController::SetupGameplayCamera - Camera set to player pawn"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StoryPlayerController::SetupGameplayCamera - No pawn to set camera to"));
	}
}
