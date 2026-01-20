// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "GlobalAtmosphereField.h"

//=============================================================================
// UAtmosphereConfig
//=============================================================================

UAtmosphereConfig::UAtmosphereConfig()
{
	DisplayName = FText::FromString(TEXT("Earth Standard"));
}

//=============================================================================
// UGlobalAtmosphereField
//=============================================================================

UGlobalAtmosphereField::UGlobalAtmosphereField()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Initialize random stream with a consistent seed for reproducibility
	GustRandomStream.Initialize(12345);
}

FAtmosphereState UGlobalAtmosphereField::GetAtmosphereAtLocation(const FVector& WorldLocation) const
{
	FAtmosphereState State;

	if (!AtmosphereConfig)
	{
		// Return default Earth-like values if no config
		return State;
	}

	// Calculate altitude (in cm, relative to sea level)
	const float AltitudeCm = WorldLocation.Z - AtmosphereConfig->SeaLevelAltitude;
	State.Altitude = AltitudeCm;

	// Check if above atmosphere top
	if (AltitudeCm >= AtmosphereConfig->AtmosphereTopAltitude)
	{
		// Vacuum
		State.Pressure = 0.0f;
		State.Density = 0.0f;
		State.Temperature = 2.7f; // Cosmic background temperature
		State.SpeedOfSound = 0.0f;
		State.WindVelocity = FVector::ZeroVector;
		State.Humidity = 0.0f;
		State.bIsVacuum = true;
		return State;
	}

	// Calculate atmospheric properties
	State.Temperature = GetTemperatureAtAltitude(AltitudeCm);
	State.Pressure = GetPressureAtAltitude(AltitudeCm);
	State.Density = GetDensityAtAltitude(AltitudeCm);
	State.SpeedOfSound = GetSpeedOfSoundAtAltitude(AltitudeCm);
	State.WindVelocity = GetWindAtLocation(WorldLocation);
	
	// Simple humidity model - decreases with altitude
	const float AltitudeM = AltitudeCm / 100.0f;
	State.Humidity = FMath::Clamp(1.0f - (AltitudeM / 10000.0f), 0.0f, 1.0f) * 0.5f;

	// Check vacuum threshold
	State.bIsVacuum = State.Density < AtmosphereConfig->VacuumDensityThreshold;

	return State;
}

float UGlobalAtmosphereField::GetPressureAtAltitude(float AltitudeCm) const
{
	if (!AtmosphereConfig)
	{
		return 101.325f; // Default sea level pressure
	}

	// Barometric formula: P = P0 * exp(-h/H)
	// where H is the scale height
	const float AltitudeM = AltitudeCm / 100.0f;
	
	// Clamp to valid range
	if (AltitudeM <= 0.0f)
	{
		return AtmosphereConfig->SeaLevelPressure;
	}

	const float Exponent = -AltitudeM / AtmosphereConfig->PressureScaleHeight;
	float Pressure = AtmosphereConfig->SeaLevelPressure * FMath::Exp(Exponent);

	return FMath::Max(Pressure, 0.0f);
}

float UGlobalAtmosphereField::GetDensityAtAltitude(float AltitudeCm) const
{
	if (!AtmosphereConfig)
	{
		return 1.225f; // Default sea level density
	}

	// TODO: implement Ideal gas law: ρ = P / (R * T)
	// But for simplicity, we use the same exponential decay as pressure
	// (which is accurate for isothermal atmosphere)
	
	const float AltitudeM = AltitudeCm / 100.0f;
	
	if (AltitudeM <= 0.0f)
	{
		return AtmosphereConfig->SeaLevelDensity;
	}

	// For a more accurate model, account for temperature lapse
	const float Temperature = GetTemperatureAtAltitude(AltitudeCm);
	const float Pressure = GetPressureAtAltitude(AltitudeCm);
	
	// ρ = P * 1000 / (R * T)  (P in kPa, need to convert to Pa)
	float Density = (Pressure * 1000.0f) / (AtmosphereConfig->SpecificGasConstant * Temperature);

	return FMath::Max(Density, 0.0f);
}

float UGlobalAtmosphereField::GetTemperatureAtAltitude(float AltitudeCm) const
{
	if (!AtmosphereConfig)
	{
		return 288.15f; // Default sea level temperature
	}

	// Linear lapse rate model: T = T0 - L * h
	// where L is lapse rate (K per 100m)
	const float AltitudeM = AltitudeCm / 100.0f;
	
	if (AltitudeM <= 0.0f)
	{
		return AtmosphereConfig->SeaLevelTemperature;
	}

	// Lapse rate is per 100m, so convert
	const float LapseRatePerMeter = AtmosphereConfig->TemperatureLapseRate / 100.0f;
	float Temperature = AtmosphereConfig->SeaLevelTemperature - (LapseRatePerMeter * AltitudeM);

	// In reality, temperature stops decreasing around tropopause (~11km)
	// and even increases in stratosphere. TODO: Implement for accuracy.
    // for now, for simplicity, clamp at minimum.
	const float MinTemperature = 180.0f; // Approximate tropopause minimum
	Temperature = FMath::Max(Temperature, MinTemperature);

	return Temperature;
}

