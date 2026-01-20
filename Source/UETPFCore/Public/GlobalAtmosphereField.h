// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "SpecTypes.h"
#include "GlobalAtmosphereField.generated.h"

/**
 * Output struct for atmosphere field queries.
 * Contains all computed atmospheric properties at a point.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FAtmosphereState
{
	GENERATED_BODY()

	/** Pressure in kPa */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	float Pressure = 101.325f;

	/** Density in kg/m³ */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	float Density = 1.225f;

	/** Temperature in Kelvin */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	float Temperature = 288.15f;

	/** Speed of sound in m/s */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	float SpeedOfSound = 343.0f;

	/** Wind velocity in cm/s */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	FVector WindVelocity = FVector::ZeroVector;

	/** Humidity (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	float Humidity = 0.5f;

	/** Whether this is effectively vacuum */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	bool bIsVacuum = false;

	/** Altitude above sea level (cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	float Altitude = 0.0f;

	FAtmosphereState() = default;
};

/**
 * Global atmosphere field configuration.
 * Defines the atmospheric model parameters for a planet/world.
 */
UCLASS(BlueprintType)
class UETPFCORE_API UAtmosphereConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UAtmosphereConfig();

	/** Display name for this atmosphere */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	//--- Sea Level Reference ---
	
	/** Sea level altitude (Z coordinate in world units, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reference")
	float SeaLevelAltitude = 0.0f;

	/** Pressure at sea level (kPa) - Earth: 101.325 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reference", meta = (ClampMin = "0.0"))
	float SeaLevelPressure = 101.325f;

	/** Temperature at sea level (Kelvin) - Earth: 288.15 (15°C) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reference", meta = (ClampMin = "0.0"))
	float SeaLevelTemperature = 288.15f;

	/** Density at sea level (kg/m³) - Earth: 1.225 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reference", meta = (ClampMin = "0.0"))
	float SeaLevelDensity = 1.225f;

	//--- Lapse Rates (how properties change with altitude) ---
	
	/** Temperature lapse rate (K per 100m altitude) - Earth: ~0.65 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lapse", meta = (ClampMin = "0.0"))
	float TemperatureLapseRate = 0.65f;

	/** Pressure scale height (m) - altitude at which pressure drops to 1/e - Earth: ~8500 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lapse", meta = (ClampMin = "100.0"))
	float PressureScaleHeight = 8500.0f;

	//--- Atmosphere Bounds ---
	
	/** Altitude above which atmosphere becomes vacuum (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounds")
	float AtmosphereTopAltitude = 10000000.0f; // 100km in cm

	/** Minimum density below which considered vacuum (kg/m³) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounds", meta = (ClampMin = "0.0"))
	float VacuumDensityThreshold = 0.00001f;

	//--- Gas Properties ---
	
	/** Specific gas constant for this atmosphere (J/(kg·K)) - Air: 287.05 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gas", meta = (ClampMin = "1.0"))
	float SpecificGasConstant = 287.05f;

	/** Ratio of specific heats (Cp/Cv) - Air: 1.4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gas", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float HeatCapacityRatio = 1.4f;

	/** Is this atmosphere breathable? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	bool bIsBreathable = true;

	//--- Wind Model ---
	
	/** Base wind velocity at sea level (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector BaseWindVelocity = FVector::ZeroVector;

	/** Wind speed multiplier vs altitude (higher = stronger winds at altitude) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float WindAltitudeScale = 1.0f;

	/** Wind gust strength (random variation, 0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindGustStrength = 0.1f;
};

/**
 * Global atmosphere field component.
 * Attach to a world settings actor or game mode to define the world's atmosphere.
 * 
 * This provides the "continuous field" for medium queries - cheap, altitude-based
 * atmospheric simulation that scales to planetary distances.
 * 
 * Local volumes (EnvironmentVolumeComponent) override this where needed for
 * caves, habitats, underwater, vacuum chambers, etc.
 */
UCLASS(ClassGroup = "Environment", meta = (BlueprintSpawnableComponent))
class UETPFCORE_API UGlobalAtmosphereField : public UActorComponent
{
	GENERATED_BODY()

public:
	UGlobalAtmosphereField();

	//--- Configuration ---
	
	/** Atmosphere configuration to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	TObjectPtr<UAtmosphereConfig> AtmosphereConfig;

	//--- Runtime Queries ---
	
	/**
	 * Query atmosphere state at a world location.
	 * This is the primary query function for global atmosphere.
	 * 
	 * @param WorldLocation - Location to query (cm)
	 * @return FAtmosphereState with all atmospheric properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	FAtmosphereState GetAtmosphereAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get pressure at altitude.
	 * Uses barometric formula: P = P0 * exp(-h/H)
	 * 
	 * @param Altitude - Height above sea level (cm)
	 * @return Pressure in kPa
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	float GetPressureAtAltitude(float Altitude) const;

	/**
	 * Get density at altitude.
	 * Derived from pressure and temperature via ideal gas law.
	 * 
	 * @param Altitude - Height above sea level (cm)
	 * @return Density in kg/m³
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	float GetDensityAtAltitude(float Altitude) const;

	/**
	 * Get temperature at altitude.
	 * Uses linear lapse rate model.
	 * 
	 * @param Altitude - Height above sea level (cm)
	 * @return Temperature in Kelvin
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	float GetTemperatureAtAltitude(float Altitude) const;

	/**
	 * Get speed of sound at altitude.
	 * c = sqrt(γ * R * T)
	 * 
	 * @param Altitude - Height above sea level (cm)
	 * @return Speed of sound in m/s
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	float GetSpeedOfSoundAtAltitude(float Altitude) const;

	/**
	 * Get wind velocity at location.
	 * Includes base wind + altitude scaling + gust noise.
	 * 
	 * @param WorldLocation - Location to query (cm)
	 * @return Wind velocity in cm/s
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	FVector GetWindAtLocation(const FVector& WorldLocation) const;

	/**
	 * Check if altitude is in vacuum.
	 * 
	 * @param Altitude - Height above sea level (cm)
	 * @return True if density is below vacuum threshold
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	bool IsVacuumAtAltitude(float Altitude) const;

	/**
	 * Convert a MediumSpecId to use this atmosphere's computed values.
	 * Useful for blending global field with volume-based medium specs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	FEnvironmentContext CreateEnvironmentContextAtLocation(const FVector& WorldLocation) const;

protected:
	/** Convert world Z to altitude above sea level (in meters for calculations) */
	float WorldZToAltitudeMeters(float WorldZ) const;

	/** Calculate wind gust noise at a location */
	FVector CalculateGustNoise(const FVector& Location) const;

	/** Cached random stream for wind gusts */
	mutable FRandomStream GustRandomStream;
};
