// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Biome Subsystem - Terrain-to-Spec Mapping Layer
 * 
 * Purpose:
 * Bridges terrain authoring (PCG, RVT, heightmaps) with the physics spec system.
 * Queries terrain properties and maps them to SurfaceSpecId/MediumSpecId.
 * 
 * Query Pipeline:
 *   World Location → BiomeSubsystem → BiomeId → SpecIds → Query Subsystems → Runtime State
 * 
 * Data Flow:
 * 1. Artist authors biomes in terrain tools (Landscape, PCG, RVT masks)
 * 2. BiomeSpecs define mapping: BiomeId → {SurfaceSpecId, MediumSpecId}
 * 3. Runtime queries: Location → Biome resolution → Spec lookup → Physics state
 * 
 * Biome Resolution Algorithm:
 * 1. Sample RVT masks at location (if available)
 * 2. Query terrain altitude and slope
 * 3. Match against BiomeSpec rules (altitude range, slope range, RVT channel)
 * 4. Return primary biome + optional secondary for blending
 * 
 * RVT Integration:
 * - Each BiomeSpec can specify an RVT channel (0-3 for RGBA)
 * - RVT provides high-resolution biome masks painted by artists
 * - Fallback to altitude/slope rules if RVT not available
 * 
 * Integration Points:
 * - SurfaceQuerySubsystem: Can call GetSurfaceSpecForLocation()
 * - EnvironmentSubsystem: Uses biome data to modify atmosphere
 * - PCG: Can query biomes for procedural placement rules
 * 
 * @see USurfaceQuerySubsystem for surface spec resolution
 * @see UEnvironmentSubsystem for medium/atmosphere specs
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpecTypes.h"
#include "BiomeSubsystem.generated.h"

class UGlobalAtmosphereField;
class USurfaceQuerySubsystem;
class UEnvironmentSubsystem;
class ULandscapeComponent;

/**
 * Biome identifier.
 * Used to map terrain/RVT data to spec IDs.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FBiomeId
{
	GENERATED_BODY()

	/** Unique biome identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FName Id;

	FBiomeId() : Id(NAME_None) {}
	explicit FBiomeId(FName InId) : Id(InId) {}

	bool IsValid() const { return Id != NAME_None; }
	bool operator==(const FBiomeId& Other) const { return Id == Other.Id; }
	bool operator!=(const FBiomeId& Other) const { return Id != Other.Id; }

	friend uint32 GetTypeHash(const FBiomeId& BiomeId)
	{
		return GetTypeHash(BiomeId.Id);
	}
};

/**
 * Biome specification - defines the properties and associated specs for a biome.
 * 
 * A biome is a region of the world with consistent environmental properties.
 * This struct maps a biome to its surface/medium specs and defines its spatial extent.
 * 
 * Biome Resolution:
 * - RVT channel matching (if RVT available)
 * - Altitude range matching (AltitudeRange.X to AltitudeRange.Y)
 * - Slope range matching (SlopeRange.X to SlopeRange.Y degrees)
 * 
 * Example Biomes:
 * - "DesertFloor": Sand surface, hot/dry medium, altitude 0-500m, slope 0-15°
 * - "MountainRock": Rock surface, thin air medium, altitude 2000-5000m, slope 30-90°
 * - "Ocean": Water surface, water medium, altitude < 0m
 * 
 * Usage:
 *   FBiomeSpec DesertBiome;
 *   DesertBiome.BiomeId = FBiomeId(TEXT("Desert"));
 *   DesertBiome.DefaultSurfaceSpecId = FSurfaceSpecId(TEXT("Sand"));
 *   DesertBiome.TemperatureModifier = 10.0f; // +10K hotter than base
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FBiomeSpec
{
	GENERATED_BODY()

	/** Biome identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FBiomeId BiomeId;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FText DisplayName;

	/** Default surface spec for this biome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FSurfaceSpecId DefaultSurfaceSpecId;

	/** Default medium spec for this biome (usually atmosphere) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FMediumSpecId DefaultMediumSpecId;

	/** RVT color channel that represents this biome (for mask-based lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	int32 RVTChannel = 0;

	/** Temperature modifier (Kelvin offset from base) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	float TemperatureModifier = 0.0f;

	/** Humidity (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	float Humidity = 0.5f;

	/** Wind strength multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	float WindMultiplier = 1.0f;

	/** Altitude range for this biome (min, max in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FVector2D AltitudeRange = FVector2D(-1000000.0f, 1000000.0f);

	/** Slope range for this biome (min, max in degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FVector2D SlopeRange = FVector2D(0.0f, 90.0f);

	FBiomeSpec() = default;
};

/**
 * Result of a biome query.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FBiomeQueryResult
{
	GENERATED_BODY()

	/** Primary biome at this location */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	FBiomeId PrimaryBiome;

	/** Blend weight of primary biome (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	float PrimaryWeight = 1.0f;

	/** Secondary biome (for blending) */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	FBiomeId SecondaryBiome;

	/** Blend weight of secondary biome (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	float SecondaryWeight = 0.0f;

	/** Computed surface spec ID (may be blended) */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	FSurfaceSpecId SurfaceSpecId;

	/** Computed medium spec ID */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	FMediumSpecId MediumSpecId;

	/** Local altitude above sea level (cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	float Altitude = 0.0f;

	/** Local slope angle (degrees) */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	float SlopeAngle = 0.0f;

	/** Whether this query result is valid */
	UPROPERTY(BlueprintReadOnly, Category = "Biome")
	bool bIsValid = false;

	FBiomeQueryResult() = default;
};

