// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UMenuSubsystem;
class ACameraActor;

/**
 * UMainMenuWidget
 * 
 * Base C++ class for main menu UMG widget.
 * Provides:
 * - Menu state coordination with MenuSubsystem
 * - Camera transition triggers
 * - Input handling for menu navigation
 * - Motion Design integration points
 * 
 * Visual layout is created in UMG Designer (Blueprint child).
 * C++ provides functionality and Motion Design hooks.
 */
UCLASS(Abstract, Blueprintable)
class GAMELAUNCHER_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ========================================
	// Menu Actions (Callable from UMG)
	// ========================================

	/**
	 * Start game with story template module
	 */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void OnStartGameClicked();

	/**
	 * Open settings menu
	 */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void OnSettingsClicked();

	/**
	 * Open module selection menu (for future modules)
	 */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void OnModuleSelectClicked();

	/**
	 * Exit game
	 */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void OnExitClicked();

	// ========================================
	// Motion Design Integration
	// ========================================

	/**
	 * Called when menu item is hovered
	 * Use this to trigger particle effects, camera tweaks, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Motion Design")
	void OnMenuItemHovered(const FString& ItemName);

	/**
	 * Called when menu item is clicked (before action)
	 * Use this to trigger particle explosion, camera transition, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Motion Design")
	void OnMenuItemClicked(const FString& ItemName);

	/**
	 * Called when menu item animation completes
	 * Use this as timing hook for camera transitions
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Motion Design")
	void OnMenuItemAnimationComplete(const FString& ItemName);

	// ========================================
	// Camera Integration
	// ========================================

	/**
	 * Get the menu camera actor
	 */
	UFUNCTION(BlueprintPure, Category = "Main Menu")
	ACameraActor* GetMenuCamera() const;

	/**
	 * Trigger camera transition to a named location
	 * (Locations can be set up as spline points or transforms in level)
	 */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void TriggerCameraTransition(const FString& TransitionName);

protected:
	/**
	 * Get menu subsystem
	 */
	UFUNCTION(BlueprintPure, Category = "Main Menu")
	UMenuSubsystem* GetMenuSubsystem() const;

	// ========================================
	// Animation Events (Blueprint Override)
	// ========================================

	/**
	 * Called when widget is shown (fade in, fly in, etc.)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Animation")
	void PlayShowAnimation();

	/**
	 * Called when widget is hidden (fade out, fly out, etc.)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Animation")
	void PlayHideAnimation();
};
