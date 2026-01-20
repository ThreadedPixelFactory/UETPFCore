// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Space/Subsystems/CelestialMathLibrary.h"

FVector UCelestialMathLibrary::EquatorialDir_FromRaDec(double RaHours, double DecDegrees)
{
	// RA hours -> radians (24h == 2π)
	const double RaRad  = RaHours * (PI / 12.0);
	const double DecRad = FMath::DegreesToRadians(DecDegrees);

	const double CosDec = FMath::Cos(DecRad);
	const double X = CosDec * FMath::Cos(RaRad);
	const double Y = CosDec * FMath::Sin(RaRad);
	const double Z = FMath::Sin(DecRad);

	return FVector(X, Y, Z).GetSafeNormal();
}

double UCelestialMathLibrary::ParsecsToCentimeters(double Parsecs)
{
	// 1 parsec ≈ 3.085677581e16 meters
	// meters -> cm = * 100
	constexpr double MetersPerParsec = 3.085677581e16;
	return Parsecs * MetersPerParsec * 100.0;
}

double UCelestialMathLibrary::KilometersToCentimeters(double Km)
{
	// km -> m -> cm
	return Km * 1000.0 * 100.0;
}

float UCelestialMathLibrary::MagToIntensity(float ApparentMag, float kExposure)
{
	// Standard photometric relation: brightness ratio = 10^(-0.4 * mag)
	// We clamp for stability and artistic control.
	const float I = FMath::Pow(10.0f, -0.4f * ApparentMag) * kExposure;
	return FMath::Clamp(I, 0.0f, 100000.0f);
}

double UCelestialMathLibrary::ApproxGMST_Radians(double SimTimeSeconds, double EarthSiderealDaySeconds)
{
	// For a game/sandbox, the stable thing we want is:
	// GMST advances at sidereal rate (one full rotation per sidereal day).
	// This is the minimum viable "sidereal time" to rotate starfield correctly vs solar time.
	if (EarthSiderealDaySeconds <= 0.0)
	{
		EarthSiderealDaySeconds = 86164.0905;
	}

	const double Phase01 = FMath::Fmod(SimTimeSeconds / EarthSiderealDaySeconds, 1.0);
	const double PhaseWrapped = (Phase01 < 0.0) ? (Phase01 + 1.0) : Phase01;
	return PhaseWrapped * TWO_PI;
}
