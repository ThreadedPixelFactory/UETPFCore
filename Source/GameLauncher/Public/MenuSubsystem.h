// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MenuSubsystem.generated.h"

class UMainMenuWidget;
class USettingsMenuWidget;

/**
 * Menu State Enum
 * 
 * Represents the current menu state in the state machine.
 */
UENUM(BlueprintType)
enum class EMenuState : uint8
{
	MainMenu       UMETA(DisplayName = "Main Menu"),
	Settings       UMETA(DisplayName = "Settings"),
	ModuleSelect   UMETA(DisplayName = "Module Selection"),
	Loading        UMETA(DisplayName = "Loading"),
	None           UMETA(DisplayName = "None")
};

/**
 * Menu Subsystem - Menu State Machine and Coordination
 * 
 * Lifecycle:
 * 1. Initialize() - Called on GameInstance creation, sets initial state
 * 2. TransitionToState() - Handle user navigation (button clicks)
 * 3. State handlers - EnterState/ExitState manage widget visibility
 * 4. Deinitialize() - Cleanup on shutdown
 * 
 * State Machine:
 * ```
 * MainMenu → Settings → (back to) MainMenu
 *         → ModuleSelect → Loading → (game starts)
 *         → Exit (quit game)
 * ```
 * 
 * Responsibilities:
 * - Menu state transitions with history stack
 * - Widget visibility management
 * - Camera transition coordination (future feature)
 * - Input context switching (UI vs gameplay)
 * - Integration with ModuleLoaderSubsystem
 * 
 * Dependencies:
 * - UModuleLoaderSubsystem: For starting gameplay modules
 * - Widget classes: UMainMenuWidget, USettingsMenuWidget
 * 
 * Usage (C++):
 * \code{.cpp}
 *   UMenuSubsystem* Menu = GameInstance->GetSubsystem<UMenuSubsystem>();
 *   Menu->TransitionToState(EMenuState::Settings, true); // Animate
 *   Menu->NavigateBack(); // Return to previous state
 * \endcode
 * 
 * Usage (Blueprint):
 *   Get Game Instance → Get Subsystem (Menu) → Transition To State
 * 
 * @note This is a GameInstance subsystem, persisting across level transitions
 * @note State history maintained for back navigation
 */
UCLASS()
class GAMELAUNCHER_API UMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================
	// Menu State Management
	// ========================================

	/**
	 * Get current menu state
	 */
	UFUNCTION(BlueprintPure, Category = "Menu")
	EMenuState GetCurrentState() const { return CurrentState; }

	/**
	 * Transition to a new menu state
	 * 
	 * @param NewState - Target menu state
	 * @param bAnimate - Whether to animate the transition
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void TransitionToState(EMenuState NewState, bool bAnimate = true);

	/**
	 * Go back to previous menu state
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void NavigateBack();

	/**
	 * Get previous menu state (for back navigation)
	 */
	UFUNCTION(BlueprintPure, Category = "Menu")
	EMenuState GetPreviousState() const { return PreviousState; }

	// ========================================
	// Menu Actions
	// ========================================

	/**
	 * Start the game with selected module
	 * 
	 * @param ModuleName - Name of gameplay module to load
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame(const FString& ModuleName);

	/**
	 * Open settings menu
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenSettings();

	/**
	 * Exit game
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ExitGame();

protected:
	/**
	 * Current menu state
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	EMenuState CurrentState = EMenuState::MainMenu;

	/**
	 * Previous menu state (for back navigation)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	EMenuState PreviousState = EMenuState::None;

	/**
	 * State history stack for back navigation
	 */
	UPROPERTY()
	TArray<EMenuState> StateHistory;

	/**
	 * Maximum state history size
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	int32 MaxStateHistorySize = 10;

	// ========================================
	// Internal Methods
	// ========================================

	/**
	 * Handle entering a new state
	 */
	void EnterState(EMenuState NewState, bool bAnimate);

	/**
	 * Handle exiting current state
	 */
	void ExitState(EMenuState OldState);

	/**
	 * Trigger camera transition for state
	 */
	void TriggerCameraTransition(EMenuState NewState);
};
