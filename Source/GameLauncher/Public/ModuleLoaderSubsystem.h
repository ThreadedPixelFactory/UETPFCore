// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModuleLoaderSubsystem.generated.h"

/**
 * UModuleLoaderSubsystem
 * 
 * Handles dynamic loading and unloading of gameplay modules.
 * Manages:
 * - Module validation
 * - Level streaming for module maps
 * - Module state persistence
 * - Loading screen coordination
 * 
 * Supports modular architecture where gameplay modules (SinglePlayerStoryTemplate, Multiplayer)
 * can be loaded on demand.
 */
UCLASS()
class GAMELAUNCHER_API UModuleLoaderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================
	// Module Loading API
	// ========================================

	/**
	 * Load a gameplay module by name
	 * 
	 * @param ModuleName - Name of module (e.g., "SinglePlayerStoryTemplate", "Multiplayer")
	 * @return true if loading initiated successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Module Loader")
	bool LoadModule(const FString& ModuleName);

	/**
	 * Unload the currently loaded module and return to menu
	 */
	UFUNCTION(BlueprintCallable, Category = "Module Loader")
	void UnloadCurrentModule();

	/**
	 * Check if a module is currently loaded
	 */
	UFUNCTION(BlueprintPure, Category = "Module Loader")
	bool IsModuleLoaded() const { return !CurrentModuleName.IsEmpty(); }

	/**
	 * Get the name of currently loaded module
	 */
	UFUNCTION(BlueprintPure, Category = "Module Loader")
	FString GetCurrentModuleName() const { return CurrentModuleName; }

	/**
	 * Get list of available modules
	 */
	UFUNCTION(BlueprintPure, Category = "Module Loader")
	TArray<FString> GetAvailableModules() const;

protected:
	/**
	 * Name of currently loaded module
	 */
	UPROPERTY()
	FString CurrentModuleName;

	/**
	 * Map of module names to their entry map paths
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Module Loader")
	TMap<FString, FString> ModuleMapPaths;

	/**
	 * Default menu map to return to on unload
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Module Loader")
	FString MenuMapPath = TEXT("/Game/_Launcher/Maps/MainMenu");

	// ========================================
	// Internal Methods
	// ========================================

	/**
	 * Validate module name and check if it exists
	 */
	bool ValidateModule(const FString& ModuleName) const;

	/**
	 * Get map path for module
	 */
	FString GetModuleMapPath(const FString& ModuleName) const;
};
