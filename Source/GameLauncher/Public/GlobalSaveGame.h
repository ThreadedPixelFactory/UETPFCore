// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GlobalSaveGame.generated.h"

/**
 * Graphics Quality Preset
 */
UENUM(BlueprintType)
enum class EGraphicsQuality : uint8
{
	Low        UMETA(DisplayName = "Low"),
	Medium     UMETA(DisplayName = "Medium"),
	High       UMETA(DisplayName = "High"),
	Epic       UMETA(DisplayName = "Epic"),
	Cinematic  UMETA(DisplayName = "Cinematic"),
	Custom     UMETA(DisplayName = "Custom")
};

/**
 * UGlobalSaveGame
 * 
 * Persistent storage for global settings that apply across all gameplay modules.
 * Includes:
 * - Graphics settings (resolution, quality, VSync, etc.)
 * - Audio settings (master, music, SFX volumes)
 * - Input settings (sensitivity, key bindings)
 * - Accessibility settings
 * - Last played module and save slot
 * 
 * Saved asynchronously to avoid blocking gameplay.
 * Hot-reload safe with version serialization.
 */
UCLASS()
class GAMELAUNCHER_API UGlobalSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UGlobalSaveGame();

	// ========================================
	// Graphics Settings
	// ========================================

	/** Screen resolution (width x height) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	FIntPoint Resolution = FIntPoint(1920, 1080);

	/** Window mode (Fullscreen, Windowed, Borderless) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::Fullscreen;

	/** Graphics quality preset */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	EGraphicsQuality GraphicsQuality = EGraphicsQuality::High;

	/** VSync enabled */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	bool bVSyncEnabled = true;

	/** Frame rate limit (0 = unlimited) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 FrameRateLimit = 0;

	/** Field of view */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	float FieldOfView = 90.0f;

	/** View distance quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 ViewDistanceQuality = 3;

	/** Anti-aliasing quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 AntiAliasingQuality = 3;

	/** Shadow quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 ShadowQuality = 3;

	/** Post-processing quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 PostProcessQuality = 3;

	/** Texture quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 TextureQuality = 3;

	/** Effects quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 EffectsQuality = 3;

	/** Foliage quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 FoliageQuality = 3;

	/** Shading quality (0-3, Epic scale) */
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 ShadingQuality = 3;

	// ========================================
	// Audio Settings
	// ========================================

	/** Master volume (0.0 - 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MasterVolume = 1.0f;

	/** Music volume (0.0 - 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MusicVolume = 0.8f;

	/** SFX volume (0.0 - 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float SFXVolume = 1.0f;

	/** Dialogue volume (0.0 - 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float DialogueVolume = 1.0f;

	/** Ambient volume (0.0 - 1.0) */
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float AmbientVolume = 0.7f;

	// ========================================
	// Input Settings
	// ========================================

	/** Mouse sensitivity */
	UPROPERTY(BlueprintReadWrite, Category = "Input")
	float MouseSensitivity = 1.0f;

	/** Invert Y axis */
	UPROPERTY(BlueprintReadWrite, Category = "Input")
	bool bInvertYAxis = false;

	/** Controller sensitivity */
	UPROPERTY(BlueprintReadWrite, Category = "Input")
	float ControllerSensitivity = 1.0f;

	// ========================================
	// Gameplay Settings
	// ========================================

	/** Last played module name */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	FString LastPlayedModule;

	/** Last used save slot */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	int32 LastSaveSlot = 0;

	// ========================================
	// Save Management
	// ========================================

	/**
	 * Initialize with default settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Save Game")
	void InitializeDefaults();

	/**
	 * Apply all settings to engine/game
	 */
	UFUNCTION(BlueprintCallable, Category = "Save Game")
	void ApplySettings();

	/**
	 * Capture current settings from engine/game
	 */
	UFUNCTION(BlueprintCallable, Category = "Save Game")
	void CaptureCurrentSettings();

	/**
	 * Apply graphics quality preset
	 */
	UFUNCTION(BlueprintCallable, Category = "Save Game")
	void ApplyGraphicsPreset(EGraphicsQuality Preset);

protected:
	/**
	 * Apply graphics settings to engine
	 */
	void ApplyGraphicsSettings();

	/**
	 * Apply audio settings to engine
	 */
	void ApplyAudioSettings();

	/**
	 * Apply input settings to engine
	 */
	void ApplyInputSettings();
};
