// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Solar System Subsystem - Astronomical Calculations for Sky Rendering
 * 
 * Purpose:
 * Provides cheap, stable, deterministic astronomy outputs for gameplay and rendering:
 * - Sun direction (day/night cycles, seasons)
 * - Moon orbit and phase (lighting, tides)
 * - GMST (Greenwich Mean Sidereal Time) for starfield rotation
 * 
 * Coordinate System:
 * - Canonical frame: ECI-ish (Earth-Centered Inertial)
 * - Earth at origin (0,0,0)
 * - Moon in simple circular orbit
 * - Sun direction computed from time
 * - Units: kilometers for positions, km/s for velocities
 * 
 * Time Anchoring:
 * Two modes controlled by bUseUnixEpochTime:
 * 1. Unix Epoch Mode (bUseUnixEpochTime=true):
 *    - TimeSubsystem::SimTimeSeconds interpreted as Unix seconds (UTC-ish)
 *    - Directly maps to real-world dates (time to build that time machine :p)
 *    
 * 2. Game Epoch Mode (bUseUnixEpochTime=false):
 *    - SimTimeSeconds=0 anchored to GameEpochUnixSeconds
 *    - Allows fictional timelines while preserving astronomy
 * 
 * Accuracy:
 * - NOT a full ephemeris - simplified models for performance
 * - Accurate enough for visual/gameplay purposes (< 1° error)
 * - Can be swapped with higher-accuracy library without changing API
 * 
 * Integration:
 * - Driven by UTimeSubsystem::SimTimeSeconds
 * - Consumed by UWorldFrameSubsystem for per-world anchoring
 * - Consumed by AUniversalSkyActor for sky rendering
 * - Can be extended for orbital mechanics gameplay
 * 
 * @see UTimeSubsystem for simulation time management
 * @see UWorldFrameSubsystem for coordinate transform to world space
 * @see UCelestialMathLibrary for pure math utilities
 */

#pragma once

#include "CoreMinimal.h"
#include "Environment/SkyContext.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SolarSystemSubsystem.generated.h"


