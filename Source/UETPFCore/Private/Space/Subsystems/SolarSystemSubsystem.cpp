// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Space/Subsystems/SolarSystemSubsystem.h"
#include "Subsystems/TimeSubsystem.h"
#include "Environment/SkyContext.h"
#include "Misc/DateTime.h"
#include "Math/UnrealMathUtility.h"

// If you want hard dependency, include your TimeSubsystem header path here:
#include "Subsystems/TimeSubsystem.h"

namespace SolarMath
{
	static constexpr double TwoPi = 6.283185307179586476925286766559;
	static constexpr double DegToRad = PI / 180.0;

	static double Wrap0ToTwoPi(double Rad)
	{
		double X = FMath::Fmod(Rad, TwoPi);
		return (X < 0.0) ? (X + TwoPi) : X;
	}

	static FVector3d SafeNormal(const FVector3d& V, const FVector3d& Fallback = FVector3d(1,0,0))
	{
		const double L2 = V.SquaredLength();
		return (L2 > 1e-18) ? (V / FMath::Sqrt(L2)) : Fallback;
	}
}


void USolarSystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	InitDefaults();

	// Subscribe to TimeSubsystem's authoritative time updates
	const UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (UTimeSubsystem* TimeSys = GI->GetSubsystem<UTimeSubsystem>())
		{
			TimeAdvancedHandle = TimeSys->OnSimTimeAdvanced.AddUObject(this, &USolarSystemSubsystem::OnTimeAdvanced);
		}
	}
}

void USolarSystemSubsystem::Deinitialize()
{
	// Unsubscribe
	const UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (UTimeSubsystem* TimeSys = GI->GetSubsystem<UTimeSubsystem>())
		{
			TimeSys->OnSimTimeAdvanced.Remove(TimeAdvancedHandle);
		}
	}

	Super::Deinitialize();
}

void USolarSystemSubsystem::OnTimeAdvanced(double NewSimTimeSeconds)
{
	// Invalidate cache to force recompute on next query
	CachedSimUnixSeconds = -1.0;
}

void USolarSystemSubsystem::InitDefaults()
{
	BodyDefs.Empty();

	{
		FCelestialBodyDef Sun;
		Sun.Id = ECelestialBodyId::Sun;
		Sun.RadiusKm = 696340.0; // mean solar radius (km)
		Sun.bHasAtmosphere = false;
		Sun.bHasClouds = false;
		BodyDefs.Add(Sun.Id, Sun);
	}
	{
		FCelestialBodyDef Earth;
		Earth.Id = ECelestialBodyId::Earth;
		Earth.RadiusKm = 6371.0;
		Earth.bHasAtmosphere = true;
		Earth.bHasClouds = true;
		BodyDefs.Add(Earth.Id, Earth);
	}
	{
		FCelestialBodyDef Moon;
		Moon.Id = ECelestialBodyId::Moon;
		Moon.RadiusKm = 1737.4;
		Moon.bHasAtmosphere = false;
		Moon.bHasClouds = false;
		BodyDefs.Add(Moon.Id, Moon);
	}
}

FCelestialBodyDef USolarSystemSubsystem::GetBodyDef(ECelestialBodyId Body) const
{
	if (const FCelestialBodyDef* Def = BodyDefs.Find(Body))
	{
		return *Def;
	}
	return FCelestialBodyDef{};
}

FCelestialBodyState USolarSystemSubsystem::GetBodyState(ECelestialBodyId Body) const
{
	EnsureCacheUpToDate();

	// Canonical choice: Earth at origin, Sun is a direction (we don't place it at AU distance in this cheap model).
	// If you later add AU distance, you can return Sun position too.
	if (Body == ECelestialBodyId::Earth)
	{
		return FCelestialBodyState{}; // origin
	}
	if (Body == ECelestialBodyId::Moon)
	{
		FCelestialBodyState M;
		M.PositionKm = FVector(CachedMoonPositionKm_D);
		M.VelocityKmS = FVector(CachedMoonVelocityKmS_D);
		return M;
	}
	if (Body == ECelestialBodyId::Sun)
	{
		FCelestialBodyState S;
		// Put Sun at far direction for “visual proxy” placement if you want:
		// Distance here is arbitrary unless you introduce AU scale.
		S.PositionKm = FVector(CachedSunDir_D * 100000.0);
		S.VelocityKmS = FVector::ZeroVector;
		return S;
	}
	return FCelestialBodyState{};
}

