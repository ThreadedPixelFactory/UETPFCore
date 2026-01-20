// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "MainMenuGameMode.h"
#include "MainMenuPlayerController.h"
#include "MainMenuHUD.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	// Use menu-specific player controller (can be overridden in Blueprint)
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();

	// Set default HUD class (can be overridden in Blueprint)
	if (!HUDClass)
	{
		HUDClass = AMainMenuHUD::StaticClass();
	}

	// Default camera class (can be overridden in Blueprint to use custom camera actors)
	MenuCameraClass = ACameraActor::StaticClass();
	
	// Default camera position and rotation (adjust in Blueprint for your menu environment)
	DefaultCameraLocation = FVector(0.0f, 0.0f, 200.0f);
	DefaultCameraRotation = FRotator(0.0f, 0.0f, 0.0f);
}

void AMainMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode::InitGame - Initializing menu"));
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	ValidateConfiguration();
	SetupMenuCamera();
}

void AMainMenuGameMode::ValidateConfiguration()
{
	if (!HUDClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode: HUDClass not set"));
	}

	if (!PlayerControllerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: PlayerControllerClass is null"));
	}

	if (!MenuCameraClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode: MenuCameraClass not set, camera will not spawn"));
	}
}

void AMainMenuGameMode::SetupMenuCamera()
{
	if (!MenuCameraClass)
	{
		return;
	}

	// Check for existing camera actor in level with tag "MenuCamera"
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ACameraActor::StaticClass(), FName("MenuCamera"), FoundCameras);

	if (FoundCameras.Num() > 0)
	{
		MenuCamera = Cast<ACameraActor>(FoundCameras[0]);
		UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Using existing camera from level"));
	}
	else
	{
		// Spawn camera at configured location
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = TEXT("MenuCamera");
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		MenuCamera = GetWorld()->SpawnActor<ACameraActor>(
			MenuCameraClass,
			DefaultCameraLocation,
			DefaultCameraRotation,
			SpawnParams
		);

		if (MenuCamera)
		{
			MenuCamera->Tags.Add(FName("MenuCamera"));
			UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Spawned camera at %s"), *DefaultCameraLocation.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: Failed to spawn camera"));
			return;
		}
	}

	// Set as view target for player
	if (MenuCamera)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->SetViewTargetWithBlend(MenuCamera, 0.5f);
		}
	}
}

