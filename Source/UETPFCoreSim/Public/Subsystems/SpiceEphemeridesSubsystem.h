// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpiceEphemeridesSubsystem.generated.h"

/**
 * Ephemeris time format
 * Converts between game time and ephemeris time (seconds past J2000)
 */
USTRUCT(BlueprintType)
struct UETPFCORESIM_API FEphemerisTime
{
	GENERATED_BODY()

	/** Seconds past J2000 epoch (2000-01-01 12:00:00 TDB) */
	UPROPERTY(BlueprintReadWrite, Category = "Time")
	double SecondsPastJ2000 = 0.0;

	/** Convert from Unix timestamp */
	static FEphemerisTime FromUnixTime(double UnixSeconds);

	/** Convert to Unix timestamp */
	double ToUnixTime() const;

	/** Convert from Julian Date */
	static FEphemerisTime FromJulianDate(double JD);

	/** Convert to Julian Date */
	double ToJulianDate() const;
};

/**
 * Celestial body state vector
 * Position and velocity in J2000 inertial frame
 */
USTRUCT(BlueprintType)
struct UETPFCORESIM_API FCelestialStateVector
{
	GENERATED_BODY()

	/** Position in kilometers (J2000 frame) */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FVector PositionKm = FVector::ZeroVector;

	/** Velocity in km/s (J2000 frame) */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FVector VelocityKmS = FVector::ZeroVector;

	/** Light-time delay in seconds (from observer to target) */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	double LightTimeSeconds = 0.0;

	/** Whether this state is valid */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsValid = false;
};

/**
 * SPICE Ephemerides Subsystem
 * 
 * Provides validated celestial mechanics via NASA's SPICE Toolkit.
 * Queries planetary positions, orientations, and reference frames.
 * 
 * Requires SPICE kernels to be loaded (planetary ephemerides, leap seconds, etc.)
 * Download kernels from: https://naif.jpl.nasa.gov/pub/naif/generic_kernels/
 * 
 * Thread-safe for queries (kernel loading must be done on game thread)
 * 
 * Example usage:
 * \code{.cpp}
 *   USpiceEphemeridesSubsystem* Spice = GetGameInstance()->GetSubsystem<USpiceEphemeridesSubsystem>();
 *   
 *   // Load kernels
 *   Spice->LoadKernel("de440.bsp");  // Planetary ephemerides
 *   Spice->LoadKernel("naif0012.tls");  // Leap seconds
 *   
 *   // Query Mars position at current time
 *   FEphemerisTime ET = FEphemerisTime::FromUnixTime(CurrentTimeSeconds);
 *   FCelestialStateVector MarsState = Spice->GetBodyState("MARS", "SUN", ET);
 * \endcode
 */
UCLASS()
class UETPFCORESIM_API USpiceEphemeridesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//==========================================================================
	// KERNEL MANAGEMENT
	//==========================================================================

	/**
	 * Load a SPICE kernel file
	 * Supports: SPK (ephemerides), PCK (orientation), LSK (leap seconds), etc.
	 * 
	 * @param KernelPath - Absolute or project-relative path to kernel file
	 * @return true if kernel loaded successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Kernels")
	bool LoadKernel(const FString& KernelPath);

	/**
	 * Unload a specific kernel
	 * 
	 * @param KernelPath - Path to kernel to unload
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Kernels")
	void UnloadKernel(const FString& KernelPath);

	/**
	 * Unload all loaded kernels
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Kernels")
	void UnloadAllKernels();

	/**
	 * Get list of currently loaded kernels
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Kernels")
	TArray<FString> GetLoadedKernels() const;

	//==========================================================================
	// EPHEMERIDES QUERIES
	//==========================================================================

	/**
	 * Get celestial body state vector at given time
	 * 
	 * @param TargetBody - NAIF name or ID (e.g., "EARTH", "MARS", "MOON", "399")
	 * @param ObserverBody - Observer reference (e.g., "SUN", "EARTH")
	 * @param EphemerisTime - Time of query
	 * @param ReferenceFrame - Frame for results (default: "J2000")
	 * @param AberrationCorrection - Light-time correction ("NONE", "LT", "LT+S")
	 * @return State vector (position, velocity)
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Query")
	FCelestialStateVector GetBodyState(
		const FString& TargetBody,
		const FString& ObserverBody,
		FEphemerisTime EphemerisTime,
		const FString& ReferenceFrame = TEXT("J2000"),
		const FString& AberrationCorrection = TEXT("NONE")
	) const;

	/**
	 * Get position only (faster than full state vector)
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Query")
	FVector GetBodyPosition(
		const FString& TargetBody,
		const FString& ObserverBody,
		FEphemerisTime EphemerisTime,
		const FString& ReferenceFrame = TEXT("J2000")
	) const;

	/**
	 * Get sun direction from observer to sun
	 * Useful for lighting calculations
	 */
	UFUNCTION(BlueprintCallable, Category = "SPICE|Query")
	FVector GetSunDirection(
		const FString& ObserverBody,
		FEphemerisTime EphemerisTime
	) const;

	//==========================================================================
	// UTILITY
	//==========================================================================

	/**
	 * Check if SPICE is available and initialized
	 */
	UFUNCTION(BlueprintPure, Category = "SPICE")
	bool IsSpiceAvailable() const;

	/**
	 * Get last SPICE error message
	 */
	UFUNCTION(BlueprintPure, Category = "SPICE")
	FString GetLastError() const { return LastErrorMessage; }

private:
	/** Track loaded kernels */
	UPROPERTY()
	TArray<FString> LoadedKernels;

	/** Last error message from SPICE */
	FString LastErrorMessage;

	/** Critical section for thread-safe queries */
	mutable FCriticalSection SpiceMutex;
};
