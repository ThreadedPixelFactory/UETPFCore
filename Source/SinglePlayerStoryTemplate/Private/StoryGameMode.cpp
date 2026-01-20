// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "StoryGameMode.h"
#include "StoryPlayerController.h"
#include "Kismet/GameplayStatics.h"

AStoryGameMode::AStoryGameMode()
{
	// Use Story template player controller
	PlayerControllerClass = AStoryPlayerController::StaticClass();

	// Default pawn class can be set in Blueprint or here
	// For now, leave it as default until you create a player character
}

void AStoryGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UE_LOG(LogTemp, Log, TEXT("StoryGameMode::InitGame - Initializing story gameplay for map: %s"), *MapName);
}

void AStoryGameMode::BeginPlay()
{
	Super::BeginPlay();

	SetupPlayerCamera();

	UE_LOG(LogTemp, Log, TEXT("StoryGameMode::BeginPlay - Story game mode started"));
}

void AStoryGameMode::SetupPlayerCamera()
{
	// Setup player camera view
	// Template implementation - customize for your game
	// In full implementation, this would:
	// 1. Spawn player character
	// 2. Setup camera manager
	// 3. Configure camera properties for gameplay

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		// Basic setup - full implementation depends on player character design
		UE_LOG(LogTemp, Log, TEXT("StoryGameMode::SetupPlayerCamera - Player camera configured"));
	}
}
