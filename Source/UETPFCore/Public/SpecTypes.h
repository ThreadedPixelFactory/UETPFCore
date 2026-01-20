// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "NiagaraSystem.h"
#include "SpecTypes.generated.h"

/**
 * Result of spec resolution - indicates where the spec came from.
 * All results are valid and usable; this is for debugging/telemetry.
 */
UENUM(BlueprintType)
enum class ESpecResolveResult : uint8
{
	/** Spec found in runtime registry (SpecPack/JSON) */
	Runtime,
	/** Spec found in DataAsset registry (editor workflow) */
	DataAsset,
	/** Using hardcoded fallback (spec ID not found) */
	Fallback
};

// Forward declarations
class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;

//=============================================================================
// SPEC IDS - Stable identifiers for registry lookups
//=============================================================================

/**
 * Unique identifier for surface specs. Used for DataRegistry lookups.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FSurfaceSpecId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	FName Id;

	FSurfaceSpecId() : Id(NAME_None) {}
	FSurfaceSpecId(FName InId) : Id(InId) {}

	bool IsValid() const { return !Id.IsNone(); }
	bool operator==(const FSurfaceSpecId& Other) const { return Id == Other.Id; }
	bool operator!=(const FSurfaceSpecId& Other) const { return Id != Other.Id; }
	
	friend uint32 GetTypeHash(const FSurfaceSpecId& SpecId) { return GetTypeHash(SpecId.Id); }
};

/**
 * Unique identifier for medium specs (atmosphere/fluid environments).
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FMediumSpecId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medium")
	FName Id;

	FMediumSpecId() : Id(NAME_None) {}
	FMediumSpecId(FName InId) : Id(InId) {}

	bool IsValid() const { return !Id.IsNone(); }
	bool operator==(const FMediumSpecId& Other) const { return Id == Other.Id; }
	bool operator!=(const FMediumSpecId& Other) const { return Id != Other.Id; }
	
	friend uint32 GetTypeHash(const FMediumSpecId& SpecId) { return GetTypeHash(SpecId.Id); }
};

/**
 * Unique identifier for damage/destruction specs.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FDamageSpecId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FName Id;

	FDamageSpecId() : Id(NAME_None) {}
	FDamageSpecId(FName InId) : Id(InId) {}

	bool IsValid() const { return !Id.IsNone(); }
	bool operator==(const FDamageSpecId& Other) const { return Id == Other.Id; }
	bool operator!=(const FDamageSpecId& Other) const { return Id != Other.Id; }
	
	friend uint32 GetTypeHash(const FDamageSpecId& SpecId) { return GetTypeHash(SpecId.Id); }
};

/**
 * Unique identifier for FX profiles (sound/VFX mapping).
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FFXProfileId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FName Id;

	FFXProfileId() : Id(NAME_None) {}
	FFXProfileId(FName InId) : Id(InId) {}

	bool IsValid() const { return !Id.IsNone(); }
	bool operator==(const FFXProfileId& Other) const { return Id == Other.Id; }
	bool operator!=(const FFXProfileId& Other) const { return Id != Other.Id; }
	
	friend uint32 GetTypeHash(const FFXProfileId& SpecId) { return GetTypeHash(SpecId.Id); }
};

//=============================================================================
// SURFACE SPEC - Contact material behavior
//=============================================================================

/**
 * Defines contact behavior for a surface material.
 * Used by vehicles, characters, Metal, asphalt, snow, cloth, debris for friction/deformation/FX etc.
 */
