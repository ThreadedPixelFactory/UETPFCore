// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Celestial Math Library - Pure Astronomy Math Functions
 * 
 * Purpose:
 * Provides stateless, thread-safe mathematical utilities for astronomical calculations.
 * No dependencies on subsystems or world state - pure functions only.
 * 
 * Coordinate System Conventions:
 * - Equatorial J2000-ish frame for celestial coordinates:
 *   - X axis: RA 0h (vernal equinox direction)
 *   - Y axis: RA 6h (90° east in equatorial plane)
 *   - Z axis: North celestial pole (Dec +90°)
 * 
 * - Right Ascension (RA): Measured in HOURS (0-24), NOT degrees
 *   - Conversion: 1 hour RA = 15 degrees
 *   - Example: RA 6h = 90° east
 * 
 * - Declination (Dec): Measured in DEGREES (-90 to +90)
 *   - Dec 0° = celestial equator
 *   - Dec +90° = north celestial pole
 *   - Dec -90° = south celestial pole
 * 
 * Unreal Engine Mapping:
 * - Canonical celestial frame is SEPARATE from UE world space
 * - WorldFrameSubsystem handles the transform: Celestial → World
 * - This separation allows flexible world anchoring (Earth/Moon/Space)
 * 
 * Thread Safety:
 * - All functions are static and stateless
 * - Safe to call from game thread, tasks, or editor
 * - No caching or mutable state
 * 
 * Usage:
 * \code{.cpp}
 *   // Convert star coordinates to direction vector
 *   FVector StarDir = UCelestialMathLibrary::EquatorialDir_FromRaDec(6.75, -16.7); // Sirius
 *   
 *   // Convert magnitude to rendering intensity
 *   float Intensity = UCelestialMathLibrary::MagToIntensity(-1.46f); // Sirius magnitude
 * \endcode
 * 
 * @see USolarSystemSubsystem for stateful astronomy engine
 * @see UWorldFrameSubsystem for coordinate transforms
 * @see UStarCatalogSubsystem for star database
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CelestialMathLibrary.generated.h"

/**
 * Pure conversion + astronomy math helpers.
 * No state, safe anywhere (game thread, tasks, editor).
 *
 * Conventions:
 * - RA is HOURS in HYG. Dec is DEGREES.
 * - Output unit vectors are in an equatorial J2000-ish basis:
 *   X = RA 0h, Dec 0
 *   Y = RA 6h, Dec 0
 *   Z = North celestial pole
 *
 * Unreal mapping policy:
 * - Keep "canonical celestial frame" separate from world frame.
 * - WorldFrameSubsystem decides how canonical maps into UE world axes.
 */
UCLASS()
class UETPFCORE_API UCelestialMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Convert equatorial coordinates (RA, Dec) to a 3D direction vector.
	 * 
	 * Input Conventions:
	 * - RA in HOURS (0-24), NOT degrees. Common mistake: don't pass degrees!
	 * - Dec in DEGREES (-90 to +90)
	 * 
	 * Output:
	 * - Unit vector in equatorial J2000-ish frame
	 * - X: RA 0h, Dec 0° (vernal equinox)
	 * - Y: RA 6h, Dec 0° (90° east)
	 * - Z: Dec +90° (north celestial pole)
	 * 
	 * @param RaHours - Right Ascension in hours (0-24). Example: Sirius = 6.75h
	 * @param DecDegrees - Declination in degrees (-90 to +90). Example: Sirius = -16.7°
	 * @return Normalized direction vector in celestial frame
	 * 
	 * @note HYG catalog stores RA in hours, Dec in degrees
	 * @note To convert degrees to hours: RA_hours = RA_degrees / 15.0
	 * 
	 * Example:
	 * \code{.cpp}
	 *   // Sirius: RA 6h 45m = 6.75h, Dec -16° 43' = -16.7°
	 *   FVector SiriusDir = UCelestialMathLibrary::EquatorialDir_FromRaDec(6.75, -16.7);
	 * \endcode
	 */
	UFUNCTION(BlueprintPure, Category="Space|Celestial")
	static FVector EquatorialDir_FromRaDec(double RaHours, double DecDegrees);

	/** Convert parsecs to centimeters (UE default length). */
	UFUNCTION(BlueprintPure, Category="Space|Units")
	static double ParsecsToCentimeters(double Parsecs);

	/** Convert kilometers to centimeters (UE default length). */
	UFUNCTION(BlueprintPure, Category="Space|Units")
	static double KilometersToCentimeters(double Km);

	/**
	 * Convert apparent magnitude to an intensity-ish scalar.
	 * This is NOT physically correct luminance; it's a stable perceptual mapping for rendering.
	 * You can tune kExposure to match your sky material/niagara.
	 */
	UFUNCTION(BlueprintPure, Category="Space|Rendering")
	static float MagToIntensity(float ApparentMag, float kExposure = 1.0f);

	/**
	 * Approximate sidereal angle (Greenwich Mean Sidereal Time) in radians.
	 * Input: Unix-like seconds since an epoch you define. For now we assume:
	 * - SimTimeSeconds = seconds since J2000 (2000-01-01 12:00:00 TT) OR since game epoch.
	 *
	 * If your epoch differs, keep this function but define the offset in SolarSystemSubsystem.
	 * This returns an angle you can use to rotate the starfield around Earth's axis.
	 */
	UFUNCTION(BlueprintPure, Category="Space|Time")
	static double ApproxGMST_Radians(double SimTimeSeconds, double EarthSiderealDaySeconds = 86164.0905);
};
