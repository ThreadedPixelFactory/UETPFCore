// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpecTypes.h"
#include "SurfaceQuerySubsystem.generated.h"

class UPhysicalMaterial;
class USurfaceSpec;

/**
 * World subsystem that provides surface state queries.
 * This is the central "thought → action" link for all physics/FX consumers.
 * 
 * Usage:
 *   - Call GetSurfaceStateFromHit() with a hit result to get the full surface state
 *   - Surface state includes friction, deformation, wetness, snow depth, FX profile
 *   - Consumers: vehicles (tire forces), characters (footsteps), cloth, FX routing
 */
UCLASS()
class UETPFCORE_API USurfaceQuerySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	USurfaceQuerySubsystem();

	//--- USubsystem Interface ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	//==========================================================================
	// SURFACE QUERIES
	//==========================================================================

	/**
	 * Get the surface state at a hit location.
	 * This is the primary query function - consumes hit result and returns full state.
	 * 
	 * @param HitResult - The hit result from a trace or collision
	 * @return FSurfaceState with all relevant surface properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Query")
	FSurfaceState GetSurfaceStateFromHit(const FHitResult& HitResult) const;

	/**
	 * Get the surface state at a world location (performs downward trace).
	 * Convenience function for when you don't have a hit result.
	 * 
	 * @param WorldLocation - Location to query
	 * @param TraceDistance - How far to trace downward (default 500cm)
	 * @return FSurfaceState with all relevant surface properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Query")
	FSurfaceState GetSurfaceStateAtLocation(const FVector& WorldLocation, float TraceDistance = 500.0f) const;

	/**
	 * Batch query surface states for multiple locations.
	 * More efficient than individual queries when you need many results.
	 * 
	 * @param WorldLocations - Array of locations to query
	 * @param TraceDistance - How far to trace downward
	 * @return Array of FSurfaceState results (same order as input)
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Query")
	TArray<FSurfaceState> BatchGetSurfaceStates(const TArray<FVector>& WorldLocations, float TraceDistance = 500.0f) const;

	//==========================================================================
	// SPEC REGISTRATION
	//==========================================================================

	/**
	 * Register a SurfaceSpec for a PhysicalMaterial.
	 * Call this during initialization to bind PhysicalMaterials to their specs.
	 * 
	 * @param PhysMat - The physical material to bind
	 * @param Spec - The surface spec to use for this material
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Registration")
	void RegisterSurfaceSpec(UPhysicalMaterial* PhysMat, USurfaceSpec* Spec);

	/**
	 * Get the surface spec for a physical material.
	 * 
	 * @param PhysMat - The physical material to look up
	 * @return The associated USurfaceSpec, or DefaultSurfaceSpec if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Registration")
	USurfaceSpec* GetSurfaceSpecForMaterial(UPhysicalMaterial* PhysMat) const;

	/**
	 * Set the default surface spec used when no specific binding exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Registration")
	void SetDefaultSurfaceSpec(USurfaceSpec* Spec);

	/**
	 * Get the default surface spec.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Registration")
	USurfaceSpec* GetDefaultSurfaceSpec() const { return DefaultSurfaceSpec; }

	//==========================================================================
	// RUNTIME SPEC REGISTRATION (SpecPack / JSON)
	//==========================================================================

	/**
	 * Register a runtime surface spec by ID.
	 * This is the primary method for SpecPack loading.
	 * 
	 * @param Id - Unique spec identifier
	 * @param Spec - The struct spec data
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Registration")
	void RegisterRuntimeSurfaceSpec(const FSurfaceSpecId& Id, const FRuntimeSurfaceSpec& Spec);

	/**
	 * Resolve a surface spec by ID.
	 * Resolution order: Runtime registry → DataAsset fallback → Default
	 * 
	 * @param Id - Spec ID to resolve
	 * @param OutSpec - Output spec data
	 * @return true if spec was found (runtime or asset), false if using default
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Query")
	bool ResolveSurfaceSpec(const FSurfaceSpecId& Id, FRuntimeSurfaceSpec& OutSpec) const;

	/**
	 * Check if a runtime spec is registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Query")
	bool HasRuntimeSpec(const FSurfaceSpecId& Id) const;

	/**
	 * Get all registered runtime spec IDs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Query")
	TArray<FSurfaceSpecId> GetAllRuntimeSpecIds() const;

	/**
	 * Clear all runtime specs (for hot-reload / testing).
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Registration")
	void ClearRuntimeSpecs();

	/**
	 * Get the hardcoded fallback spec (never fails).
	 */
	static const FRuntimeSurfaceSpec& GetFallbackSpec();

	//==========================================================================
	// DELTA INTEGRATION (for sparse delta system)
	//==========================================================================

	/**
	 * Get current wetness at a world location (from delta tiles).
	 * Override this in derived classes to integrate with your delta storage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Delta")
	virtual float GetWetnessAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get current snow depth at a world location (from delta tiles).
	 * Override this in derived classes to integrate with your delta storage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Delta")
	virtual float GetSnowDepthAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get current compaction at a world location (from delta tiles).
	 * Override this in derived classes to integrate with your delta storage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Delta")
	virtual float GetCompactionAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get current temperature at a world location.
	 * Override this to integrate with environment/weather systems.
	 */
	UFUNCTION(BlueprintCallable, Category = "Surface|Delta")
	virtual float GetTemperatureAtLocation(const FVector& WorldLocation) const;

protected:
	/** Build a complete SurfaceState from a spec and environmental conditions */
	FSurfaceState BuildSurfaceState(const USurfaceSpec* Spec, const FVector& Location) const;

	/** Apply environmental modifiers (wetness, temperature) to base friction values */
	void ApplyEnvironmentalModifiers(FSurfaceState& State, const USurfaceSpec* Spec) const;

private:
	/** Map from PhysicalMaterial to SurfaceSpec (DataAsset workflow) */
	UPROPERTY()
	TMap<UPhysicalMaterial*, USurfaceSpec*> SurfaceSpecMap;

	/** Default spec used when no specific binding exists */
	UPROPERTY()
	USurfaceSpec* DefaultSurfaceSpec;

	/** Cached trace channel for surface queries */
	ECollisionChannel SurfaceTraceChannel;

	/** Runtime spec registry (SpecPack / JSON workflow) */
	TMap<FName, FRuntimeSurfaceSpec> RuntimeSurfaceSpecs;

	/** Hardcoded fallback spec */
	static FRuntimeSurfaceSpec FallbackSurfaceSpec;
};
