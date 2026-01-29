// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/SpiceEphemeridesSubsystem.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

#if WITH_SPICE
extern "C" {
	// SPICE C library functions
	void furnsh_c(const char* file);
	void unload_c(const char* file);
	void kclear_c();
	void spkezr_c(const char* targ, double et, const char* ref, const char* abcorr, const char* obs, double starg[6], double* lt);
	void spkpos_c(const char* targ, double et, const char* ref, const char* abcorr, const char* obs, double ptarg[3], double* lt);
	void reset_c();
	int failed_c();
	void getmsg_c(const char* option, int lenout, char* msg);
}
#endif

//=============================================================================
// FEphemerisTime Implementation
//=============================================================================

FEphemerisTime FEphemerisTime::FromUnixTime(double UnixSeconds)
{
	// J2000 epoch: 2000-01-01 12:00:00 UTC = Unix timestamp 946728000
	constexpr double J2000UnixEpoch = 946728000.0;
	
	FEphemerisTime Result;
	Result.SecondsPastJ2000 = UnixSeconds - J2000UnixEpoch;
	return Result;
}

double FEphemerisTime::ToUnixTime() const
{
	constexpr double J2000UnixEpoch = 946728000.0;
	return SecondsPastJ2000 + J2000UnixEpoch;
}

FEphemerisTime FEphemerisTime::FromJulianDate(double JD)
{
	// JD of J2000 epoch: 2451545.0
	constexpr double J2000JD = 2451545.0;
	constexpr double SecondsPerDay = 86400.0;
	
	FEphemerisTime Result;
	Result.SecondsPastJ2000 = (JD - J2000JD) * SecondsPerDay;
	return Result;
}

double FEphemerisTime::ToJulianDate() const
{
	constexpr double J2000JD = 2451545.0;
	constexpr double SecondsPerDay = 86400.0;
	return J2000JD + (SecondsPastJ2000 / SecondsPerDay);
}

//=============================================================================
// USpiceEphemeridesSubsystem Implementation
//=============================================================================

void USpiceEphemeridesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if WITH_SPICE
	UE_LOG(LogTemp, Log, TEXT("SpiceEphemeridesSubsystem initialized"));
	UE_LOG(LogTemp, Log, TEXT("SPICE Toolkit available - scientific ephemerides enabled"));
#else
	UE_LOG(LogTemp, Warning, TEXT("SpiceEphemeridesSubsystem initialized WITHOUT SPICE"));
	UE_LOG(LogTemp, Warning, TEXT("Queries will return invalid states. Install CSPICE library."));
#endif
}

void USpiceEphemeridesSubsystem::Deinitialize()
{
	UnloadAllKernels();
	Super::Deinitialize();
}

bool USpiceEphemeridesSubsystem::LoadKernel(const FString& KernelPath)
{
#if WITH_SPICE
	FScopeLock Lock(&SpiceMutex);

	// Resolve path
	FString FullPath = KernelPath;
	if (!FPaths::FileExists(FullPath))
	{
		// Try project content directory
		FullPath = FPaths::ProjectContentDir() / KernelPath;
	}

	if (!FPaths::FileExists(FullPath))
	{
		LastErrorMessage = FString::Printf(TEXT("Kernel file not found: %s"), *KernelPath);
		UE_LOG(LogTemp, Error, TEXT("%s"), *LastErrorMessage);
		return false;
	}

	// Load kernel
	furnsh_c(TCHAR_TO_UTF8(*FullPath));

	if (failed_c())
	{
		char msg[1841];
		getmsg_c("LONG", sizeof(msg), msg);
		LastErrorMessage = FString(UTF8_TO_TCHAR(msg));
		UE_LOG(LogTemp, Error, TEXT("SPICE kernel load failed: %s"), *LastErrorMessage);
		reset_c();
		return false;
	}

	LoadedKernels.AddUnique(FullPath);
	UE_LOG(LogTemp, Log, TEXT("Loaded SPICE kernel: %s"), *FullPath);
	return true;
#else
	LastErrorMessage = TEXT("SPICE library not available");
	return false;
#endif
}