/**
 * BiomeSubsystem
 * 
 * Queries terrain/RVT for biome identification and maps biomes to spec IDs.
 * This is the link between PCG/terrain authoring and the physics spec system.
 * 
 * Query Pipeline:
 *   Terrain/RVT → BiomeSubsystem → SurfaceSpecId/MediumSpecId → Query Subsystems → State
 * 
 * Data Flow:
 * - BiomeSpecs loaded from SpecPacks (runtime JSON/CBOR)
 * - RVT sampled for biome masks
 * - Altitude/slope computed from terrain
 * - Biome resolved via rules (altitude + slope + RVT)
 * 
 * Integration:
 * - SurfaceQuerySubsystem can call GetSurfaceSpecForLocation()
 * - EnvironmentSubsystem uses GlobalAtmosphereField as default, biome modifies
 */
UCLASS()
class UETPFCORE_API UBiomeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UBiomeSubsystem();

	//--- USubsystem Interface ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	//==========================================================================
	// BIOME QUERIES
	//==========================================================================

	/**
	 * Query the biome at a world location.
	 * Uses terrain height, slope, and RVT masks to determine biome.
	 * 
	 * @param WorldLocation - Location to query (cm)
	 * @return FBiomeQueryResult with biome info and associated spec IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FBiomeQueryResult GetBiomeAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get the biome ID at a location (simplified query).
	 * 
	 * @param WorldLocation - Location to query
	 * @return FBiomeId (may be invalid if no biome found)
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FBiomeId GetBiomeIdAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get the surface spec ID for a biome.
	 * 
	 * @param BiomeId - The biome to query
	 * @return FSurfaceSpecId for the biome's default surface, or invalid if biome not found
	 * 
	 * @note Returns the default surface spec defined in the biome spec
	 * @note Use SurfaceQuerySubsystem to resolve the spec ID to actual data
	 *
	 * @see USurfaceQuerySubsystem
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FSurfaceSpecId GetSurfaceSpecForBiome(const FBiomeId& BiomeId) const;

	/**
	 * Get the medium spec ID for a biome.
	 * 
	 * @param BiomeId - The biome to query
	 * @return FMediumSpecId for this biome
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FMediumSpecId GetMediumSpecForBiome(const FBiomeId& BiomeId) const;

	/**
	 * Get the surface spec ID at a world location.
	 * Convenience function combining biome query + spec lookup.
	 * 
	 * @param WorldLocation - Location to query
	 * @return FSurfaceSpecId for this location
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome")
	FSurfaceSpecId GetSurfaceSpecAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get terrain height at a location.
	 * 
	 * @param WorldLocation - XY location to query
	 * @return Terrain height (Z) in cm, or 0 if no terrain
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome|Terrain")
	float GetTerrainHeightAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get terrain slope at a location (in degrees).
	 * 
	 * @param WorldLocation - Location to query
	 * @return Slope angle in degrees (0 = flat, 90 = vertical)
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome|Terrain")
	float GetTerrainSlopeAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get terrain normal at a location.
	 * 
	 * @param WorldLocation - Location to query
	 * @return Surface normal vector
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome|Terrain")
	FVector GetTerrainNormalAtLocation(const FVector& WorldLocation) const;

	//==========================================================================
	// BIOME REGISTRATION
	//==========================================================================

	/**
	 * Register a biome spec.
	 * Called during SpecPack loading.
	 * 
	 * @param Spec - The biome specification to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome|Registration")
	void RegisterBiomeSpec(const FBiomeSpec& Spec);

	/**
	 * Get a registered biome spec.
	 * 
	 * @param BiomeId - The biome to look up
	 * @return Pointer to spec, or nullptr if not found
	 */
	const FBiomeSpec* GetBiomeSpec(const FBiomeId& BiomeId) const;

	/**
	 * Set the default biome (used when no other biome matches).
	 * 
	 * @param BiomeId - The default biome ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome|Registration")
	void SetDefaultBiome(const FBiomeId& BiomeId);

	/**
	 * Get all registered biome IDs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Biome|Registration")
	TArray<FBiomeId> GetAllBiomeIds() const;

	//==========================================================================
	// CONFIGURATION
	//==========================================================================

	/** Sea level altitude (Z coordinate in world units, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Config")
	float SeaLevelAltitude = 0.0f;

	/** Enable RVT-based biome lookup (requires RVT setup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Config")
	bool bUseRVTBiomeLookup = false;

	/** Enable altitude-based biome rules */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Config")
	bool bUseAltitudeRules = true;

	/** Enable slope-based biome rules */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome|Config")
	bool bUseSlopeRules = true;

protected:
	/**
	 * Resolve biome from altitude and slope rules.
	 * Used when RVT lookup is disabled or unavailable.
	 */
	FBiomeId ResolveBiomeFromRules(float Altitude, float SlopeAngle) const;

	/**
	 * Sample RVT at a location for biome mask values.
	 * Returns biome weights per channel.
	 */
	bool SampleRVTAtLocation(const FVector& WorldLocation, TArray<float>& OutWeights) const;

	/**
	 * Find the landscape component at a location.
	 */
	ULandscapeComponent* FindLandscapeAtLocation(const FVector& WorldLocation) const;

private:
	/** Registered biome specs (keyed by FBiomeId) */
	UPROPERTY()
	TMap<FName, FBiomeSpec> BiomeSpecs;

	/** Default biome ID when no rules match */
	FBiomeId DefaultBiomeId;

	/** Cached reference to other subsystems */
	UPROPERTY()
	TObjectPtr<USurfaceQuerySubsystem> SurfaceQuerySubsystem;

	UPROPERTY()
	TObjectPtr<UEnvironmentSubsystem> EnvironmentSubsystem;
};
