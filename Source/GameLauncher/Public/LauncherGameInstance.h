// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LauncherGameInstance.generated.h"

class UGlobalSaveGame;
class UMenuSubsystem;
class UModuleLoaderSubsystem;

/**
 * ULauncherGameInstance
 * 
 * Main game instance for the launcher.
 * Manages:
 * - Global settings (graphics, audio, controls)
 * - Module loading/unloading
 * - Persistent state across level transitions
 * - Subsystem coordination
 * 
 * This is a singleton that persists for the entire application lifetime.
 */
UCLASS()
class GAMELAUNCHER_API ULauncherGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	ULauncherGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	// ========================================
	// Global Settings
	// ========================================

	/**
	 * Load global settings from save game
	 */
	UFUNCTION(BlueprintCallable, Category = "Launcher|Settings")
	void LoadGlobalSettings();

	/**
	 * Save global settings to save game
	 */
	UFUNCTION(BlueprintCallable, Category = "Launcher|Settings")
	void SaveGlobalSettings();

	/**
	 * Get global save game object
	 */
	UFUNCTION(BlueprintPure, Category = "Launcher|Settings")
	UGlobalSaveGame* GetGlobalSaveGame() const { return GlobalSaveGame; }

	// ========================================
	// Module Management
	// ========================================

	/**
	 * Load a gameplay module by name
	 * 
	 * @param ModuleName - Name of module (e.g., "SinglePlayerStoryTemplate", "Multiplayer")
	 */
	UFUNCTION(BlueprintCallable, Category = "Launcher|Modules")
	void LoadGameplayModule(const FString& ModuleName);

	/**
	 * Get the currently loaded gameplay module name
	 */
	UFUNCTION(BlueprintPure, Category = "Launcher|Modules")
	FString GetCurrentModuleName() const { return CurrentModuleName; }

	// ========================================
	// Subsystem Access
	// ========================================

	/**
	 * Get the menu subsystem
	 */
	UFUNCTION(BlueprintPure, Category = "Launcher|Subsystems")
	UMenuSubsystem* GetMenuSubsystem() const;

	/**
	 * Get the module loader subsystem
	 */
	UFUNCTION(BlueprintPure, Category = "Launcher|Subsystems")
	UModuleLoaderSubsystem* GetModuleLoaderSubsystem() const;

protected:
	/**
	 * Global save game object (graphics, audio, controls)
	 */
	UPROPERTY()
	TObjectPtr<UGlobalSaveGame> GlobalSaveGame;

	/**
	 * Name of currently loaded gameplay module
	 */
	UPROPERTY()
	FString CurrentModuleName;

	/**
	 * Global save game slot name
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	FString GlobalSettingsSlotName = TEXT("GlobalSettings");

	/**
	 * Global save game user index
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	int32 GlobalSettingsUserIndex = 0;
};
