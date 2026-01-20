// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StoryGameMode.generated.h"

/**
 * AStoryGameMode
 * 
 * Template game mode for single-player story-driven gameplay.
 * Demonstrates:
 * - Player spawn and camera setup
 * - Story progression patterns
 * - Game state management
 * - Integration with UETPFCore systems
 * 
 * This is the game mode used during gameplay,
 * not for the main menu (use MainMenuGameMode for that).
 */
UCLASS()
class SINGLEPLAYERSTORYTEMPLATE_API AStoryGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStoryGameMode();

	virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
	/**
	 * Setup player camera and spawn point
	 */
	void SetupPlayerCamera();
};