UCLASS(BlueprintType)
class UETPFCORE_API USurfaceSpec : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USurfaceSpec();

	/** Unique stable identifier for this spec */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FSurfaceSpecId SpecId;

	/** Human-readable display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	//--- Friction ---
	
	/** Static friction coefficient (force to start sliding) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Friction", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FrictionStatic = 0.8f;

	/** Dynamic friction coefficient (force while sliding) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Friction", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FrictionDynamic = 0.6f;

	/** Restitution (bounciness) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Friction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Restitution = 0.2f;

	//--- Deformation ---
	
	/** Compliance (softness) - how much the surface gives under load. 0=rigid, 1=very soft */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Compliance = 0.0f;

	/** How easily this surface deforms (tracks, footprints). 0=no deformation, 1=easy deformation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeformationStrength = 0.0f;

	/** How quickly deformation recovers (0=permanent, 1=instant recovery) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deformation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeformationRecoveryRate = 0.0f;

	//--- Environmental Response ---
	
	/** How friction changes with wetness. X=wetness(0-1), Y=friction multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environmental Response")
	FRuntimeFloatCurve WetnessResponseCurve;

	/** How friction changes with temperature. X=temp(Kelvin), Y=friction multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environmental Response")
	FRuntimeFloatCurve TemperatureResponseCurve;

	//--- Thermal ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal", meta=(ClampMin="0.0"))
	float ThermalConductivityWmK = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal", meta=(ClampMin="0.0"))
	float HeatCapacityJkgK = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Emissivity01 = 0.9f;

	//--- FX ---
	
	/** FX profile for impacts, footsteps, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FFXProfileId FXProfileId;

	/** Optional link to PhysicalMaterial for bidirectional binding */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
	TSoftObjectPtr<UPhysicalMaterial> PhysicalMaterial;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Convert this DataAsset to a runtime struct (for caching/resolution) */
	bool ToStruct(FRuntimeSurfaceSpec& OutSpec) const;
};

//=============================================================================
// MEDIUM SPEC - Atmosphere/fluid environment
//=============================================================================

/**
 * Defines an atmosphere or fluid environment + environment heat transport
 * Used for drag, buoyancy, sound propagation, thermal behavior.
 */
UCLASS(BlueprintType)
class UETPFCORE_API UMediumSpec : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UMediumSpec();

	/** Unique stable identifier for this spec */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FMediumSpecId SpecId;

	/** Human-readable display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	//--- Physical Properties ---
	
	/** Density in kg/m³ (Air ~1.225, Water ~1000, Vacuum ~0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physical", meta = (ClampMin = "0.0"))
	float Density = 1.225f;

	/** Viscosity proxy for damping calculations. Higher = more resistance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physical", meta = (ClampMin = "0.0"))
	float ViscosityProxy = 0.001f;

	/** Pressure in kPa (Earth sea level ~101.325, Mars ~0.6, Vacuum ~0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physical", meta = (ClampMin = "0.0"))
	float Pressure = 101.325f;

	//--- Thermal Properties ---

	/** Temperature in Kelvin (Earth ~288, Mars ~210) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal", meta = (ClampMin = "0.0"))
	float TemperatureK = 288.0f;

	/** Thermal conductivity in W/(m·K) - conduction / convection proxy
	 * Measures a material's ability to conduct heat:
	 *  - a higher W/mK means faster heat transfer (like metals),
	 *  - while a lower W/mK indicates better insulation (like wood or styrofoam) */
	UPROPERTY(EditDefaultsOnly, Category="Thermal")
	float ThermalConductivityWmK = 0.026f; // air baseline

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal", meta=(ClampMin="0.0"))
	float HeatCapacityJkgK = 1005.0f; // air baseline

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal", meta=(ClampMin="0.0"))
	float BackgroundRadiationTempK = 288.0f; // typical “ambient” sky/room proxy. Temperature of ambient radiative environment (space ~2.7K, Earth sky effective varies)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal", meta=(ClampMin="0.0"))
	float SolarIrradianceWm2 = 0.0f; // 0..1100+ depending on location/time

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Thermal")
	FVector SunDirection = FVector(0,0,-1); // unit vector, normalized

	//--- Dynamics ---
	
	/** Gravity vector in cm/s² (Earth = 0,0,-980) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics")
	FVector GravityVector = FVector(0.0f, 0.0f, -980.0f);

	/** Linear drag coefficient (low speed drag) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0"))
	float LinearDragCoeff = 0.1f;

	/** Quadratic drag coefficient (high speed drag, proportional to v²) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0"))
	float QuadraticDragCoeff = 0.01f;

	/** Buoyancy scale factor. 1.0 = standard Archimedes, 0.0 = no buoyancy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BuoyancyScale = 1.0f;

	//--- Audio ---
	
	/** Speed of sound in m/s (Air ~343, Water ~1480, Vacuum = 0 for gameplay purposes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float SpeedOfSound = 343.0f;

	/** Sound attenuation multiplier. 0 = no sound propagation (vacuum) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SoundAttenuationMultiplier = 1.0f;

	//--- Environment Flags ---
	
	/** Whether this medium is breathable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment")
	bool bIsBreathable = true;

	/** Whether precipitation can occur in this medium */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment")
	bool bAllowsPrecipitation = true;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Convert this DataAsset to a runtime struct (for caching/resolution) */
	bool ToStruct(FRuntimeMediumSpec& OutSpec) const;
};

