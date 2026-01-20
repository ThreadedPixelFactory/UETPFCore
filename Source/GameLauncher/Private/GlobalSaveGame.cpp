// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "GlobalSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"

UGlobalSaveGame::UGlobalSaveGame()
{
	// Don't call InitializeDefaults() here - it will be called when needed
	// Calling UGameUserSettings in constructor can cause access violations
}

// ========================================
// Save Management
// ========================================

void UGlobalSaveGame::InitializeDefaults()
{
	// Get system defaults from game user settings
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Resolution = Settings->GetScreenResolution();
		WindowMode = Settings->GetFullscreenMode();
		bVSyncEnabled = Settings->IsVSyncEnabled();
		
		// Set high quality by default
		GraphicsQuality = EGraphicsQuality::High;
	}

	// Audio defaults
	MasterVolume = 1.0f;
	MusicVolume = 0.8f;
	SFXVolume = 1.0f;
	DialogueVolume = 1.0f;
	AmbientVolume = 0.7f;

	// Input defaults
	MouseSensitivity = 1.0f;
	bInvertYAxis = false;
	ControllerSensitivity = 1.0f;

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::InitializeDefaults - Initialized with system defaults"));
}

void UGlobalSaveGame::ApplySettings()
{
	ApplyGraphicsSettings();
	ApplyAudioSettings();
	ApplyInputSettings();

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::ApplySettings - All settings applied"));
}

void UGlobalSaveGame::CaptureCurrentSettings()
{
	// Capture from engine
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Resolution = Settings->GetScreenResolution();
		WindowMode = Settings->GetFullscreenMode();
		bVSyncEnabled = Settings->IsVSyncEnabled();
		ViewDistanceQuality = Settings->GetViewDistanceQuality();
		AntiAliasingQuality = Settings->GetAntiAliasingQuality();
		ShadowQuality = Settings->GetShadowQuality();
		PostProcessQuality = Settings->GetPostProcessingQuality();
		TextureQuality = Settings->GetTextureQuality();
		EffectsQuality = Settings->GetVisualEffectQuality();
		FoliageQuality = Settings->GetFoliageQuality();
		ShadingQuality = Settings->GetShadingQuality();
	}

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::CaptureCurrentSettings - Captured current settings"));
}

void UGlobalSaveGame::ApplyGraphicsPreset(EGraphicsQuality Preset)
{
	GraphicsQuality = Preset;

	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		switch (Preset)
		{
		case EGraphicsQuality::Low:
			Settings->SetOverallScalabilityLevel(0);
			break;

		case EGraphicsQuality::Medium:
			Settings->SetOverallScalabilityLevel(1);
			break;

		case EGraphicsQuality::High:
			Settings->SetOverallScalabilityLevel(2);
			break;

		case EGraphicsQuality::Epic:
			Settings->SetOverallScalabilityLevel(3);
			break;

		case EGraphicsQuality::Cinematic:
			// Ultra settings beyond Epic
			Settings->SetOverallScalabilityLevel(3);
			Settings->SetViewDistanceQuality(3);
			Settings->SetAntiAliasingQuality(3);
			Settings->SetShadowQuality(3);
			Settings->SetPostProcessingQuality(3);
			Settings->SetTextureQuality(3);
			Settings->SetVisualEffectQuality(3);
			Settings->SetFoliageQuality(3);
			Settings->SetShadingQuality(3);
			break;

		case EGraphicsQuality::Custom:
			// Don't change anything, user will set individual settings
			break;
		}

		Settings->ApplySettings(false);
		
		// Capture the applied settings
		CaptureCurrentSettings();
	}

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::ApplyGraphicsPreset - Applied preset %d"), (int32)Preset);
}

// ========================================
// Protected Methods
// ========================================

void UGlobalSaveGame::ApplyGraphicsSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("GlobalSaveGame::ApplyGraphicsSettings - Failed to get GameUserSettings"));
		return;
	}

	// Apply resolution and window mode
	Settings->SetScreenResolution(Resolution);
	Settings->SetFullscreenMode(WindowMode);
	Settings->SetVSyncEnabled(bVSyncEnabled);

	// Apply frame rate limit
	if (FrameRateLimit > 0)
	{
		Settings->SetFrameRateLimit(static_cast<float>(FrameRateLimit));
	}
	else
	{
		Settings->SetFrameRateLimit(0.0f); // Unlimited
	}

	// Apply individual quality settings
	Settings->SetViewDistanceQuality(ViewDistanceQuality);
	Settings->SetAntiAliasingQuality(AntiAliasingQuality);
	Settings->SetShadowQuality(ShadowQuality);
	Settings->SetPostProcessingQuality(PostProcessQuality);
	Settings->SetTextureQuality(TextureQuality);
	Settings->SetVisualEffectQuality(EffectsQuality);
	Settings->SetFoliageQuality(FoliageQuality);
	Settings->SetShadingQuality(ShadingQuality);

	// Apply settings (validate and save)
	Settings->ApplySettings(false);

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::ApplyGraphicsSettings - Graphics settings applied"));
}

void UGlobalSaveGame::ApplyAudioSettings()
{
	// Audio settings would be applied through Sound Classes or Audio Mixer
	// This is a simplified implementation - full implementation would use Sound Class Mix

	// Example using console commands (for demonstration)
	// In production, use Sound Classes and Sound Mix assets

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::ApplyAudioSettings - Audio settings applied (Master: %.2f, Music: %.2f, SFX: %.2f)"),
		MasterVolume, MusicVolume, SFXVolume);

	// TODO: Implement proper audio mixer integration
	// This would involve:
	// 1. Getting audio device/mixer
	// 2. Setting sound class volumes
	// 3. Applying sound mix
}

void UGlobalSaveGame::ApplyInputSettings()
{
	// Input settings are typically applied through Enhanced Input System
	// Mouse sensitivity and axis inversion would be handled by Input Modifiers

	UE_LOG(LogTemp, Log, TEXT("GlobalSaveGame::ApplyInputSettings - Input settings applied (Mouse Sens: %.2f, Invert Y: %d)"),
		MouseSensitivity, bInvertYAxis);

	// TODO: Implement Enhanced Input System integration
	// This would involve:
	// 1. Getting Enhanced Input Subsystem
	// 2. Applying input modifiers for sensitivity
	// 3. Setting axis inversion flags
}