/**
 * Static definition for a celestial body (design-time constants).
 * 
 * Defines immutable physical properties of a celestial body.
 * Units are explicit to prevent confusion:
 * - RadiusKm: Planet/moon radius in kilometers
 * 
 * Usage:
 *   FCelestialBodyDef EarthDef = SolarSystem->GetBodyDef(ECelestialBodyId::Earth);
 *   float RadiusKm = EarthDef.RadiusKm; // 6371 km
 * 
 * @note These are simplified values suitable for rendering, not full ephemeris data
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FCelestialBodyDef
{
	GENERATED_BODY()

	/** Unique identifier for this celestial body */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	ECelestialBodyId Id = ECelestialBodyId::Earth;

	/** Radius in kilometers (Earth: 6371 km, Moon: 1737 km) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	double RadiusKm = 1.0;

	/** Does this body have an atmosphere? (affects rendering) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	bool bHasAtmosphere = false;

	/** Does this body have clouds? (affects rendering) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	bool bHasClouds = false;
};

/**
 * Dynamic state of a celestial body in canonical ECI-ish frame.
 * 
 * Represents instantaneous position and velocity at current simulation time.
 * Units are explicit:
 * - PositionKm: Position vector in kilometers from origin
 * - VelocityKmS: Velocity vector in kilometers per second
 * 
 * Coordinate Frame:
 * - Origin: Typically Earth center (ECI-ish)
 * - X: Vernal equinox direction
 * - Y: 90° east in equatorial plane
 * - Z: North celestial pole
 * 
 * Usage:
 *   FCelestialBodyState MoonState = SolarSystem->GetBodyState(ECelestialBodyId::Moon);
 *   FVector MoonPosKm = MoonState.PositionKm; // Position relative to Earth
 * 
 * @note FVector used instead of FVector3d for Blueprint compatibility (automatic conversion)
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FCelestialBodyState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="State")
	FVector PositionKm = FVector::ZeroVector;  // Changed to FVector for Blueprint compatibility

	UPROPERTY(BlueprintReadOnly, Category="State")
	FVector VelocityKmS = FVector::ZeroVector;  // Changed to FVector for Blueprint compatibility
};

/**
 * Consolidated solar system state for rendering/physics.
 * Single truth for sun direction, moon phase, starfield rotation, etc.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FSolarSystemState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Solar")
	FVector SunDir_World = FVector(1,0,0);

	UPROPERTY(BlueprintReadOnly, Category="Solar")
	float SunIlluminanceLux = 100000.0f;  // Scalar for UE light units

	UPROPERTY(BlueprintReadOnly, Category="Solar")
	FVector MoonDir_World = FVector(0,1,0);

	UPROPERTY(BlueprintReadOnly, Category="Solar")
	float MoonPhase01 = 0.5f;  // 0=new, 1=full

	UPROPERTY(BlueprintReadOnly, Category="Solar")
	float LocalSiderealTimeRad = 0.0f;  // For starfield rotation

	UPROPERTY(BlueprintReadOnly, Category="Solar")
	ECelestialBodyId ActiveBody = ECelestialBodyId::Earth;
};

/**
 * Minimal world frame context for anchoring.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FWorldFrameContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Frame")
	ECelestialBodyId PrimaryBody = ECelestialBodyId::Earth;

	UPROPERTY(BlueprintReadOnly, Category="Frame")
	FVector GravityDir = FVector(0,0,-1);

	UPROPERTY(BlueprintReadOnly, Category="Frame")
	float GravityCmPerS2 = 980.0f;  // Earth gravity approx

	UPROPERTY(BlueprintReadOnly, Category="Frame")
	double AltitudeKm = 0.0;
};

/**
 * Environment state for atmosphere/clouds/weather.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FEnvironmentState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Environment")
	float TemperatureC = 20.0f;

	UPROPERTY(BlueprintReadOnly, Category="Environment")
	float PressurePa = 101325.0f;

	UPROPERTY(BlueprintReadOnly, Category="Environment")
	float Humidity01 = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category="Environment")
	float WindSpeedMS = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category="Environment")
	FVector WindDir = FVector(1,0,0);
};

/**
 * Solar System Subsystem - Game Instance Astronomy Engine
 * 
 * Lifecycle:
 * 1. Initialize() - Subscribes to TimeSubsystem, loads body definitions
 * 2. OnTimeAdvanced() - Updates celestial positions when time changes
 * 3. Query functions - GetSunDirCanonical(), GetMoonPhaseRad(), etc.
 * 4. Deinitialize() - Cleanup
 * 
 * Dependencies:
 * - UTimeSubsystem: Provides SimTimeSeconds for calculations
 * 
 * Dependent Systems:
 * - UWorldFrameSubsystem: Transforms canonical positions to world space
 * - AUniversalSkyActor: Consumes sun direction, moon phase for rendering
 * - Any system needing astronomical data
 * 
 * Usage Example (C++):
 * \code{.cpp}
 *   USolarSystemSubsystem* Solar = GameInstance->GetSubsystem<USolarSystemSubsystem>();
 *   FVector SunDir = Solar->GetSunDirCanonical(); // Unit vector to sun
 *   float MoonPhase = Solar->GetMoonPhaseRad(); // 0 = new, PI = full
 * \endcode
 * 
 * Usage Example (Blueprint):
 *   Get Game Instance -> Get Subsystem (Solar System) -> Get Sun Dir Canonical
 * 
 * Performance:
 * - Calculations are O(1) trigonometry, very cheap
 * - Results cached between time updates
 * - Safe to query every frame from multiple systems
 * 
 * Accuracy Trade-offs:
 * - Sun position: ± 0.5° (good enough for lighting)
 * - Moon position: ± 2° (good enough for visuals)
 * - Not suitable for precise orbital mechanics simulation
 * 
 * @note This is NOT a full ephemeris - optimized for visual/gameplay accuracy - can be extended if need be
 * @note The API is designed to allow swapping in higher-accuracy later without changing call sites
 */
