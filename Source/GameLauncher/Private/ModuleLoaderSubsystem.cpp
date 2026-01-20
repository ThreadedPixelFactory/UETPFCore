// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "ModuleLoaderSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UModuleLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("ModuleLoaderSubsystem::Initialize - Module loader initialized"));

	// Setup default module map paths, update as needed for your project.
	// These can be configured in DefaultGame.ini or via Data Assets
	ModuleMapPaths.Add(TEXT("SinglePlayerStoryTemplate"), TEXT("/Game/YourGame/Maps/StoryEntry"));
	// eg. ModuleMapPaths.Add(TEXT("Multiplayer"), TEXT("/Game/_Multiplayer/Maps/MultiplayerEntry"));
}

void UModuleLoaderSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("ModuleLoaderSubsystem::Deinitialize - Module loader shutdown"));
}

// ========================================
// Module Loading API
// ========================================

bool UModuleLoaderSubsystem::LoadModule(const FString& ModuleName)
{
	if (!ValidateModule(ModuleName))
	{
		UE_LOG(LogTemp, Error, TEXT("ModuleLoaderSubsystem::LoadModule - Invalid module: %s"), *ModuleName);
		return false;
	}

	// Get map path for module
	const FString MapPath = GetModuleMapPath(ModuleName);
	if (MapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ModuleLoaderSubsystem::LoadModule - No map path for module: %s"), *ModuleName);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ModuleLoaderSubsystem::LoadModule - Loading module %s at path %s"), 
		*ModuleName, *MapPath);

	// Open level (non-blocking, shows loading screen)
	UGameplayStatics::OpenLevel(GetWorld(), FName(*MapPath), true);

	// Update current module
	CurrentModuleName = ModuleName;

	return true;
}

void UModuleLoaderSubsystem::UnloadCurrentModule()
{
	if (!IsModuleLoaded())
	{
		UE_LOG(LogTemp, Warning, TEXT("ModuleLoaderSubsystem::UnloadCurrentModule - No module currently loaded"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ModuleLoaderSubsystem::UnloadCurrentModule - Unloading module %s"), 
		*CurrentModuleName);

	// Clear current module
	CurrentModuleName.Empty();

	// Return to main menu
	UGameplayStatics::OpenLevel(GetWorld(), FName(*MenuMapPath), true);
}

TArray<FString> UModuleLoaderSubsystem::GetAvailableModules() const
{
	TArray<FString> Modules;
	ModuleMapPaths.GetKeys(Modules);
	return Modules;
}

// ========================================
// Internal Methods
// ========================================

bool UModuleLoaderSubsystem::ValidateModule(const FString& ModuleName) const
{
	if (ModuleName.IsEmpty())
	{
		return false;
	}

	// Check if module exists in map
	return ModuleMapPaths.Contains(ModuleName);
}

FString UModuleLoaderSubsystem::GetModuleMapPath(const FString& ModuleName) const
{
	if (const FString* MapPath = ModuleMapPaths.Find(ModuleName))
	{
		return *MapPath;
	}
	return FString();
}