void USpiceEphemeridesSubsystem::UnloadKernel(const FString& KernelPath)
{
#if WITH_SPICE
	FScopeLock Lock(&SpiceMutex);

	if (!LoadedKernels.Contains(KernelPath))
	{
		return;
	}

	unload_c(TCHAR_TO_UTF8(*KernelPath));

	LoadedKernels.Remove(KernelPath);
	UE_LOG(LogTemp, Log, TEXT("Unloaded SPICE kernel: %s"), *KernelPath);
#endif
}

void USpiceEphemeridesSubsystem::UnloadAllKernels()
{
#if WITH_SPICE
	FScopeLock Lock(&SpiceMutex);
	
	kclear_c();
	LoadedKernels.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("Unloaded all SPICE kernels"));
#endif
}

TArray<FString> USpiceEphemeridesSubsystem::GetLoadedKernels() const
{
	return LoadedKernels;
}

FCelestialStateVector USpiceEphemeridesSubsystem::GetBodyState(
	const FString& TargetBody,
	const FString& ObserverBody,
	FEphemerisTime EphemerisTime,
	const FString& ReferenceFrame,
	const FString& AberrationCorrection
) const
{
	FCelestialStateVector Result;

#if WITH_SPICE
	FScopeLock Lock(&SpiceMutex);

	double starg[6];
	double lt;

	spkezr_c(
		TCHAR_TO_UTF8(*TargetBody),
		EphemerisTime.SecondsPastJ2000,
		TCHAR_TO_UTF8(*ReferenceFrame),
		TCHAR_TO_UTF8(*AberrationCorrection),
		TCHAR_TO_UTF8(*ObserverBody),
		starg,
		&lt
	);

	if (failed_c())
	{
		char msg[1841];
		getmsg_c("LONG", sizeof(msg), msg);
		const_cast<USpiceEphemeridesSubsystem*>(this)->LastErrorMessage = FString(UTF8_TO_TCHAR(msg));
		UE_LOG(LogTemp, Warning, TEXT("SPICE query failed: %s"), *LastErrorMessage);
		reset_c();
		return Result;
	}

	// Convert to UE coordinates (SPICE uses km)
	Result.PositionKm = FVector(starg[0], starg[1], starg[2]);
	Result.VelocityKmS = FVector(starg[3], starg[4], starg[5]);
	Result.LightTimeSeconds = lt;
	Result.bIsValid = true;

#else
	const_cast<USpiceEphemeridesSubsystem*>(this)->LastErrorMessage = TEXT("SPICE not available");
#endif

	return Result;
}

FVector USpiceEphemeridesSubsystem::GetBodyPosition(
	const FString& TargetBody,
	const FString& ObserverBody,
	FEphemerisTime EphemerisTime,
	const FString& ReferenceFrame
) const
{
#if WITH_SPICE
	FScopeLock Lock(&SpiceMutex);

	double ptarg[3];
	double lt;

	spkpos_c(
		TCHAR_TO_UTF8(*TargetBody),
		EphemerisTime.SecondsPastJ2000,
		TCHAR_TO_UTF8(*ReferenceFrame),
		"NONE",
		TCHAR_TO_UTF8(*ObserverBody),
		ptarg,
		&lt
	);

	if (failed_c())
	{
		char msg[1841];
		getmsg_c("LONG", sizeof(msg), msg);
		const_cast<USpiceEphemeridesSubsystem*>(this)->LastErrorMessage = FString(UTF8_TO_TCHAR(msg));
		reset_c();
		return FVector::ZeroVector;
	}

	return FVector(ptarg[0], ptarg[1], ptarg[2]);
#else
	return FVector::ZeroVector;
#endif
}

FVector USpiceEphemeridesSubsystem::GetSunDirection(
	const FString& ObserverBody,
	FEphemerisTime EphemerisTime
) const
{
	FVector SunPos = GetBodyPosition(TEXT("SUN"), ObserverBody, EphemerisTime);
	
	if (SunPos.IsNearlyZero())
	{
		return FVector(1, 0, 0); // Default direction if query fails
	}

	return SunPos.GetSafeNormal();
}

bool USpiceEphemeridesSubsystem::IsSpiceAvailable() const
{
#if WITH_SPICE
	return true;
#else
	return false;
#endif
}