//=============================================================================
// DAMAGE SPEC - Destruction/fracture behavior
//=============================================================================

/**
 * Defines destruction and fracture behavior for objects.
 */
UCLASS(BlueprintType)
class UETPFCORE_API UDamageSpec : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDamageSpec();

	/** Unique stable identifier for this spec */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FDamageSpecId SpecId;

	/** Human-readable display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	//--- Fracture Thresholds ---
	
	/** Minimum impact energy (Joules) to cause any damage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fracture", meta = (ClampMin = "0.0"))
	float ImpactThresholdMin = 100.0f;

	/** Impact energy for maximum damage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fracture", meta = (ClampMin = "0.0"))
	float ImpactThresholdMax = 1000.0f;

	/** Multiplier for fracture energy calculation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fracture", meta = (ClampMin = "0.0"))
	float FractureEnergyScale = 1.0f;

	//--- Debris Behavior ---
	
	/** Velocity threshold below which debris goes to sleep (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris", meta = (ClampMin = "0.0"))
	float SleepVelocityThreshold = 5.0f;

	/** Time debris must be below threshold before sleeping (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris", meta = (ClampMin = "0.0"))
	float SleepTimeThreshold = 1.0f;

	/** Maximum lifetime for debris before forced cleanup (seconds, 0 = infinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris", meta = (ClampMin = "0.0"))
	float MaxDebrisLifetime = 300.0f;

	//--- FX ---
	
	/** FX profile for destruction effects */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FFXProfileId FXProfileId;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

//=============================================================================
// FX PROFILE - Sound/VFX mapping
//=============================================================================

/**
 * Maps surface/event types to VFX and audio assets.
 */
UCLASS(BlueprintType)
class UETPFCORE_API UFXProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFXProfile();

	/** Unique stable identifier for this spec */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FFXProfileId SpecId;

	/** Human-readable display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	//--- Impact FX ---
	
	/** Niagara system for impacts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	TSoftObjectPtr<UNiagaraSystem> ImpactVFX;

	/** Sound for impacts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	TSoftObjectPtr<USoundBase> ImpactSound;

	/** Decal material for impact marks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	TSoftObjectPtr<UMaterialInterface> ImpactDecal;

	//--- Footstep FX ---
	
	/** Niagara system for footsteps */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	TSoftObjectPtr<UNiagaraSystem> FootstepVFX;

	/** Sound for footsteps */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	TSoftObjectPtr<USoundBase> FootstepSound;

	/** Decal material for footprints */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	TSoftObjectPtr<UMaterialInterface> FootprintDecal;

	//--- Tire/Track FX ---
	
	/** Niagara system for tire spray */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tire")
	TSoftObjectPtr<UNiagaraSystem> TireSprayVFX;

	/** Sound for tire rolling */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tire")
	TSoftObjectPtr<USoundBase> TireRollSound;

	/** Sound for tire slip/skid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tire")
	TSoftObjectPtr<USoundBase> TireSlipSound;

	//--- Destruction FX ---
	
	/** Niagara system for destruction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destruction")
	TSoftObjectPtr<UNiagaraSystem> DestructionVFX;

	/** Sound for destruction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destruction")
	TSoftObjectPtr<USoundBase> DestructionSound;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

//=============================================================================
// RUNTIME SPEC STRUCTS (SpecPack / JSON loaded)
//=============================================================================

USTRUCT(BlueprintType)
struct FTemperatureResponseLUT
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float MinTempK = 200.f;

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float MaxTempK = 350.f;

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	TArray<float> Samples; // e.g. 16 entries
};

/**
 * Runtime surface spec data (loaded from SpecPack/JSON).
 * POD struct - no UObject references, thread-safe, deterministic.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FRuntimeSurfaceSpec
{
	GENERATED_BODY()

	//--- Identity (required for hashing/migration) ---
	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	FName SpecId;

	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	int32 Version = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	FString DisplayName;

	//--- Friction ---
	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float StaticFriction = 0.8f;

	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float DynamicFriction = 0.6f;

	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float Restitution = 0.2f;

	/** Wetness friction multiplier (1.0 = no change when wet) */
	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float WetFrictionMultiplier = 0.7f;

	//--- Deformation --- how they combine: (strength gates + rate integrates)
	/** How easily surface yields under load (unitless, 0=rigid, 1=very soft) */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float DeformationStrength = 0.0f;

	/** Time-based creep rate (cm/s under sustained load) - multiply by dt */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float DeformationRatePerS = 0.0f;

	/** Maximum deformation depth (cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float MaxDeformationDepthCm = 0.0f;

	/** Time-based recovery rate (1/s) - multiply by dt */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float RecoveryRatePerS = 0.0f;

	//--- Vehicle/Character response ---
	UPROPERTY(BlueprintReadOnly, Category = "Response")
	float RollingResistance = 0.01f;

	UPROPERTY(BlueprintReadOnly, Category = "Response")
	float FootstepImpulseDamping = 0.0f;

	//--- Thermal ---
	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float ThermalConductivityWmK = 1.0f;    // W/(m·K) proxy

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float HeatCapacityJkgK = 1000.0f;        // J/(kg·K) proxy

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float Emissivity01 = 0.9f;

	// --- Temperature response ---
	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	bool bHasTemperatureResponse = false;

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	FTemperatureResponseLUT TempFrictionLUT;

	//--- Flags ---
	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bIsDeformable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bIsSlippery = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bAffectedByWetness = true;

	FRuntimeSurfaceSpec() = default;

	/** Clamp all values to valid ranges. Returns true if any value was modified. */
	bool ValidateAndClamp()
	{
		bool bModified = false;
		auto Clamp = [&bModified](float& Val, float Min, float Max) {
			float Clamped = FMath::Clamp(Val, Min, Max);
			if (Clamped != Val) { Val = Clamped; bModified = true; }
		};
		Clamp(StaticFriction, 0.0f, 2.0f);
		Clamp(DynamicFriction, 0.0f, 2.0f);
		Clamp(Restitution, 0.0f, 1.0f);
		Clamp(WetFrictionMultiplier, 0.0f, 2.0f);
		Clamp(DeformationStrength, 0.0f, 1.0f);
		Clamp(DeformationRatePerS, 0.0f, 10.0f);
		Clamp(MaxDeformationDepthCm, 0.0f, 100.0f);
		Clamp(RecoveryRatePerS, 0.0f, 10.0f);
		Clamp(RollingResistance, 0.0f, 1.0f);
		Clamp(FootstepImpulseDamping, 0.0f, 1.0f);
		Clamp(ThermalConductivityWmK, 0.0f, 1000.0f);
		Clamp(HeatCapacityJkgK, 0.0f, 10000.0f);
		return bModified;
	}
};

