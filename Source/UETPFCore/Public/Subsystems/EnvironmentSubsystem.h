// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Components/BoxComponent.h"
#include "SpecTypes.h"
#include "EnvironmentSubsystem.generated.h"

class UMediumSpec;
class UGlobalAtmosphereField;

/**
 * Volume component that defines an environmental medium region.
 * Place these in your level to define vacuum chambers, water bodies, etc.
 */
UCLASS(ClassGroup = "Environment", meta = (BlueprintSpawnableComponent))
class UETPFCORE_API UEnvironmentVolumeComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UEnvironmentVolumeComponent();

	/** The medium spec to use within this volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FMediumSpecId MediumSpecId;

	/** Priority for overlapping volumes (higher = takes precedence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	int32 Priority = 0;

	/** Temperature override (0 = use MediumSpec default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Overrides")
	float TemperatureOverride = 0.0f;

	/** Pressure override (0 = use MediumSpec default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Overrides")
	float PressureOverride = 0.0f;

	/** Wind velocity within this volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Overrides")
	FVector WindVelocity = FVector::ZeroVector;
};

/**
 * World subsystem that resolves environmental context (medium/atmosphere).
 * Queries volumes to determine density, pressure, temperature, gravity, sound at any location.
 * 
 * Usage:
 *   - Call GetEnvironmentAtLocation() to get the full environmental context
 *   - Used by drag/buoyancy calculations, audio system, precipitation
 */
UCLASS()
class UETPFCORE_API UEnvironmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UEnvironmentSubsystem();

	//--- USubsystem Interface ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	//==========================================================================
	// ENVIRONMENT QUERIES
	//==========================================================================

	/**
	 * Get the environmental context at a world location.
	 * Resolves overlapping volumes by priority.
	 * 
	 * @param WorldLocation - Location to query
	 * @return FEnvironmentContext with all relevant environmental properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	FEnvironmentContext GetEnvironmentAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get the medium spec ID at a world location.
	 * Quick lookup without building full context.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	FMediumSpecId GetMediumAtLocation(const FVector& WorldLocation) const;

	/**
	 * Check if a location is in vacuum (density effectively zero).
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	bool IsVacuumAtLocation(const FVector& WorldLocation) const;

	/**
	 * Check if a location is underwater.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	bool IsUnderwaterAtLocation(const FVector& WorldLocation) const;

	/**
	 * Get sound attenuation multiplier at a location.
	 * Returns 0 in vacuum, reduced in thin atmosphere.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	float GetSoundAttenuationAtLocation(const FVector& WorldLocation) const;

	/**
	 * Calculate drag force for an object at a location.
	 * 
	 * @param WorldLocation - Object's location
	 * @param Velocity - Object's velocity (cm/s)
	 * @param DragArea - Effective drag area (cm²)
	 * @param DragCoefficient - Cd (typically 0.3-1.0)
	 * @return Drag force vector (opposing velocity)
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Physics")
	FVector CalculateDragForce(const FVector& WorldLocation, const FVector& Velocity, 
		float DragArea, float DragCoefficient = 0.5f) const;

	/**
	 * Calculate buoyancy force for an object at a location.
	 * 
	 * @param WorldLocation - Object's location
	 * @param DisplacedVolume - Volume of fluid displaced (cm³)
	 * @return Buoyancy force vector (typically upward)
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Physics")
	FVector CalculateBuoyancyForce(const FVector& WorldLocation, float DisplacedVolume) const;

	//==========================================================================
	// SPEC REGISTRATION
	//==========================================================================

	/**
	 * Register a MediumSpec for use in the environment system.
	 * 
	 * @param SpecId - The ID to register
	 * @param Spec - The medium spec asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	void RegisterMediumSpec(const FMediumSpecId& SpecId, UMediumSpec* Spec);

	/**
	 * Get a registered medium spec by ID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	UMediumSpec* GetMediumSpec(const FMediumSpecId& SpecId) const;

	/**
	 * Set the default medium spec (used when no volumes overlap and no atmosphere field).
	 * Typically this is your "normal atmosphere" spec.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	void SetDefaultMediumSpec(UMediumSpec* Spec);

	/**
	 * Set the global atmosphere field for altitude-based medium queries.
	 * When set, this provides the default environment when not inside a volume.
	 * The atmosphere field computes density/pressure/temperature from altitude.
	 * 
	 * @param AtmosphereField - The global atmosphere field component
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	void SetGlobalAtmosphereField(UGlobalAtmosphereField* AtmosphereField);

	/**
	 * Get the current global atmosphere field.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	UGlobalAtmosphereField* GetGlobalAtmosphereField() const { return GlobalAtmosphereField; }

	//==========================================================================
	// RUNTIME SPEC REGISTRATION (SpecPack / JSON)
	//==========================================================================

	/**
	 * Register a runtime medium spec by ID.
	 * This is the primary method for SpecPack loading.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	void RegisterRuntimeMediumSpec(const FMediumSpecId& Id, const FRuntimeMediumSpec& Spec);

	/**
	 * Resolve a medium spec by ID.
	 * Resolution order: Runtime registry → DataAsset fallback → Default
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	bool ResolveMediumSpec(const FMediumSpecId& Id, FRuntimeMediumSpec& OutSpec) const;

	/**
	 * Check if a runtime spec is registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	bool HasRuntimeMediumSpec(const FMediumSpecId& Id) const;

	/**
	 * Get all registered runtime medium spec IDs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Query")
	TArray<FMediumSpecId> GetAllRuntimeMediumSpecIds() const;

	/**
	 * Clear all runtime medium specs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Registration")
	void ClearRuntimeMediumSpecs();

	/**
	 * Get the hardcoded fallback medium spec (Earth atmosphere).
	 */
	static const FRuntimeMediumSpec& GetFallbackMediumSpec();

	/**
	 * Register an environment volume component.
	 * Called automatically when volumes are spawned.
	 */
	void RegisterVolume(UEnvironmentVolumeComponent* Volume);

	/**
	 * Unregister an environment volume component.
	 * Called automatically when volumes are destroyed.
	 */
	void UnregisterVolume(UEnvironmentVolumeComponent* Volume);

