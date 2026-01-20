// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "CoreMinimal.h"

/**
 * Canonical astronomy units + conversions.
 * Keep celestial math in canonical frames, convert to Unreal at the edge.
 */
namespace CelestialUnits
{
	static constexpr double KmToCm = 100000.0;          // 1 km = 100,000 cm
	static constexpr double AuToKm = 149597870.7;       // exact-ish
	static constexpr double TwoPi = 6.2831853071795864769;

	FORCEINLINE double DegToRad(const double Deg) { return Deg * (PI / 180.0); }
	FORCEINLINE double HoursToRad(const double Hours) { return Hours * (PI / 12.0); } // 24h -> 2π

	/**
	 * Convert RA(hours), Dec(degrees) to a unit direction in equatorial J2000 frame.
	 * Frame: +X vernal equinox, +Z north celestial pole, +Y RA=6h.
	 */
	FORCEINLINE FVector3d DirFromRaDecHoursDeg(const double RaHours, const double DecDeg)
	{
		const double Ra = HoursToRad(RaHours);
		const double Dec = DegToRad(DecDeg);

		const double CosDec = FMath::Cos(Dec);
		return FVector3d(
			CosDec * FMath::Cos(Ra),
			CosDec * FMath::Sin(Ra),
			FMath::Sin(Dec)
		);
	}
}
