// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Star Catalog Subsystem - Astronomical Star Database
 * 
 * Purpose:
 * Loads and manages the HYG (Hipparcos, Yale, Gliese) star catalog for starfield rendering.
 * Provides precise celestial positions for ~5000+ visible stars.
 * 
 * Data Source:
 * - HYG Database v3.7 (CSV format)
 * - Contains: Position (RA/Dec), Magnitude, Color, Distance, Names
 * - Stars down to magnitude ~6.5 (naked eye visible)
 * 
 * Architecture:
 * - Loads once at initialization (lazy on first EnsureLoaded() call)
 * - Internal storage: High-precision (double) for calculations
 * - Output format: Single-precision (float) for Niagara/Blueprint
 * - Cached conversion to avoid redundant precision downgrade
 * 
 * Usage:
 * \code{.cpp}
 *   UStarCatalogSubsystem* Catalog = GameInstance->GetSubsystem<UStarCatalogSubsystem>();
 *   Catalog->EnsureLoaded();
 *   const TArray<FStarRecord>& Stars = Catalog->GetStars();
 *   // Pass to Niagara for rendering
 * \endcode
 * 
 * Performance:
 * - One-time load cost: ~50ms for 5000 stars
 * - Memory: ~500KB for full catalog
 * - No per-frame cost (static data)
 * 
 * Coordinate System:
 * - Star directions in equatorial J2000 frame
 * - RA stored in HOURS (0-24), Dec in DEGREES (-90 to +90)
 * - Output vectors normalized and ready for rendering
 * 
 * @see UCelestialMathLibrary::EquatorialDir_FromRaDec for coordinate conversion
 * @see AUniversalSkyActor for starfield rendering integration
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StarCatalogSubsystem.generated.h"

/**
 * Internal star record for precise astronomical calculations.
 * Uses double/FVector3d for sidereal math precision.
 */
struct FStarRecordInternal
{
	int32 Id = 0;            // Star catalog ID
	FString Name;            // Harvard Revised catalog name
	FString ProperName;      // Common name (e.g., "Sirius", "Betelgeuse")
	FString BayerFlamsteed;  // Bayer/Flamsteed designation (e.g., "Alpha CMa")
	FVector3d DirEquatorial; // Unit vector in equatorial J2000 frame (double precision)
	float Mag;               // Apparent magnitude
	float CI;                // Color index (B-V)
	float DistanceParsecs;   // Distance in parsecs for parallax
	FString SpectralType;    // Spectral classification (e.g., "A1V")
	FString Constellation;   // 3-letter constellation code (e.g., "CMa")
};

/**
 * Minimal star record for GPU starfield rendering (Niagara-compatible).
 * Converted from internal double precision at the edge.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FStarRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Id = 0;
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString ProperName;
	UPROPERTY(BlueprintReadOnly) FString BayerFlamsteed;
	UPROPERTY(BlueprintReadOnly) FVector3f DirEquatorial = FVector3f(1, 0, 0);
	UPROPERTY(BlueprintReadOnly) float Mag = 6.0f;
	UPROPERTY(BlueprintReadOnly) float CI = 0.65f;
	UPROPERTY(BlueprintReadOnly) float DistanceParsecs = 1000.0f;
	UPROPERTY(BlueprintReadOnly) FString SpectralType;
	UPROPERTY(BlueprintReadOnly) FString Constellation;
};

/**
 * Star catalog subsystem for astronomical starfield rendering.
 * 
 * Lifecycle:
 * 1. Initialize() - Registers subsystem (does not load data yet)
 * 2. EnsureLoaded() - Lazy loads star catalog from CSV on first call
 * 3. GetStars() - Returns array of star records for rendering
 * 4. Deinitialize() - Cleanup
 * 
 * Data Flow:
 * 1. Load CSV from Content/SpecPacks/Space/starmap_milkyway.csv - generate with script if needed
 * 2. Parse each line: ID, Name, RA, Dec, Magnitude, Color, Distance
 * 3. Convert to internal high-precision format (FStarRecordInternal)
 * 4. Convert to output format (FStarRecord) for Niagara
 * 5. Cache both for efficient access
 * 
 * Thread Safety:
 * - Loading is synchronous on game thread
 * - Once loaded, GetStars() is read-only and safe from any thread
 * - No locking needed after initial load
 * 
 * Integration:
 * - AUniversalSkyActor: Fetches stars for Niagara particle system
 * - Blueprint: Can query star data for UI/gameplay (constellation names, etc.)
 * - Editor tools: Can use for procedural sky generation
 * 
 * Configuration:
 * - RelativeCsvPath: Path to CSV file (relative to Content/)
 * - MaxStars: Safety cap to prevent memory explosion on bad files
 * 
 * CSV Format:
 * ```csv
 * id,hip,hd,hr,gl,bf,proper,ra,dec,dist,pmra,pmdec,rv,mag,absmag,spect,ci,x,y,z,vx,vy,vz,rarad,decrad,pmrarad,pmdecrad,bayer,flam,con,comp,comp_primary,base
 * 0,,,,,,,0,0,0,0,0,0,-26.73,,,0.656,0,0,10,0,0,0,0,0,0,0,,,,0,,Sol
 * ```
 * 
 * @note Uses double precision internally for astronomical accuracy
 * @note Converts to float for rendering (Niagara compatibility)
 */
UCLASS()
class UETPFCORE_API UStarCatalogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Load once (idempotent). Returns true if we have stars after call. */
	UFUNCTION(BlueprintCallable, Category="Astro|Stars")
	bool EnsureLoaded();

	/** Optional manual reload for dev. */
	UFUNCTION(BlueprintCallable, Category="Astro|Stars")
	bool Reload();

	/** Get stars in Niagara-compatible format (converted from internal precision). */
	const TArray<FStarRecord>& GetStars() const;

	UFUNCTION(BlueprintPure, Category="Astro|Stars")
	int32 GetStarCount() const { return Stars.Num(); }

	UFUNCTION(BlueprintCallable, Category="Astro|Stars")
	void GetStarsCopy(TArray<FStarRecord>& OutStars) const { OutStars = Stars; }

	UFUNCTION(BlueprintPure, Category="Astro|Stars")
	bool IsLoaded() const { return bLoaded && StarsInternal.Num() > 0; }

	/** Config: relative to Content/ */
	UPROPERTY(EditAnywhere, Category="Astro|Stars")
	FString RelativeCsvPath = TEXT("SpecPacks/Space/starmap_milkyway.csv");

	/** Config: safety cap so bad files don't explode memory. */
	UPROPERTY(EditAnywhere, Category="Astro|Stars")
	int32 MaxStars = 5000;

private:
	bool LoadFromCsv(const FString& FullPath);
	void ConvertInternalToOutput();

private:
	UPROPERTY() bool bLoaded = false;
	TArray<FStarRecordInternal> StarsInternal;  // Internal precise data
	UPROPERTY() TArray<FStarRecord> Stars;      // Output Niagara-compatible data
};
