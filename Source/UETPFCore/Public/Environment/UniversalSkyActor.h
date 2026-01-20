// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Universal Sky Actor - Unified Sky Rendering System
 * 
 * Purpose:
 * Provides a unified sky rig that can be driven by the runtime spec system.
 * Supports Earth-like atmospheres, alien planets, space, and dynamic weather.
 * 
 * Architecture:
 * - Owns all UE sky components (DirectionalLight, SkyAtmosphere, Clouds, etc.)
 * - Driven by FRuntimeMediumSpec (atmosphere properties) + FRuntimeWeatherState
 * - Subscribes to TimeSubsystem for automatic sun/star updates
 * - Integrates with SolarSystemSubsystem for astronomical accuracy
 * 
 * Component Roles:
 * - SunLight: Directional light for sun (intensity driven by medium spec)
 * - SkyAtmosphere: UE's physical sky (Rayleigh/Mie scattering)
 * - VolumetricCloud: Dynamic cloud rendering
 * - SkyLight: Ambient skylight (driven by atmosphere)
 * - HeightFog: Exponential fog (driven by weather + medium)
 * - StarfieldComponent: Niagara-based star rendering (HYG catalog)
 * - PostProcess: Optional color grading for alien worlds
 * 
 * Usage:
 * 1. Place AUniversalSkyActor in level
 * 2. Configure StarfieldNiagaraSystem (or leave default)
 * 3. ApplyEnvironment() will be called automatically by subsystems
 * 4. Or manually: SkyActor->ApplyEnvironment(MediumSpec, WeatherState)
 * 
 * Integration:
 * - EnvironmentSubsystem: Provides MediumSpec at player location
 * - SolarSystemSubsystem: Provides sun direction, moon phase
 * - TimeSubsystem: Triggers updates via OnTimeAdvanced delegate
 * - StarCatalogSubsystem: Provides star data for starfield rendering
 * 
 * Performance:
 * - Starfield update rate configurable (StarfieldUpdateRateHz)
 * - Most components update only when environment changes
 * - Niagara bounds configured for large-scale scenes
 * 
 * @see FRuntimeMediumSpec for atmosphere configuration
 * @see FRuntimeWeatherState for weather parameters
 * @see USolarSystemSubsystem for sun/moon calculations
 */

#pragma once

#include "SpecTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyContext.h"
#include "UniversalSkyActor.generated.h"

// Forward declares for engine components
class USceneComponent;
class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UNiagaraComponent;
class UNiagaraSystem;

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
 * Universal sky rig actor that can be driven by your runtime spec system.
 * Earth-first, extensible to other planets by swapping MediumSpec + WeatherState.
 */
UCLASS(BlueprintType)
class UETPFCORE_API AUniversalSkyActor : public AActor
{
	GENERATED_BODY()

public:
	AUniversalSkyActor();

	// --- Components (built-in UE) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UDirectionalLightComponent> SunLight = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<USkyLightComponent> SkyLight = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UVolumetricCloudComponent> VolumetricCloud = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UPostProcessComponent> PostProcess = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UNiagaraComponent> StarfieldComponent = nullptr;

	/** Niagara system for starfield rendering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Starfield")
	TObjectPtr<UNiagaraSystem> StarfieldNiagaraSystem = nullptr;

	/**
	 * Render radius (in centimeters) for placing the star sprites on a sphere.
	 * This is a *rendering* control, not a physics distance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Starfield", meta=(ClampMin="1000.0"))
	float StarSphereRadiusCm = 1000000.0f; // default 10 km

	/** Maximum magnitude to render (dimmer stars culled). Default 6.0 = naked eye limit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Starfield", meta=(ClampMin="0.0", ClampMax="10.0"))
	float MaxVisibleMagnitude = 6.0f;

	/** Reference celestial body for sun direction (affects physics and sky). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Solar")
	ECelestialBodyId ReferenceBody = ECelestialBodyId::Earth;

	// --- Tuning knobs (gameplay-friendly defaults; you can adjust in BP) ---
	/** Update rate for starfield rotation (Hz). Lower = better performance. 0 = every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky|Runtime", meta=(ClampMin="0.0", ClampMax="60.0"))
	float StarfieldUpdateRateHz = 0.0f;

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
	 * This is the main entry point for updating the sky. Call this when:
	 * - Player moves to new biome/altitude
	 * - Weather changes
	 * - Time of day changes (automatic via TimeSubsystem)
	 * - Traveling between planets/worlds
	 * 
	 * What it does:
	 * 1. Updates sun light (direction, intensity, color from Medium.SolarIrradiance)
	 * 2. Configures atmosphere (scattering from Medium density/pressure)
	 * 3. Updates clouds (coverage from Weather.CloudCover01)
	 * 4. Configures fog (density from Weather + Medium)
	 * 5. Updates sky light (ambient from atmosphere)
	 * 6. Triggers starfield update if needed
	 * 
	 * @param Medium - Atmosphere properties (density, pressure, temperature, solar direction)
	 * @param Weather - Weather state (clouds, fog, precipitation, wind)
	 * 
	 * @note Caches state to avoid redundant updates
	 * @note Safe to call every frame, but typically only needed on environment change
	 * @note Subsystems call this automatically - manual calls usually unnecessary
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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float StarfieldUpdateAccumulator = 0.0f;
	FDelegateHandle TimeAdvancedHandle;

	// Starfield caching (prevents re-pushing arrays + reinitializing every ApplyEnvironment call)
	bool bStarfieldInitialized = false;
	int32 CachedStarCount = 0;

	// Event handlers
	void OnTimeAdvanced(double NewSimTimeSeconds);

	// Helpers
	void ConfigureDefaults();
	void ApplySun(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyAtmosphere(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyFog(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplyClouds(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
	void ApplySkyLight(const FRuntimeMediumSpec& Medium, const FRuntimeWeatherState& Weather);
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
