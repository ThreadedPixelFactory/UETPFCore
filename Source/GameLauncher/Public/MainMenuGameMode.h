// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class ACameraActor;

/**
 * AMainMenuGameMode
 * 
 * Game mode for the main menu interface.
 * 
 * Handles menu initialization and camera setup. Camera actors can be placed in the level
 * or spawned at runtime using the MenuCameraClass property. Override BeginPlay() in
 * Blueprint subclasses to customize menu environment setup.
 * 
 * @see AMainMenuPlayerController
 * @see UMainMenuWidget
 */
UCLASS()
class GAMELAUNCHER_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 * Get the menu camera actor
	 */
	UFUNCTION(BlueprintPure, Category = "Menu")
	ACameraActor* GetMenuCamera() const { return MenuCamera; }

protected:
	/**
	 * Menu camera actor (spawned or placed in level)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	TObjectPtr<ACameraActor> MenuCamera;

	/**
	 * Camera class to spawn if none exists in level (defaults to ACameraActor)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<ACameraActor> MenuCameraClass;

	/**
	 * Default camera position if spawning
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	FVector DefaultCameraLocation = FVector(0.0f, 0.0f, 0.0f);

	/**
	 * Default camera rotation if spawning
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	FRotator DefaultCameraRotation = FRotator(0.0f, 0.0f, 0.0f);

	/**
	 * Validate game mode configuration (logs warnings/errors if misconfigured)
	 */
	void ValidateConfiguration();

	/**
	 * Setup menu camera. Override in Blueprint to customize camera setup behavior.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	virtual void SetupMenuCamera();
};
