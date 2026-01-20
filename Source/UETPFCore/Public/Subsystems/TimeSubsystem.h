// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Time Subsystem - Centralized Simulation Time Management
 * 
 * Architecture:
 * - UTimeSubsystem (GameInstance): Authority for simulation time
 * - UTimeWorldBridgeSubsystem (World): Per-world tick distributor
 * 
 * Key Features:
 * - Unified clock across all loaded worlds
 * - Pause/resume without affecting engine time
 * - Time scale for fast-forward/slow-motion
 * - Fixed timestep mode for deterministic physics
 * - Negative time scale support (with bAllowNegativeTimeScale)
 * 
 * Usage:
 *   UTimeSubsystem* Time = GameInstance->GetSubsystem<UTimeSubsystem>();
 *   Time->SetTimeScale(2.0); // 2x speed
 *   double SimTime = Time->GetSimTimeSeconds();
 * 
 * Integration:
 * - Physics: Use fixed timestep mode for deterministic simulations
 * - Astronomy: SimTimeSeconds drives SolarSystemSubsystem
 * - Networking: Synchronize SimTimeSeconds for multiplayer
 * 
 * @see UTimeWorldBridgeSubsystem for per-world integration
 * @see USolarSystemSubsystem for time-driven astronomy
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimeSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSimTimeAdvanced, double);

UENUM(BlueprintType)
enum class ESimClockMode : uint8
{
	RealTime,      // advance by real delta
	FixedStep      // advance by fixed dt (deterministic)
};

/**
 * Simulation clock mode - determines how time advances.
 */
UENUM(BlueprintType)
enum class ESimClockMode : uint8
{
	/** Advance by scaled real delta time (variable timestep) */
	RealTime,
	/** Advance by fixed timestep (deterministic, suitable for networked physics) */
	FixedStep
};

/**
 * Game-wide simulation time subsystem.
 * 
 * Lifecycle:
 * 1. Initialize() - Called on GameInstance creation
 * 2. Advance() - Called each frame by TimeWorldBridgeSubsystem
 * 3. OnSimTimeAdvanced broadcast - Systems react to time change
 * 4. Deinitialize() - Cleanup on shutdown
 * 
 * Dependencies:
 * - None (base subsystem, other systems depend on this)
 * 
 * Dependent Systems:
 * - UTimeWorldBridgeSubsystem: Propagates time to world-level systems
 * - USolarSystemSubsystem: Uses SimTimeSeconds for astronomy
 * - Any system needing deterministic time
 * 
 * Thread Safety:
 * - All operations are game thread only
 * - SimTimeSeconds is read-only outside of Advance()
 * 
 * Deterministic Mode:
 * - Use FixedStep mode for networked/replayed gameplay
 * - Fixed timestep accumulates real delta and steps in discrete intervals
 * - Prevents floating point drift over long sessions
 * 
 * @note Manages a central clock with pause, time scale, and deterministic fixed-step modes
 * @note All worlds tick from this unified time source via TimeWorldBridgeSubsystem
 */
UCLASS()
class UETPFCORE_API UTimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// UGameInstanceSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Control ----
	
	/**
	 * Pause or resume simulation time.
	 * 
	 * @param bInPaused - true to pause, false to resume
	 * 
	 * @note Does not affect engine/real time, only simulation time
	 * @note Useful for pause menus, cutscenes, etc.
	 */
	UFUNCTION(BlueprintCallable, Category="Time")
	void SetPaused(bool bInPaused);

	/**
	 * Set time scale multiplier for fast-forward or slow-motion.
	 * 
	 * @param InTimeScale - Multiplier for time advancement (0.5 = half speed, 2.0 = double speed)
	 * 
	 * @note Values < 0 require bAllowNegativeTimeScale = true
	 * @note TimeScale = 0 is equivalent to pause
	 * @note Clamped to reasonable ranges internally
	 */
	UFUNCTION(BlueprintCallable, Category="Time")
	void SetTimeScale(double InTimeScale);

	/**
	 * Switch between real-time and fixed-step clock modes.
	 * 
	 * @param InMode - ESimClockMode::RealTime or ESimClockMode::FixedStep
	 * 
	 * @note FixedStep mode is deterministic and suitable for networked physics
	 * @note RealTime mode gives smoother visuals but variable timesteps
	 */
	UFUNCTION(BlueprintCallable, Category="Time")
	void SetClockMode(ESimClockMode InMode);

	/**
	 * Set the fixed timestep interval for deterministic mode.
	 * 
	 * @param InFixedStepSeconds - Time interval per step (e.g., 1/60 = 0.01666...)
	 * 
	 * @note Only used when ClockMode == FixedStep
	 * @note Common values: 1/30, 1/60, 1/120
	 * @note Smaller values = more accurate but more expensive
	 */
	UFUNCTION(BlueprintCallable, Category="Time")
	void SetFixedStepSeconds(double InFixedStepSeconds);

	/**
	 * Enable or disable negative time scale (rewind).
	 * 
	 * @param bAllow - true to allow negative time scale values
	 * 
	 * @note Rewind requires simulation state to be rewindable (snapshots/deltas)
	 * @note Most physics simulations are not naturally rewindable
	 * @note This is application-defined - subsystem only enforces the flag
	 */
	UFUNCTION(BlueprintCallable, Category="Time")
	void SetAllowNegativeTimeScale(bool bAllow);

	// ---- Read ----
    UFUNCTION(BlueprintPure, Category="Time")
    float GetCurrentTimeOfDayHours() const
    {
        const double Hours = FMath::Fmod(SimTimeSeconds / 3600.0, 24.0);
        return (float)(Hours < 0 ? Hours + 24.0 : Hours);
    }

	UFUNCTION(BlueprintPure, Category="Time")
	double GetSimTimeSeconds() const { return SimTimeSeconds; }

	UFUNCTION(BlueprintPure, Category="Time")
	double GetTimeScale() const { return TimeScale; }

	UFUNCTION(BlueprintPure, Category="Time")
	bool IsPaused() const { return bPaused; }

	UFUNCTION(BlueprintPure, Category="Time")
	double GetStepSeconds() const { return (ClockMode == ESimClockMode::FixedStep) ? FixedStepSeconds : LastStepSeconds; }

	// Broadcast for systems that want a single, authoritative “time advanced” signal.

	FOnSimTimeAdvanced OnSimTimeAdvanced;

	// Called by world bridge (or any system) once per frame.
	void Advance(double RealDeltaSeconds);

private:
	// Authority state
	double SimTimeSeconds = 0.0;
	double TimeScale = 1.0;
	bool bPaused = false;
	bool bAllowNegativeTimeScale = false;

	ESimClockMode ClockMode = ESimClockMode::RealTime;
	double FixedStepSeconds = 1.0 / 60.0;

	// For telemetry/debug
	double LastStepSeconds = 0.0;

	// Accumulator for fixed-step
	double Accumulator = 0.0;

	// Guards
	void ClampAndValidate();
};