FVector USolarSystemSubsystem::GetSunDirCanonical() const
{
	EnsureCacheUpToDate();
	return FVector(CachedSunDir_D);
}

float USolarSystemSubsystem::GetMoonPhaseRad() const
{
	EnsureCacheUpToDate();

	// Earth at origin. ToMoon = Moon - Earth.
	const FVector3d ToMoon = CachedMoonPositionKm_D;
	// ToSun: in our model, SunDir is the direction from Earth towards Sun.
	const FVector3d ToSun = CachedSunDir_D;

	return ComputePhaseAngleRad(FVector(ToMoon), FVector(ToSun));
}

double USolarSystemSubsystem::GetMoonIlluminationFraction() const
{
	// Illuminated fraction = (1 - cos(phase)) / 2
	const double Phase = GetMoonPhaseRad();
	return 0.5 * (1.0 - FMath::Cos(Phase));
}

double USolarSystemSubsystem::GetGMSTAngleRad() const
{
	EnsureCacheUpToDate();
	return CachedGMST;
}

double USolarSystemSubsystem::GetJulianDate() const
{
	EnsureCacheUpToDate();
	return CachedJulianDate;
}

FSolarSystemState USolarSystemSubsystem::GetSolarSystemState() const
{
	EnsureCacheUpToDate();

	FSolarSystemState State;
	State.SunDir_World = FVector(CachedSunDir_D);  // For now, canonical == world; transform later if needed
	State.SunIlluminanceLux = 100000.0f;  // Placeholder; compute based on distance/angle later
	State.MoonDir_World = FVector(CachedMoonPositionKm_D.GetSafeNormal());
	State.MoonPhase01 = (float)GetMoonIlluminationFraction();
	State.LocalSiderealTimeRad = CachedGMST;
	State.ActiveBody = ECelestialBodyId::Earth;  // Placeholder; drive from WorldFrameSubsystem later

	return State;
}

double USolarSystemSubsystem::GetSimUnixSeconds() const
{
	// Single source of truth: your UTimeSubsystem
	const UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return GameEpochUnixSeconds;
	}

	const UTimeSubsystem* Time = GI->GetSubsystem<UTimeSubsystem>();
	if (!Time)
	{
		return GameEpochUnixSeconds;
	}

	const double SimSeconds = Time->GetSimTimeSeconds();

	if (bUseUnixEpochTime)
	{
		return SimSeconds;
	}
	return GameEpochUnixSeconds + SimSeconds;
}

void USolarSystemSubsystem::EnsureCacheUpToDate() const
{
	const double SimUnix = GetSimUnixSeconds();
	if (FMath::IsNearlyEqual(SimUnix, CachedSimUnixSeconds, 1e-6))
	{
		return; // cache valid
	}

	CachedSimUnixSeconds = SimUnix;
	CachedJulianDate = UnixSecondsToJulianDate(SimUnix);

	CachedSunDir_D = ComputeSunDirApprox_J2000(CachedJulianDate);
	CachedGMST = ComputeGMSTAngleRad_FromJD(CachedJulianDate);

	ComputeMoonState(SimUnix);
}

double USolarSystemSubsystem::UnixSecondsToJulianDate(double UnixSeconds)
{
	// Unix epoch 1970-01-01 00:00:00 UTC = JD 2440587.5
	static constexpr double UnixEpochJD = 2440587.5;
	return UnixEpochJD + (UnixSeconds / 86400.0);
}