protected:
	/** Find the highest-priority volume containing a location */
	UEnvironmentVolumeComponent* FindVolumeAtLocation(const FVector& WorldLocation) const;

	/** Build environment context from a medium spec and optional volume overrides */
	FEnvironmentContext BuildEnvironmentContext(const UMediumSpec* Spec, 
		const UEnvironmentVolumeComponent* Volume = nullptr) const;

private:
	/** Map from MediumSpecId to MediumSpec (DataAsset workflow) */
	UPROPERTY()
	TMap<FName, UMediumSpec*> MediumSpecMap;

	/** Default medium spec (used when no volumes overlap and no atmosphere field) */
	UPROPERTY()
	UMediumSpec* DefaultMediumSpec;

	/** Global atmosphere field for altitude-based environment (used when not in a volume) */
	UPROPERTY()
	TObjectPtr<UGlobalAtmosphereField> GlobalAtmosphereField;

	/** All registered environment volumes */
	UPROPERTY()
	TArray<UEnvironmentVolumeComponent*> RegisteredVolumes;

	/** Runtime medium spec registry (SpecPack / JSON workflow) */
	TMap<FName, FRuntimeMediumSpec> RuntimeMediumSpecs;

	/** Hardcoded fallback medium spec (Earth atmosphere) */
	static FRuntimeMediumSpec FallbackMediumSpec;

	/** Threshold below which density is considered vacuum */
	static constexpr float VacuumDensityThreshold = 0.001f;

	/** Density threshold above which medium is considered "water-like" */
	static constexpr float WaterDensityThreshold = 500.0f;
};
