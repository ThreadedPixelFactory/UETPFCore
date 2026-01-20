// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * SinglePlayerStoryTemplate Module
 * 
 * Template module for single-player story-driven games, demonstrating:
 * 
 * - FileDeltaStore: Local file-based persistence implementing IDeltaStore
 *   Example of storing world state changes (surface deltas, fractures, transforms) to disk
 * 
 * - SpecPackLoader: JSON-based spec loading for runtime-first architecture
 *   Example of loading SurfaceSpec, MediumSpec, BiomeSpec, etc. from JSON files
 * 
 * - Game-specific systems and initialization patterns
 * 
 * Architecture:
 *   UETPFCore (foundation) → SinglePlayerStoryTemplate (story game template)
 *                         → [Your Game Module Here]
 * 
 * Data Flow:
 *   SpecPacks/*.json → SpecPackLoader → Runtime Registries → Subsystems
 *   World Changes → IDeltaStore → FileDeltaStore → SaveData/*.json
 */
class FSinglePlayerStoryTemplate : public IModuleInterface
{
public:
	static inline FSinglePlayerStoryTemplate& Get()
	{
		return FModuleManager::LoadModuleChecked<FSinglePlayerStoryTemplate>("SinglePlayerStoryTemplate");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("SinglePlayerStoryTemplate");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