UCLASS()
class UETPFCORE_API USolarSystemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------------- Time Anchoring ----------------
	// If true: TimeSubsystem::SimTimeSeconds is interpreted as Unix seconds (UTC-ish).
	// If false: SimTimeSeconds is seconds since "game epoch", anchored by GameEpochUnixSeconds.
	UPROPERTY(EditAnywhere, Category="Solar|Time")
	bool bUseUnixEpochTime = true;

	// If bUseUnixEpochTime=false, this anchors "SimTimeSeconds=0" to a real UTC epoch.
	UPROPERTY(EditAnywhere, Category="Solar|Time")
	double GameEpochUnixSeconds = 1704067200.0; // 2024-01-01 00:00:00 UTC

	UFUNCTION(BlueprintCallable, Category="Solar|Time")
	void SetGameEpochUnixSeconds(double InUnixSeconds) { GameEpochUnixSeconds = InUnixSeconds; }

	// ---------------- Body def/state ----------------
	UFUNCTION(BlueprintCallable, Category="Solar|Bodies")
	FCelestialBodyDef GetBodyDef(ECelestialBodyId Body) const;

	/**
	 * Returns body state in canonical ECI-ish frame.
	 * NOTE: For Earth/Moon we keep Earth at origin and Moon in simple orbit.
	 */
	UFUNCTION(BlueprintCallable, Category="Solar|Bodies")
	FCelestialBodyState GetBodyState(ECelestialBodyId Body) const;

	// ---------------- Key Outputs ----------------
	/** Sun direction in canonical frame (unit vector). */
	UFUNCTION(BlueprintCallable, Category="Solar|Outputs")
	FVector GetSunDirCanonical() const;

	/**
	 * Moon phase angle in radians:
	 * 0   = New (Moon near Sun direction)
	 * PI  = Full (Moon opposite Sun direction)
	 */
	UFUNCTION(BlueprintCallable, Category="Solar|Outputs")
	float GetMoonPhaseRad() const;

	/** Convenience: illuminated fraction [0..1]. */
	UFUNCTION(BlueprintCallable, Category="Solar|Outputs")
	double GetMoonIlluminationFraction() const;

	/**
	 * Greenwich Mean Sidereal Time angle in radians [0..2pi).
	 * Use to rotate a starfield around Earth's spin axis.
	 */
	UFUNCTION(BlueprintCallable, Category="Solar|Outputs")
	double GetGMSTAngleRad() const;

	/** Julian Date (UTC-ish) derived from sim unix seconds. */
	UFUNCTION(BlueprintCallable, Category="Solar|Outputs")
	double GetJulianDate() const;

	/** Consolidated state for rendering/physics. */
	UFUNCTION(BlueprintCallable, Category="Solar|Outputs")
	FSolarSystemState GetSolarSystemState() const;

	// ---------------- Tunables ----------------
	UPROPERTY(EditAnywhere, Category="Solar|Moon")
	float EarthMoonDistanceKm = 384400.0f;

	UPROPERTY(EditAnywhere, Category="Solar|Moon")
	float MoonOrbitalPeriodS = 27.321661f * 24.0f * 3600.0f;

	UPROPERTY(EditAnywhere, Category="Solar|Moon")
	float MoonInclinationDeg = 5.145f;

private:
	void InitDefaults();

	void OnTimeAdvanced(double NewSimTimeSeconds);

	// Returns sim time as Unix seconds (UTC-ish), regardless of how TimeSubsystem is configured.
	double GetSimUnixSeconds() const;

	// Cache update (called lazily on getter)
	void EnsureCacheUpToDate() const;

	// Helpers
	static double UnixSecondsToJulianDate(double UnixSeconds);
	static FVector3d ComputeSunDirApprox_J2000(double JulianDate);
	static double ComputeGMSTAngleRad_FromJD(double JulianDate);
	void ComputeMoonState(double SimUnixSeconds) const;

	// General phase computation:
	// PhaseAngle = acos( dot( normalize(Moon-Planet), normalize(Sun-Planet) ) ) with sign conventions.
	static float ComputePhaseAngleRad(const FVector& ToMoon, const FVector& ToSun);

private:
	TMap<ECelestialBodyId, FCelestialBodyDef> BodyDefs;

	// ---- Cached state (mutable because getters are const) ----
	mutable double CachedSimUnixSeconds = -1.0;
	mutable double CachedJulianDate = 0.0;
	mutable FVector3d CachedSunDir_D = FVector3d(1,0,0);
	mutable float CachedGMST = 0.0f;
	mutable FVector3d CachedMoonPositionKm_D = FVector3d::Zero();
	mutable FVector3d CachedMoonVelocityKmS_D = FVector3d::Zero();

	// Event subscription
	FDelegateHandle TimeAdvancedHandle;
};