/**
 * Runtime medium spec data (loaded from SpecPack/JSON).
 * POD struct - no UObject references, thread-safe, deterministic.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FRuntimeMediumSpec
{
	GENERATED_BODY()

	//--- Identity (required for hashing/migration) ---
	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	FName SpecId;

	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	int32 Version = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	FString DisplayName;

	//--- Physical Properties ---
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float Density = 1.225f;           // kg/m³ (Air ~1.225, Water ~1000, Vacuum ~0)

	/** Linear drag coefficient (low speed drag, proportional to v) */
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float LinearDragCoeff = 0.1f;

	/** Quadratic drag coefficient (high speed drag, proportional to v²) */
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float QuadraticDragCoeff = 0.01f;

	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float Viscosity = 1.8e-5f;        // kg/(m·s) (Air ~1.8e-5, Water ~1e-3)

	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float PressurePa = 101325.0f;     // Pascal (Earth sea level ~101325)

	//--- Thermal ---
	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float TemperatureK = 288.0f;      // Kelvin baseline (Earth ~288K / 15°C)

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float ThermalConductivityWmK = 0.026f; // W/(m·K) (Air ~0.026)

	UPROPERTY(BlueprintReadOnly, Category = "Thermal")
	float HeatCapacityJkgK = 1005.0f;   // J/(kg·K) (Air ~1005)

	// 	/** Solar irradiance W/m^2 proxy (Earth afternoon ~600-900, TOA ~1361) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar")
	float SolarIrradiance_Wm2 = 800.0f;

	// 	/** Unit vector direction FROM world toward sun (or “sun direction”). Must be normalized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solar")
	FVector SunDirection = FVector(1, 0, 0);

	//--- Gravity ---
	/** Full gravity vector (cm/s²) - supports rotated/tilted environments */
	UPROPERTY(BlueprintReadOnly, Category = "Dynamics")
	FVector Gravity = FVector(0.0f, 0.0f, -980.0f);

	//--- Audio Properties ---
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float SpeedOfSound = 343.0f;      // m/s (Air ~343, Water ~1480)

	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float AbsorptionCoefficient = 0.01f;

	FRuntimeMediumSpec() = default;

	/** Clamp all values to valid ranges. Returns true if any value was modified. */
	bool ValidateAndClamp()
	{
		bool bModified = false;
		auto Clamp = [&bModified](float& Val, float Min, float Max) {
			float Clamped = FMath::Clamp(Val, Min, Max);
			if (Clamped != Val) { Val = Clamped; bModified = true; }
		};
		Clamp(Density, 0.0f, 2000.0f);         // Up to ~2x water density
		Clamp(LinearDragCoeff, 0.0f, 10.0f);
		Clamp(QuadraticDragCoeff, 0.0f, 10.0f);
		Clamp(Viscosity, 0.0f, 100.0f);
		Clamp(PressurePa, 0.0f, 1e8f);         // Up to ~1000 atm
		Clamp(TemperatureK, 0.0f, 10000.0f);   // Up to stellar temperatures
		Clamp(ThermalConductivityWmK, 0.0f, 1000.0f);
		Clamp(SpeedOfSound, 0.0f, 10000.0f);
		Clamp(AbsorptionCoefficient, 0.0f, 1.0f);
		return bModified;
	}
};

