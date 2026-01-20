// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "LauncherGameInstance.h"
#include "GlobalSaveGame.h"
#include "MenuSubsystem.h"
#include "ModuleLoaderSubsystem.h"
#include "Kismet/GameplayStatics.h"

ULauncherGameInstance::ULauncherGameInstance()
{
}

void ULauncherGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Log, TEXT("LauncherGameInstance::Init - Initializing launcher"));

	// Load global settings
	LoadGlobalSettings();

	// Initialize subsystems (they will auto-register)
	// MenuSubsystem and ModuleLoaderSubsystem are created automatically
}

void ULauncherGameInstance::Shutdown()
{
	// Save global settings on shutdown
	SaveGlobalSettings();

	Super::Shutdown();

	UE_LOG(LogTemp, Log, TEXT("LauncherGameInstance::Shutdown - Launcher shutdown complete"));
}

// ========================================
// Global Settings
// ========================================

void ULauncherGameInstance::LoadGlobalSettings()
{
	// Try to load existing save game
	if (UGameplayStatics::DoesSaveGameExist(GlobalSettingsSlotName, GlobalSettingsUserIndex))
	{
		GlobalSaveGame = Cast<UGlobalSaveGame>(
			UGameplayStatics::LoadGameFromSlot(GlobalSettingsSlotName, GlobalSettingsUserIndex)
		);

		if (GlobalSaveGame)
		{
			UE_LOG(LogTemp, Log, TEXT("LauncherGameInstance::LoadGlobalSettings - Loaded existing settings"));
			
			// Apply settings to engine
			GlobalSaveGame->ApplySettings();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("LauncherGameInstance::LoadGlobalSettings - Failed to cast save game"));
		}
	}

	// Create new save game if none exists
	if (!GlobalSaveGame)
	{
		GlobalSaveGame = Cast<UGlobalSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UGlobalSaveGame::StaticClass())
		);

		if (GlobalSaveGame)
		{
			UE_LOG(LogTemp, Log, TEXT("LauncherGameInstance::LoadGlobalSettings - Created new default settings"));
			
			// Initialize with defaults
			GlobalSaveGame->InitializeDefaults();
			
			// Save defaults
			SaveGlobalSettings();
		}
	}
}

void ULauncherGameInstance::SaveGlobalSettings()
{
	if (!GlobalSaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("LauncherGameInstance::SaveGlobalSettings - No save game object"));
		return;
	}

	// Capture current settings
	GlobalSaveGame->CaptureCurrentSettings();

	// Save to disk (async, non-blocking)
	const bool bSuccess = UGameplayStatics::SaveGameToSlot(
		GlobalSaveGame,
		GlobalSettingsSlotName,
		GlobalSettingsUserIndex
	);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("LauncherGameInstance::SaveGlobalSettings - Settings saved successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LauncherGameInstance::SaveGlobalSettings - Failed to save settings"));
	}
}

// ========================================
// Module Management
// ========================================

void ULauncherGameInstance::LoadGameplayModule(const FString& ModuleName)
{
	if (UModuleLoaderSubsystem* ModuleLoader = GetModuleLoaderSubsystem())
	{
		ModuleLoader->LoadModule(ModuleName);
		CurrentModuleName = ModuleName;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LauncherGameInstance::LoadGameplayModule - ModuleLoaderSubsystem not found"));
	}
}

// ========================================
// Subsystem Access
// ========================================

UMenuSubsystem* ULauncherGameInstance::GetMenuSubsystem() const
{
	return GetSubsystem<UMenuSubsystem>();
}

UModuleLoaderSubsystem* ULauncherGameInstance::GetModuleLoaderSubsystem() const
{
	return GetSubsystem<UModuleLoaderSubsystem>();
}
