// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenuWidget.generated.h"

class UGlobalSaveGame;
class UMenuSubsystem;

/**
 * USettingsMenuWidget
 * 
 * Base C++ class for settings menu UMG widget.
 * Provides:
 * - Settings loading/saving through GlobalSaveGame
 * - Real-time settings preview
 * - Settings validation
 * - Back navigation
 * 
 * Visual layout is created in UMG Designer (Blueprint child).
 * C++ provides functionality and save game integration.
 */
UCLASS(Abstract, Blueprintable)
class GAMELAUNCHER_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ========================================
	// Settings Management
	// ========================================

	/**
	 * Load settings from save game into UI
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettings();

	/**
	 * Apply current UI settings to game
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplySettings();

	/**
	 * Save current settings to save game
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	/**
	 * Reset settings to defaults
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetToDefaults();

	/**
	 * Discard unsaved changes
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void DiscardChanges();

	// ========================================
	// Navigation
	// ========================================

	/**
	 * Navigate back to main menu
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OnBackClicked();

	/**
	 * Check if there are unsaved changes
	 */
	UFUNCTION(BlueprintPure, Category = "Settings")
	bool HasUnsavedChanges() const { return bHasUnsavedChanges; }

	// ========================================
	// Graphics Settings
	// ========================================

	/**
	 * Apply graphics quality preset
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void ApplyGraphicsPreset(int32 PresetIndex);

	/**
	 * Set resolution
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetResolution(FIntPoint NewResolution);

	/**
	 * Set window mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetWindowMode(int32 ModeIndex);

	/**
	 * Set VSync
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetVSync(bool bEnabled);

	// ========================================
	// Audio Settings
	// ========================================

	/**
	 * Set master volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);

	/**
	 * Set music volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Volume);

	/**
	 * Set SFX volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float Volume);

	// ========================================
	// Input Settings
	// ========================================

	/**
	 * Set mouse sensitivity
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void SetMouseSensitivity(float Sensitivity);

	/**
	 * Set invert Y axis
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void SetInvertYAxis(bool bInvert);

	// ========================================
	// Blueprint Events
	// ========================================

	/**
	 * Called when settings are loaded into UI
	 * Use this to update UI elements in Blueprint
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Settings")
	void OnSettingsLoaded();

	/**
	 * Called when settings are applied
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Settings")
	void OnSettingsApplied();

protected:
	/**
	 * Get global save game
	 */
	UFUNCTION(BlueprintPure, Category = "Settings")
	UGlobalSaveGame* GetGlobalSaveGame() const;

	/**
	 * Get menu subsystem
	 */
	UFUNCTION(BlueprintPure, Category = "Settings")
	UMenuSubsystem* GetMenuSubsystem() const;

	/**
	 * Mark that settings have changed
	 */
	void MarkSettingsChanged();

private:
	/**
	 * Whether there are unsaved changes
	 */
	UPROPERTY()
	bool bHasUnsavedChanges = false;
};
