// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "MainMenuHUD.h"
#include "MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

AMainMenuHUD::AMainMenuHUD()
{
}

void AMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	// Create main menu widget
	if (MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UMainMenuWidget>(GetWorld(), MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("MainMenuHUD::BeginPlay - Main menu widget created and added to viewport"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuHUD::BeginPlay - MainMenuWidgetClass not set"));
	}
}

void AMainMenuHUD::ShowMainMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AMainMenuHUD::HideMainMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}
