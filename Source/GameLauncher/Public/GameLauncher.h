// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * GameLauncher Module
 * 
 * Provides main menu infrastructure, module loading, and global settings management.
 * This module is lightweight and serves as the entry point for the game, allowing
 * dynamic loading of gameplay modules (SinglePlayerStoryTemplate, Multiplayer, etc.).
 */
class FGameLauncherModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
