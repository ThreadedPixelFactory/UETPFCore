// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Universal Sky Actor - Data-Driven Atmospheric Manager
 * 
 * ============================================================================
 * DESIGN PHILOSOPHY: MANAGER PATTERN, NOT GOD ACTOR
 * ============================================================================
 * 
 * This actor is a MANAGER that coordinates atmospheric actors, NOT an owner
 * of atmospheric components. UE5's rendering pipeline expects:
 * 
 * - One ASkyAtmosphere actor (with USkyAtmosphereComponent)
 * - One ADirectionalLight actor (with UDirectionalLightComponent)  
 * - One ASkyLight actor (with USkyLightComponent)
 * 
 * Each component type has different registration mechanisms and the engine
 * queries them separately via GetWorld()->GetFirstXXX() patterns.
 * 
 * ============================================================================
 * ARCHITECTURE: REFERENCE-BASED COORDINATION
 * ============================================================================
 * 
 * UniversalSkyActor holds REFERENCES to separately-placed actors:
 * - SkyAtmosphereActor: Handles atmospheric scattering (Rayleigh/Mie)
 * - SunLightActor: Provides directional illumination
 * - SkyLightActor: Captures ambient skylight for GI
 * - [Optional] CloudsActor, FogActor, PostProcessVolume for weather effects
 * 
 * It DOES own the Niagara starfield component (rendered visuals only).
 * 
 * ============================================================================
 * DATA FLOW: SUBSYSTEM → MANAGER → ACTORS
 * ============================================================================
 * 
 * 1. Queries subsystems every Tick():
 *    - SolarSystemSubsystem: Sun direction, celestial mechanics
 *    - EnvironmentSubsystem: Atmosphere properties at player location
 *    - TimeSubsystem: Time of day, day/night cycle
 * 
 * 2. Processes queries into render parameters:
 *    - Converts FRuntimeMediumSpec → atmosphere scattering coefficients
 *    - Converts solar state → sun rotation + intensity + color temperature
 *    - Converts weather state → fog density, cloud coverage
 * 
 * 3. Updates referenced actors via public setters:
 *    - SkyAtmosphereComponent->SetRayleighScatteringScale(...)
 *    - DirectionalLightComponent->SetIntensity(...)
 *    - SkyLightComponent->RecaptureSky()
 * 
 * ============================================================================
 * CORRECT USAGE PATTERN (LEVEL SETUP)
 * ============================================================================
 * 
 * 1. Place Environment Light Mixer components in your level:
 *    - Drag ASkyAtmosphere into level
 *    - Drag ADirectionalLight into level (configure as Movable)
 *    - Drag ASkyLight into level (configure as Movable)
 *    - [Optional] Add VolumetricCloud, ExponentialHeightFog, PostProcessVolume
 *    NOTE: SkyLight's bRealTimeCapture is set to FALSE by UniversalSkyActor.
 *    With Lumen: Lumen traces through SkyAtmosphere directly (no capture needed).
 *    Without Lumen: Timer-based RecaptureSky at 1Hz provides ambient updates.
 * 
 * 2. Place UniversalSkyActor in level
 * 
 * 3. In UniversalSkyActor's Details panel, set references:
 *    - SkyAtmosphereActor → your placed SkyAtmosphere actor
 *    - SunLightActor → your placed DirectionalLight actor
 *    - SkyLightActor → your placed SkyLight actor
 *    - [Optional] VolumetricCloudActor, HeightFogActor, PostProcessVolume
 * 
 * 4. UniversalSkyActor will drive their properties from subsystems automatically
 * 
 * ============================================================================
 * INTEGRATION POINTS
 * ============================================================================
 * 
 * - EnvironmentSubsystem: Provides FRuntimeMediumSpec at player location
 * - SolarSystemSubsystem: Provides sun direction, moon phase, GMST rotation
 * - TimeSubsystem: Triggers updates via OnTimeAdvanced delegate
 * - StarCatalogSubsystem: Provides star data for Niagara starfield rendering
 * 
 * ============================================================================
 * PERFORMANCE CHARACTERISTICS
 * ============================================================================
 * 
 * - Starfield update rate configurable (StarfieldUpdateRateHz)
 * - Atmospheric updates only on environment/weather changes (cached)
 * - Niagara starfield renders 5000+ stars with GPU compute
 * - Subsystem queries amortized across all managers
 * 
 * @see FRuntimeMediumSpec for atmosphere configuration
 * @see FRuntimeWeatherState for weather parameters
 * @see USolarSystemSubsystem for sun/moon calculations
 * @see UEnvironmentSubsystem for atmosphere queries
 */

#pragma once

#include "SpecTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyContext.h"
#include "UniversalSkyActor.generated.h"

