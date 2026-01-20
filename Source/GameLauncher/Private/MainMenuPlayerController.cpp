// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "MainMenuPlayerController.h"
#include "MainMenuGameMode.h"
#include "Camera/CameraActor.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMainMenuPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Ensure mouse cursor visibility for UI interaction
	bShowMouseCursor = true;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Configure input mode for UI-only interaction
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	UE_LOG(LogTemp, Log, TEXT("MainMenuPlayerController::BeginPlay - UI-only input mode initialized"));
}

void AMainMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// UI widgets handle all input through their internal click event handlers
	UE_LOG(LogTemp, Log, TEXT("MainMenuPlayerController::SetupInputComponent - No custom input bindings required"));
}

ACameraActor* AMainMenuPlayerController::GetMenuCamera() const
{
	if (AMainMenuGameMode* MenuGameMode = Cast<AMainMenuGameMode>(GetWorld()->GetAuthGameMode()))
	{
		return MenuGameMode->GetMenuCamera();
	}
	return nullptr;
}