FVector3d USolarSystemSubsystem::ComputeSunDirApprox_J2000(double JD)
{
	// “Good enough” Sun position approximation (low precision) for visuals + season-ish logic.
	// Based on common solar formulas using Julian centuries from J2000.
	//
	// Output: unit vector in an Earth-centered equatorial frame (approx).
	// If you later add a full ephemeris, keep the output contract the same.

	const double T = (JD - 2451545.0) / 36525.0; // centuries since J2000
	const double L0 = SolarMath::Wrap0ToTwoPi(SolarMath::DegToRad * (280.46646 + 36000.76983 * T + 0.0003032 * T * T));
	const double M  = SolarMath::Wrap0ToTwoPi(SolarMath::DegToRad * (357.52911 + 35999.05029 * T - 0.0001537 * T * T));

	// Equation of center (degrees)
	const double C = SolarMath::DegToRad * (
		(1.914602 - 0.004817 * T - 0.000014 * T * T) * FMath::Sin(M) +
		(0.019993 - 0.000101 * T) * FMath::Sin(2.0 * M) +
		0.000289 * FMath::Sin(3.0 * M)
	);

	const double TrueLong = L0 + C;

	// Obliquity of the ecliptic (approx)
	const double Eps = SolarMath::DegToRad * (23.439291 - 0.0130042 * T);

	// Convert ecliptic longitude to equatorial unit direction
	// Sun's apparent latitude ~0 in this simple model
	const double X = FMath::Cos(TrueLong);
	const double Y = FMath::Cos(Eps) * FMath::Sin(TrueLong);
	const double Z = FMath::Sin(Eps) * FMath::Sin(TrueLong);

	// This gives direction from Earth to Sun (unit)
	return SolarMath::SafeNormal(FVector3d(X, Y, Z), FVector3d(1,0,0));
}

double USolarSystemSubsystem::ComputeGMSTAngleRad_FromJD(double JD)
{
	// GMST approximation:
	// GMST (hours) = 18.697374558 + 24.06570982441908 * D
	// where D = days since J2000 (JD 2451545.0)
	const double D = (JD - 2451545.0);
	const double GMST_Hours = 18.697374558 + 24.06570982441908 * D;

	const double GMST_Rad = SolarMath::TwoPi * FMath::Fmod(GMST_Hours / 24.0, 1.0);
	return SolarMath::Wrap0ToTwoPi(GMST_Rad);
}

void USolarSystemSubsystem::ComputeMoonState(double SimUnixSeconds) const
{
	// Simple circular inclined orbit around Earth:
	// - Enough for moon position in sky + lighting phase behavior.
	// - Upgrade later to a better lunar model without changing callers.

	const double t = SimUnixSeconds; // seconds
	const double w = SolarMath::TwoPi / FMath::Max(1.0, MoonOrbitalPeriodS); // rad/s
	const double theta = SolarMath::Wrap0ToTwoPi(w * t);

	const double R = EarthMoonDistanceKm;
	const double inc = MoonInclinationDeg * SolarMath::DegToRad;

	// Orbit in X-Y plane, then incline about X axis (cheap)
	const double x = R * FMath::Cos(theta);
	const double y0 = R * FMath::Sin(theta);
	const double y = y0 * FMath::Cos(inc);
	const double z = y0 * FMath::Sin(inc);

	CachedMoonPositionKm_D = FVector3d(x, y, z);

	// Velocity (derivative)
	const double vx = -R * w * FMath::Sin(theta);
	const double vy0 = R * w * FMath::Cos(theta);
	const double vy = vy0 * FMath::Cos(inc);
	const double vz = vy0 * FMath::Sin(inc);

	CachedMoonVelocityKmS_D = FVector3d(vx, vy, vz);
}

float USolarSystemSubsystem::ComputePhaseAngleRad(const FVector& ToMoon, const FVector& ToSun)
{
	// Generic, scalable definition:
	// For any planet+moon in any solar system:
	// - ToMoon = (MoonPos - PlanetPos) in some inertial frame
	// - ToSun  = (SunPos  - PlanetPos) OR just sun direction from planet
	//
	// PhaseAngle = acos( dot( normalize(ToMoon), normalize(ToSun) ) )
	// 0   => moon near sun direction (new)
	// pi  => moon opposite sun direction (full)

	const FVector A = SolarMath::SafeNormal(ToMoon);
	const FVector B = SolarMath::SafeNormal(ToSun);

	const float d = FMath::Clamp(A.Dot(B), -1.0f, 1.0f);
	return FMath::Acos(d);
}