// Forward declares for engine components and actors
class USceneComponent;
class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UNiagaraComponent;
class UNiagaraSystem;

// Forward declares for atmospheric actors
class ADirectionalLight;
class ASkyAtmosphere;
class ASkyLight;
class AActor;  // Generic actor for fog/cloud/PP volume
class AVolumetricCloud;
class APostProcessVolume;

/**
 * Runtime weather state for sky rendering.
 * 
 * Minimal weather parameters that drive visual effects.
 * All values normalized to 0-1 range for consistent blending.
 * 
 * Design Philosophy:
 * - Start simple, expand later
 * - POD-friendly for serialization/networking
 * - Deterministic for replay systems
 * 
 * Parameters:
 * - CloudCover01: 0=clear sky, 1=overcast
 * - Fog01: 0=clear, 1=heavy fog/haze
 * - Precip01: 0=no rain, 1=heavy rain (drives particle systems)
 * - Storm01: 0=calm, 1=thunderstorm (lightning/thunder intensity)
 * - Humidity01: 0=dry, 1=saturated (affects haze/mie scattering)
 * - WindDir: Normalized direction vector for wind
 * - WindSpeed: Wind speed in m/s (for cloud animation)
 * 
 * Usage:
 *   FRuntimeWeatherState Weather;
 *   Weather.CloudCover01 = 0.7f; // Mostly cloudy
 *   Weather.Fog01 = 0.3f; // Light fog
 *   Weather.WindSpeed = 10.0f; // 10 m/s wind
 *   SkyActor->ApplyEnvironment(MediumSpec, Weather);
 * 
 * @note Values outside 0-1 are clamped internally
 * @note Future expansion may add precipitation type, storm cells, etc.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FRuntimeWeatherState
{
	GENERATED_BODY()

	/** 0..1 cloud cover */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	float CloudCover01 = 0.5f;

	/** 0..1 fog intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	float Fog01 = 0.1f;

	/** 0..1 precipitation intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	float Precip01 = 0.1f;

	/** 0..1 storm intensity (drives lightning/thunder later) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	float Storm01 = 0.1f;

	/** 0..1 humidity proxy (affects haze / mie scattering later) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	float Humidity01 = 0.4f;

	/** Wind direction (unit vector) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	FVector WindDir = FVector(1, 0, 0);

	/** Wind speed (m/s proxy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
	float WindSpeed = 5.0f;
};

/**
 * Universal sky manager actor - coordinates atmospheric actors via subsystem queries.
 * 
 * CRITICAL: This is a MANAGER, not a component owner!
 * Set actor references in the Details panel after placing actors in your level.
 */
UCLASS(BlueprintType)
class UETPFCORE_API AUniversalSkyActor : public AActor
{
	GENERATED_BODY()

public:
	AUniversalSkyActor();

	/* ============================================================================
	 * ACTOR REFERENCES - MUST BE ASSIGNED IN EDITOR
	 * ============================================================================
	 * These are references to separately-placed actors in your level.
	 * The manager DOES NOT own these actors - it only updates their properties.
	 * 
	 * SETUP WORKFLOW:
	 * 1. Place actors in level (SkyAtmosphere, DirectionalLight, SkyLight)
	 * 2. Place UniversalSkyActor
	 * 3. In UniversalSkyActor Details, assign references to your placed actors
	 * 4. Manager will automatically drive them from subsystems
	 * ============================================================================ */

	/** Reference to the SkyAtmosphere actor (handles Rayleigh/Mie scattering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|References", meta=(DisplayName="Sky Atmosphere Actor"))
	TObjectPtr<ASkyAtmosphere> SkyAtmosphereActor = nullptr;

	/** Reference to the Directional Light actor (sun) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|References", meta=(DisplayName="Sun Light Actor"))
	TObjectPtr<ADirectionalLight> SunLightActor = nullptr;

	/** Reference to the Sky Light actor (ambient GI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|References", meta=(DisplayName="Sky Light Actor"))
	TObjectPtr<ASkyLight> SkyLightActor = nullptr;

	/** [Optional] Reference to VolumetricCloud actor for dynamic weather */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|References", meta=(DisplayName="Volumetric Cloud Actor"))
	TObjectPtr<AVolumetricCloud> VolumetricCloudActor = nullptr;

	/** [Optional] Reference to ExponentialHeightFog actor for atmospheric haze */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|References", meta=(DisplayName="Height Fog Actor"))
	TObjectPtr<AActor> HeightFogActor = nullptr;

	/** [Optional] Reference to PostProcessVolume for color grading */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|References", meta=(DisplayName="Post Process Volume"))
	TObjectPtr<APostProcessVolume> PostProcessVolume = nullptr;

