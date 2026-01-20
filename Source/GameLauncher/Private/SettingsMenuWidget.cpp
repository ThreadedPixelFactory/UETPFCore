// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "SettingsMenuWidget.h"
#include "GlobalSaveGame.h"
#include "MenuSubsystem.h"
#include "LauncherGameInstance.h"

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Load current settings into UI
	LoadSettings();

	UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::NativeConstruct - Settings menu widget constructed"));
}

void USettingsMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::NativeDestruct - Settings menu widget destroyed"));
}

// ========================================
// Settings Management
// ========================================

void USettingsMenuWidget::LoadSettings()
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		// Settings are already in save game
		// Notify Blueprint to update UI
		OnSettingsLoaded();
		
		bHasUnsavedChanges = false;

		UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::LoadSettings - Settings loaded"));
	}
}

void USettingsMenuWidget::ApplySettings()
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->ApplySettings();
		OnSettingsApplied();

		UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::ApplySettings - Settings applied"));
	}
}

void USettingsMenuWidget::SaveSettings()
{
	if (ULauncherGameInstance* Launcher = Cast<ULauncherGameInstance>(GetGameInstance()))
	{
		Launcher->SaveGlobalSettings();
		bHasUnsavedChanges = false;

		UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::SaveSettings - Settings saved"));
	}
}

void USettingsMenuWidget::ResetToDefaults()
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->InitializeDefaults();
		LoadSettings();
		MarkSettingsChanged();

		UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::ResetToDefaults - Reset to defaults"));
	}
}

void USettingsMenuWidget::DiscardChanges()
{
	// Reload settings from save game
	LoadSettings();
	bHasUnsavedChanges = false;

	UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::DiscardChanges - Changes discarded"));
}

// ========================================
// Navigation
// ========================================

void USettingsMenuWidget::OnBackClicked()
{
	// Check for unsaved changes
	if (bHasUnsavedChanges)
	{
		// In production, show confirmation dialog
		// For now, just discard changes
		UE_LOG(LogTemp, Warning, TEXT("SettingsMenuWidget::OnBackClicked - Discarding unsaved changes"));
	}

	// Navigate back to main menu
	if (UMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->NavigateBack();
	}
}

// ========================================
// Graphics Settings
// ========================================

void USettingsMenuWidget::ApplyGraphicsPreset(int32 PresetIndex)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		const EGraphicsQuality Preset = static_cast<EGraphicsQuality>(PresetIndex);
		SaveGame->ApplyGraphicsPreset(Preset);
		MarkSettingsChanged();

		UE_LOG(LogTemp, Log, TEXT("SettingsMenuWidget::ApplyGraphicsPreset - Applied preset %d"), PresetIndex);
	}
}

void USettingsMenuWidget::SetResolution(FIntPoint NewResolution)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->Resolution = NewResolution;
		MarkSettingsChanged();
	}
}

void USettingsMenuWidget::SetWindowMode(int32 ModeIndex)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->WindowMode = static_cast<EWindowMode::Type>(ModeIndex);
		MarkSettingsChanged();
	}
}

void USettingsMenuWidget::SetVSync(bool bEnabled)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->bVSyncEnabled = bEnabled;
		MarkSettingsChanged();
	}
}

// ========================================
// Audio Settings
// ========================================

void USettingsMenuWidget::SetMasterVolume(float Volume)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
		MarkSettingsChanged();
	}
}

void USettingsMenuWidget::SetMusicVolume(float Volume)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
		MarkSettingsChanged();
	}
}

void USettingsMenuWidget::SetSFXVolume(float Volume)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
		MarkSettingsChanged();
	}
}

// ========================================
// Input Settings
// ========================================

void USettingsMenuWidget::SetMouseSensitivity(float Sensitivity)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->MouseSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
		MarkSettingsChanged();
	}
}

void USettingsMenuWidget::SetInvertYAxis(bool bInvert)
{
	if (UGlobalSaveGame* SaveGame = GetGlobalSaveGame())
	{
		SaveGame->bInvertYAxis = bInvert;
		MarkSettingsChanged();
	}
}

// ========================================
// Protected Methods
// ========================================

UGlobalSaveGame* USettingsMenuWidget::GetGlobalSaveGame() const
{
	if (ULauncherGameInstance* Launcher = Cast<ULauncherGameInstance>(GetGameInstance()))
	{
		return Launcher->GetGlobalSaveGame();
	}
	return nullptr;
}

UMenuSubsystem* USettingsMenuWidget::GetMenuSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UMenuSubsystem>();
	}
	return nullptr;
}

void USettingsMenuWidget::MarkSettingsChanged()
{
	bHasUnsavedChanges = true;
}
