// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * UETPFCoreSim Module
 * 
 * Scientific simulation extensions for UETPFCore framework.
 * Provides validated ephemerides, deterministic physics, and
 * integration with scientific simulation tools.
 * 
 * Features:
 * - SPICE Toolkit integration for validated celestial mechanics
 * - High-precision ephemerides (planetary positions, orientations)
 * - Deterministic physics simulation mode
 * - USD import/export for Omniverse/Isaac Sim integration
 * - Reference frame transformations (ICRF, J2000, etc.)
 * 
 * Use Cases:
 * - Aerospace training simulations
 * - Robotics visualization (Mars rovers, spacecraft)
 * - Educational simulations with validated physics
 * - Research visualization and data analysis
 * 
 * NOT intended for:
 * - Publishable scientific results (use specialized tools)
 * - Real-time gameplay physics (use UETPFCore base)
 * - Production aerospace applications (use certified tools)
 * 
 * This module bridges the gap between scientific accuracy and
 * real-time visualization, providing "good enough" precision
 * for training, education, and visualization workflows.
 */
class FUETPFCoreSim : public IModuleInterface
{
public:
	static inline FUETPFCoreSim& Get()
	{
		return FModuleManager::LoadModuleChecked<FUETPFCoreSim>("UETPFCoreSim");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UETPFCoreSim");
	}

	/** Check if SPICE library is available */
	static bool IsSpiceAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool bSpiceInitialized = false;
};
