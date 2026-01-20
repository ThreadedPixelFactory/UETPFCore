// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class ACameraActor;

/**
 * AMainMenuPlayerController
 * 
 * Player controller for the main menu interface.
 * 
 * This controller is configured for UI-only interaction by default.
 * All menu interactions are handled through UMG widget button clicks.
 * 
 * Keyboard and gamepad inputs can be supported by:
 * - Adding Enhanced Input mappings in Blueprint subclasses (BP_MainMenuPlayerController)
 * - Binding input actions in SetupInputComponent()
 * - Changing input mode from FInputModeUIOnly to FInputModeGameAndUI in BeginPlay()
 * - Implementing navigation handlers that route to UMG widget focus system
 * 
 * @see AMainMenuGameMode
 * @see UMainMenuWidget
 */
UCLASS()
class GAMELAUNCHER_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/**
	 * Get the menu camera actor
	 */
	UFUNCTION(BlueprintPure, Category = "Menu")
	ACameraActor* GetMenuCamera() const;

protected:
	/**
	 * Show mouse cursor in menu
	 */
	virtual void PostInitializeComponents() override;
};