float UGlobalAtmosphereField::GetSpeedOfSoundAtAltitude(float AltitudeCm) const
{
	if (!AtmosphereConfig)
	{
		return 343.0f; // Default sea level speed of sound
	}

	// Speed of sound: c = sqrt(γ * R * T)
	const float Temperature = GetTemperatureAtAltitude(AltitudeCm);
	
	// Check for near-vacuum conditions
	const float Density = GetDensityAtAltitude(AltitudeCm);
	if (Density < AtmosphereConfig->VacuumDensityThreshold)
	{
		return 0.0f; // No sound propagation in vacuum
	}

	const float SpeedOfSound = FMath::Sqrt(
		AtmosphereConfig->HeatCapacityRatio * 
		AtmosphereConfig->SpecificGasConstant * 
		Temperature
	);

	return SpeedOfSound;
}

FVector UGlobalAtmosphereField::GetWindAtLocation(const FVector& WorldLocation) const
{
	if (!AtmosphereConfig)
	{
		return FVector::ZeroVector;
	}

	// Start with base wind
	FVector Wind = AtmosphereConfig->BaseWindVelocity;

	// Scale with altitude
	const float AltitudeCm = WorldLocation.Z - AtmosphereConfig->SeaLevelAltitude;
	const float AltitudeM = FMath::Max(AltitudeCm / 100.0f, 0.0f);
	
	// Wind typically increases with altitude (logarithmic profile simplified to linear)
	const float AltitudeMultiplier = 1.0f + (AltitudeM / 1000.0f) * AtmosphereConfig->WindAltitudeScale;
	Wind *= AltitudeMultiplier;

	// Add gust noise
	if (AtmosphereConfig->WindGustStrength > 0.0f)
	{
		Wind += CalculateGustNoise(WorldLocation);
	}

	return Wind;
}

bool UGlobalAtmosphereField::IsVacuumAtAltitude(float AltitudeCm) const
{
	if (!AtmosphereConfig)
	{
		return false;
	}

	const float Density = GetDensityAtAltitude(AltitudeCm);
	return Density < AtmosphereConfig->VacuumDensityThreshold;
}

FEnvironmentContext UGlobalAtmosphereField::CreateEnvironmentContextAtLocation(const FVector& WorldLocation) const
{
	FEnvironmentContext Context;

	FAtmosphereState AtmoState = GetAtmosphereAtLocation(WorldLocation);

	// Map atmosphere state to environment context
	Context.Density = AtmoState.Density;
	Context.Pressure = AtmoState.Pressure;
	Context.Temperature = AtmoState.Temperature;
	Context.SpeedOfSound = AtmoState.SpeedOfSound;
	Context.WindVelocity = AtmoState.WindVelocity;
	
	// Default gravity (can be overridden by MediumSpec if needed)
	Context.Gravity = FVector(0.0f, 0.0f, -980.0f);
	
	// Sound attenuation based on density
	if (AtmoState.bIsVacuum)
	{
		Context.SoundAttenuation = 0.0f;
	}
	else
	{
		// Relative to sea level density
		const float DensityRatio = AtmoState.Density / 1.225f;
		Context.SoundAttenuation = FMath::Sqrt(FMath::Clamp(DensityRatio, 0.0f, 1.0f));
	}

	Context.bIsValid = true;

	return Context;
}

//=============================================================================
// INTERNAL
//=============================================================================

float UGlobalAtmosphereField::WorldZToAltitudeMeters(float WorldZ) const
{
	if (!AtmosphereConfig)
	{
		return WorldZ / 100.0f;
	}

	return (WorldZ - AtmosphereConfig->SeaLevelAltitude) / 100.0f;
}

FVector UGlobalAtmosphereField::CalculateGustNoise(const FVector& Location) const
{
	if (!AtmosphereConfig || AtmosphereConfig->WindGustStrength <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	// Use location-based noise for spatial coherence
	// This creates a simple Perlin-like pattern using sine waves
	const float Scale = 0.001f; // Spatial frequency
	
	float NoiseX = FMath::Sin(Location.X * Scale) * FMath::Cos(Location.Y * Scale * 1.3f);
	float NoiseY = FMath::Sin(Location.Y * Scale * 0.7f) * FMath::Cos(Location.X * Scale);
	float NoiseZ = FMath::Sin((Location.X + Location.Y) * Scale * 0.5f);

	// Get base wind magnitude for scaling gusts
	float BaseWindMagnitude = AtmosphereConfig->BaseWindVelocity.Size();
	if (BaseWindMagnitude < 1.0f)
	{
		BaseWindMagnitude = 100.0f; // Default gust magnitude if no base wind
	}

	FVector Gust(NoiseX, NoiseY, NoiseZ * 0.3f); // Less vertical variation
	Gust *= BaseWindMagnitude * AtmosphereConfig->WindGustStrength;

	return Gust;
}
