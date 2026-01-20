// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

// Core Types - Spec IDs, Data Assets, Runtime Structs
#include "SpecTypes.h"

// Delta Types - Sparse Delta persistence
#include "DeltaTypes.h"

// Global Atmosphere Field - altitude-based medium model
#include "GlobalAtmosphereField.h"

// Subsystems
#include "Subsystems/SurfaceQuerySubsystem.h"
#include "Subsystems/EnvironmentSubsystem.h"
#include "Subsystems/PhysicsIntegrationSubsystem.h"
#include "Subsystems/BiomeSubsystem.h"

/**
 * UETPFCore (Unreal Engine Threaded Physics Factory Core) Module
 * 
 * Provides the foundational systems for physics-driven world simulation:
 * 
 * - SurfaceSpec / SurfaceState: Contact material behavior (friction, deformation, FX)
 * - MediumSpec / EnvironmentContext: Atmosphere/fluid environments (drag, buoyancy, sound)
 * - DamageSpec: Destruction and fracture behavior
 * - FXProfile: Sound/VFX mapping
 * 
 * - SurfaceQuerySubsystem: Query surface state at any location
 * - EnvironmentSubsystem: Query environmental context (density, pressure, gravity)
 * 
 * - Delta Types: Sparse delta persistence for world state changes
 *   - Surface deltas (snow, wetness, compaction)
 *   - Fracture deltas (destruction state)
 *   - Transform deltas (settled objects)
 *   - Assembly deltas (machine/vehicle state)
 * 
 * Usage:
 *   1. Create DataAssets for your specs (DA_SurfaceSpec_*, DA_MediumSpec_*, etc.)
 *   2. Register them with the appropriate subsystems
 *   3. Query SurfaceState and EnvironmentContext at runtime
 *   4. Use deltas to persist world changes
 */
class FUETPFCore : public IModuleInterface
{
public:
	static inline FUETPFCore& Get()
	{
		return FModuleManager::LoadModuleChecked<FUETPFCore>("UETPFCore");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UETPFCore");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