	/* ============================================================================
	 * OWNED COMPONENTS - STARFIELD RENDERING
	 * ============================================================================
	 * The manager DOES own the starfield component as it's purely visual
	 * and doesn't interact with UE5's atmospheric rendering pipeline.
	 * ============================================================================ */

	/** Root component for starfield attachment */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<USceneComponent> Root = nullptr;

	/** Niagara component for starfield rendering (procedurally generated) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UNiagaraComponent> StarfieldComponent = nullptr;

	/** Niagara system for starfield rendering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Starfield")
	TObjectPtr<UNiagaraSystem> StarfieldNiagaraSystem = nullptr;

	/* ============================================================================
	 * STARFIELD CONFIGURATION
	 * ============================================================================ */

	/**
	 * Render radius (in centimeters) for placing the star sprites on a sphere.
	 * This is a *rendering* control, not a physics distance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Starfield", meta=(ClampMin="1000.0"))
	float StarSphereRadiusCm = 1000000.0f; // default 10 km

	/** Maximum magnitude to render (dimmer stars culled). Default 6.0 = naked eye limit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Starfield", meta=(ClampMin="0.0", ClampMax="10.0"))
	float MaxVisibleMagnitude = 6.0f;

	/* ============================================================================
	 * CELESTIAL REFERENCE & TIMING
	 * ============================================================================ */

	/** Reference celestial body for sun direction (affects physics and sky). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Solar")
	ECelestialBodyId ReferenceBody = ECelestialBodyId::Earth;

	/** Update rate for starfield rotation (Hz). Lower = better performance. 0 = every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Runtime", meta=(ClampMin="0.0", ClampMax="60.0"))
	float StarfieldUpdateRateHz = 0.0f;

	/* ============================================================================
	 * TUNING PARAMETERS
	 * ============================================================================
	 * These scales let you artistically tune the physical simulation results
	 * without breaking the underlying data-driven model.
	 * ============================================================================ */

	/** Overall intensity scale for sun (lets you tune without touching physical proxies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Tuning", meta=(ClampMin="0.0"))
	float SunIntensityScale = 1.0f;

	/** Skylight intensity scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Tuning", meta=(ClampMin="0.0"))
	float SkyLightIntensityScale = 1.0f;

	/** Fog intensity scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Tuning", meta=(ClampMin="0.0"))
	float FogIntensityScale = 1.0f;

	/** Cloud “density” scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Tuning", meta=(ClampMin="0.0"))
	float CloudDensityScale = 1.0f;

	/** Cached state (for auto apply) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|State")
	FRuntimeMediumSpec CurrentMedium;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|State")
	FRuntimeWeatherState CurrentWeather;

	/** 
	 * Apply environment configuration to sky rendering.
	 * 
	 * MAIN ENTRY POINT for updating atmospheric actors.
	 * Called automatically by TimeSubsystem on time changes.
	 * Can also be called manually when environment changes.
	 * 
	 * Data Flow:
	 * 1. Queries subsystems (Solar, Environment, Time)
	 * 2. Converts spec data to render parameters
	 * 3. Updates referenced actors via component setters
	 * 
	 * What Gets Updated:
	 * - Sun light: Direction (from SolarSystemSubsystem), intensity, color temperature
	 * - Atmosphere: Rayleigh/Mie scattering coefficients from Medium.Density
	 * - Clouds: Coverage from Weather.CloudCover01 (if VolumetricCloudActor assigned)
	 * - Fog: Density from Weather + Medium (if HeightFogActor assigned)
	 * - Sky light: Ambient recapture (if SkyLightActor assigned)
	 * - Starfield: Rotation from GMST (Greenwich Mean Sidereal Time)
	 * 
	 * @param Medium - Atmosphere properties (density, pressure, temperature, solar irradiance)
	 * @param Weather - Weather state (clouds, fog, precipitation, wind)
	 * 
	 * @note Caches state to avoid redundant updates
	 * @note Safe to call every frame, typically only processes on actual changes
	 * @note Validates actor references before updating (safe if actors not assigned)
	 * 
	 * Example:
	 * \code{.cpp}
	 *   FRuntimeMediumSpec EarthAtmo = EnvironmentSubsystem->GetMediumSpec("Earth");
	 *   FRuntimeWeatherState ClearDay;
	 *   ClearDay.CloudCover01 = 0.2f;
	 *   SkyActor->ApplyEnvironment(EarthAtmo, ClearDay);
	 * \endcode
	 */
	UFUNCTION(BlueprintCallable, Category="Sky")
	void ApplyEnvironment(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);

	/** Get the Niagara starfield component for configuration */
	UFUNCTION(BlueprintPure, Category="Sky")
	UNiagaraComponent* GetStarfieldComponent() const { return StarfieldComponent; }

