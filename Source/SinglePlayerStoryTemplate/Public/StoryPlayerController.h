// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StoryPlayerController.generated.h"

/**
 * AStoryPlayerController
 * 
 * Template player controller for story-driven gameplay.
 * Demonstrates:
 * - Enhanced Input System integration
 * - Camera control patterns
 * - Interaction with UETPFCore systems
 * - Player input routing
 * 
 * Uses Enhanced Input for flexible, data-driven input mapping.
 */
UCLASS()
class SINGLEPLAYERSTORYTEMPLATE_API AStoryPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AStoryPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	/**
	 * Input Mapping Context for gameplay
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* GameplayMappingContext;

	/**
	 * Priority for gameplay input mapping
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 GameplayMappingPriority = 0;

	/**
	 * Setup camera for gameplay
	 */
	void SetupGameplayCamera();
};