//=============================================================================
// RUNTIME STATE STRUCTS
//=============================================================================

/**
 * Runtime surface state at a contact point.
 * This is the "unified query result" consumed by vehicles, characters, FX, etc.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FSurfaceState
{
	GENERATED_BODY()

	/** The spec ID for this surface */
	UPROPERTY(BlueprintReadOnly, Category = "Surface")
	FSurfaceSpecId SpecId;

	/** Effective static friction (after wetness/temperature modifiers) */
	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float FrictionStatic = 0.8f;

	/** Effective dynamic friction (after wetness/temperature modifiers) */
	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float FrictionDynamic = 0.6f;

	/** Effective restitution */
	UPROPERTY(BlueprintReadOnly, Category = "Friction")
	float Restitution = 0.2f;

	/** Effective compliance */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float Compliance = 0.0f;

	/** Effective deformation strength */
	UPROPERTY(BlueprintReadOnly, Category = "Deformation")
	float DeformationStrength = 0.0f;

	/** Current wetness level (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Environment")
	float Wetness = 0.0f;

	/** Current snow depth (cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Environment")
	float SnowDepth = 0.0f;

	/** Current compaction level (0-1, for snow/sand) */
	UPROPERTY(BlueprintReadOnly, Category = "Environment")
	float Compaction = 0.0f;

	/** Current temperature (Kelvin) */
	UPROPERTY(BlueprintReadOnly, Category = "Environment")
	float Temperature = 288.0f;

	/** FX profile to use for this surface */
	UPROPERTY(BlueprintReadOnly, Category = "FX")
	FFXProfileId FXProfileId;

	/** Whether this state is valid */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsValid = false;

	FSurfaceState() = default;
};

/**
 * Runtime environment context at a location.
 * Computed from MediumSpec + local conditions.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FEnvironmentContext
{
	GENERATED_BODY()

	/** The medium spec ID at this location */
	UPROPERTY(BlueprintReadOnly, Category = "Medium")
	FMediumSpecId MediumSpecId;

	/** Effective density (kg/m³) */
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float Density = 1.225f;

	/** Effective viscosity */
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float Viscosity = 0.001f;

	/** Effective pressure (kPa) */
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float Pressure = 101.325f;

	/** Effective temperature (Kelvin) */
	UPROPERTY(BlueprintReadOnly, Category = "Physical")
	float Temperature = 288.0f;

	/** Effective gravity vector (cm/s²) */
	UPROPERTY(BlueprintReadOnly, Category = "Dynamics")
	FVector Gravity = FVector(0.0f, 0.0f, -980.0f);

	/** Wind velocity at this location (cm/s) */
	UPROPERTY(BlueprintReadOnly, Category = "Dynamics")
	FVector WindVelocity = FVector::ZeroVector;

	/** Effective speed of sound (m/s) */
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float SpeedOfSound = 343.0f;

	/** Effective sound attenuation multiplier */
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
	float SoundAttenuation = 1.0f;

	/** Whether this context is valid */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsValid = false;

	FEnvironmentContext() = default;
};