	/* ============================================================================
	 * HELPER METHODS FOR COMPONENT ACCESS
	 * ============================================================================
	 * These getters safely extract components from referenced actors.
	 * Return nullptr if actor not assigned or component not found.
	 * ============================================================================ */

	/** Get the DirectionalLightComponent from the referenced sun actor */
	UFUNCTION(BlueprintPure, Category="Sky")
	UDirectionalLightComponent* GetSunLightComponent() const;

	/** Get the SkyAtmosphereComponent from the referenced atmosphere actor */
	UFUNCTION(BlueprintPure, Category="Sky")
	USkyAtmosphereComponent* GetSkyAtmosphereComponent() const;

	/** Get the SkyLightComponent from the referenced skylight actor */
	UFUNCTION(BlueprintPure, Category="Sky")
	USkyLightComponent* GetSkyLightComponent() const;

	/** Get the VolumetricCloudComponent from the referenced cloud actor */
	UFUNCTION(BlueprintPure, Category="Sky")
	UVolumetricCloudComponent* GetVolumetricCloudComponent() const;

	/** Get the ExponentialHeightFogComponent from the referenced fog actor */
	UFUNCTION(BlueprintPure, Category="Sky")
	UExponentialHeightFogComponent* GetHeightFogComponent() const;

	/** Get the PostProcessComponent from the referenced post process volume */
	UFUNCTION(BlueprintPure, Category="Sky")
	UPostProcessComponent* GetPostProcessComponent() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

private:
	// Time sampling for dirty flag evaluation
	double LastSampledSimTime = 0.0;

	// Starfield caching (prevents re-pushing arrays + reinitializing every ApplyEnvironment call)
	bool bStarfieldInitialized = false;
	int32 CachedStarCount = 0;

	// ============================================================================
	// INSTANCE STATE FOR DIAGNOSTICS & CACHING
	// ============================================================================
	// Member variables properly reset on actor construction/destruction.
	// ============================================================================

	/** Sun update counter for diagnostic logging (first 3 only) */
	int32 SunDiagnosticCount = 0;

	/** Atmosphere update counter for diagnostic logging (first 3 only) */
	int32 AtmosDiagnosticCount = 0;

	/** Cached medium/weather for change detection */
	FRuntimeMediumSpec LastAppliedMedium;
	FRuntimeWeatherState LastAppliedWeather;
	bool bFirstEnvironmentApply = true;

	/** Scattering logged flag (one-time) */
	bool bScatteringLogged = false;

	/** Last logged density for throttling log spam */
	float LastLoggedDensity = -1.0f;

	/** Post-process logged flag (one-time) */
	bool bPostProcessLogged = false;

	// NOTE: Tick interval is set in constructor (PrimaryActorTick.TickInterval = 0.1f)
	// No timers used - minimal tick architecture avoids timer queue accumulation issues

	// Dirty flags for component updates (prevents redundant render state changes)
	// IMPORTANT: Default to FALSE to respect editor-placed actor settings on first frame.
	// Real environmental/time changes from subsystems will set these true when needed.
	bool bSunDirty = true;  // Sun direction needs immediate update from subsystem
	bool bAtmosphereDirty = false;  // Respect placed SkyAtmosphere settings
	bool bFogDirty = false;  // Respect placed fog settings
	bool bCloudsDirty = false;  // Respect placed cloud settings
	bool bSkyLightDirty = true;  // SkyLight recapture needed after other setup
	bool bPostProcessDirty = false;  // Respect placed PostProcess settings

	// Cached values for change detection
	FVector CachedSunDirection = FVector::ZeroVector;
	float CachedSunIntensity = -1.0f;
	float CachedAtmosphereDensity = -1.0f;
	float CachedCloudCover = -1.0f;
	float CachedFogIntensity = -1.0f;

	// Initialization phase methods (called from BeginPlay)
	bool ValidateRequiredReferences();
	void InitializeAtmosphericComponents();
	void RegisterComponentsWithRenderer();
	void SubscribeToSubsystems();

	// Helper methods for updating referenced actors
	// NOTE: These methods validate actor references before accessing components
	void ApplySun(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyAtmosphere(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyFog(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyClouds(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplySkyLight(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyPostProcess(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyStarfield();
	void UpdateStarfieldRotation();

	/** Convert B-V color index to RGB color for star rendering */
	FLinearColor BVIndexToColor(float BV) const;

public:
	/**
	 * Configure Niagara component bounds for large-scale scenes
	 * @param BoundsRadiusCm Radius in centimeters for the fixed bounds sphere
	 */
	void SetStarfieldBounds(float BoundsRadiusCm);
};
