// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainMenuHUD.generated.h"

class UMainMenuWidget;

/**
 * AMainMenuHUD
 * 
 * HUD for main menu.
 * Manages UMG widget display and menu UI state.
 */
UCLASS()
class GAMELAUNCHER_API AMainMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	AMainMenuHUD();

	virtual void BeginPlay() override;

	/**
	 * Show the main menu widget
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMainMenu();

	/**
	 * Hide the main menu widget
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void HideMainMenu();

protected:
	/**
	 * Main menu widget class (set in Blueprint)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

	/**
	 * Current menu widget instance
	 */
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> MainMenuWidget;
};
