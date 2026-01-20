// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/EnvironmentSubsystem.h"
#include "GlobalAtmosphereField.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"

// Static fallback medium spec - Earth sea-level atmosphere (never fails)
FRuntimeMediumSpec UEnvironmentSubsystem::FallbackMediumSpec = []()
{
	FRuntimeMediumSpec Spec;
	Spec.DisplayName = TEXT("Earth Atmosphere");
	Spec.Density = 1.225f;           // kg/m³ at sea level
	Spec.QuadraticDragCoeff = 0.5f;     // typical
	Spec.Viscosity = 1.8e-5f;        // kg/(m·s) at 15°C
	Spec.SpeedOfSound = 343.0f;      // m/s at 20°C
	Spec.AbsorptionCoefficient = 0.01f;
	return Spec;
}();

//=============================================================================
// UEnvironmentVolumeComponent
//=============================================================================

UEnvironmentVolumeComponent::UEnvironmentVolumeComponent()
{
	// Set defaults that are safe during CDO construction
	// Use InitBoxExtent for default size (safe during CDO)
	InitBoxExtent(FVector(500.0f, 500.0f, 300.0f));
	
	// These property assignments are safe during CDO construction
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	
	// Make it visible in editor but not in game
	bHiddenInGame = true;
	bVisibleInReflectionCaptures = false;
}

//=============================================================================
// UEnvironmentSubsystem
//=============================================================================

UEnvironmentSubsystem::UEnvironmentSubsystem()
	: DefaultMediumSpec(nullptr)
{
}

void UEnvironmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Log, TEXT("EnvironmentSubsystem initialized for world: %s"), 
		*GetWorld()->GetName());
}

void UEnvironmentSubsystem::Deinitialize()
{
	MediumSpecMap.Empty();
	RuntimeMediumSpecs.Empty();
	DefaultMediumSpec = nullptr;
	RegisteredVolumes.Empty();
	
	Super::Deinitialize();
}

bool UEnvironmentSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

//=============================================================================
// ENVIRONMENT QUERIES
//=============================================================================

FEnvironmentContext UEnvironmentSubsystem::GetEnvironmentAtLocation(const FVector& WorldLocation) const
{
	// Find the volume at this location (if any)
	UEnvironmentVolumeComponent* Volume = FindVolumeAtLocation(WorldLocation);
	
	// Priority order:
	// 1. Volume with MediumSpec (local override)
	// 2. GlobalAtmosphereField (altitude-based default)
	// 3. DefaultMediumSpec (fallback)
	
	// If inside a volume, use volume's spec
	if (Volume && Volume->MediumSpecId.IsValid())
	{
		const UMediumSpec* Spec = GetMediumSpec(Volume->MediumSpecId);
		if (Spec)
		{
			return BuildEnvironmentContext(Spec, Volume);
		}
	}
	
	// Not in a volume - use GlobalAtmosphereField if available
	if (GlobalAtmosphereField)
	{
		// Get altitude-based environment from the atmosphere field
		return GlobalAtmosphereField->CreateEnvironmentContextAtLocation(WorldLocation);
	}

	// Final fallback to default medium spec
	if (DefaultMediumSpec)
	{
		return BuildEnvironmentContext(DefaultMediumSpec, nullptr);
	}

	// No environment data available
	return FEnvironmentContext();
}

FMediumSpecId UEnvironmentSubsystem::GetMediumAtLocation(const FVector& WorldLocation) const
{
	UEnvironmentVolumeComponent* Volume = FindVolumeAtLocation(WorldLocation);
	
	if (Volume && Volume->MediumSpecId.IsValid())
	{
		return Volume->MediumSpecId;
	}
	
	if (DefaultMediumSpec)
	{
		return DefaultMediumSpec->SpecId;
	}
	
	return FMediumSpecId();
}

bool UEnvironmentSubsystem::IsVacuumAtLocation(const FVector& WorldLocation) const
{
	FEnvironmentContext Context = GetEnvironmentAtLocation(WorldLocation);
	return Context.bIsValid && Context.Density < VacuumDensityThreshold;
}

bool UEnvironmentSubsystem::IsUnderwaterAtLocation(const FVector& WorldLocation) const
{
	FEnvironmentContext Context = GetEnvironmentAtLocation(WorldLocation);
	return Context.bIsValid && Context.Density > WaterDensityThreshold;
}

float UEnvironmentSubsystem::GetSoundAttenuationAtLocation(const FVector& WorldLocation) const
{
	FEnvironmentContext Context = GetEnvironmentAtLocation(WorldLocation);
	return Context.bIsValid ? Context.SoundAttenuation : 1.0f;
}

FVector UEnvironmentSubsystem::CalculateDragForce(const FVector& WorldLocation, const FVector& Velocity, 
	float DragArea, float DragCoefficient) const
{
	FEnvironmentContext Context = GetEnvironmentAtLocation(WorldLocation);
	
	if (!Context.bIsValid || Context.Density < VacuumDensityThreshold)
	{
		// No drag in vacuum
		return FVector::ZeroVector;
	}

	// Calculate speed
	float Speed = Velocity.Size();
	if (Speed < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	// Convert units: 
	// Density: kg/m³
	// Area: cm² → m² (divide by 10000)
	// Speed: cm/s → m/s (divide by 100)
	// Force: N → UE force units (multiply by 100 for cm-based physics)
	
	float DensityKgM3 = Context.Density;
	float AreaM2 = DragArea / 10000.0f;
	float SpeedMS = Speed / 100.0f;
	
	// Drag force magnitude: F = 0.5 * ρ * v² * Cd * A
	float DragMagnitudeN = 0.5f * DensityKgM3 * SpeedMS * SpeedMS * DragCoefficient * AreaM2;
	
	// Convert to UE units (N → UE force, roughly: 1N ≈ 100 UE force units for cm-based sim)
	float DragMagnitudeUE = DragMagnitudeN * 100.0f;
	
	// Drag opposes velocity
	FVector DragDirection = -Velocity.GetSafeNormal();
	
	return DragDirection * DragMagnitudeUE;
}

FVector UEnvironmentSubsystem::CalculateBuoyancyForce(const FVector& WorldLocation, float DisplacedVolume) const
{
	FEnvironmentContext Context = GetEnvironmentAtLocation(WorldLocation);
	
	if (!Context.bIsValid || Context.Density < VacuumDensityThreshold)
	{
		// No buoyancy in vacuum
		return FVector::ZeroVector;
	}

	// Convert units:
	// Volume: cm³ → m³ (divide by 1000000)
	// Density: kg/m³
	// Gravity: cm/s² (already in UE units)
	// Force: need to convert to UE force units
	
	float VolumeM3 = DisplacedVolume / 1000000.0f;
	float DensityKgM3 = Context.Density;
	
	// Buoyancy force magnitude: F = ρ * V * g
	// Using gravity magnitude (positive)
	float GravityMagnitude = Context.Gravity.Size();
	float BuoyancyMagnitudeN = DensityKgM3 * VolumeM3 * (GravityMagnitude / 100.0f); // gravity in m/s²
	
	// Convert to UE units
	float BuoyancyMagnitudeUE = BuoyancyMagnitudeN * 100.0f;
	
	// Buoyancy opposes gravity
	FVector BuoyancyDirection = -Context.Gravity.GetSafeNormal();
	
	return BuoyancyDirection * BuoyancyMagnitudeUE;
}

//=============================================================================
// SPEC REGISTRATION
//=============================================================================

void UEnvironmentSubsystem::RegisterMediumSpec(const FMediumSpecId& SpecId, UMediumSpec* Spec)
{
	if (SpecId.IsValid() && Spec)
	{
		MediumSpecMap.Add(SpecId.Id, Spec);
		
		UE_LOG(LogTemp, Verbose, TEXT("Registered MediumSpec: %s"), *SpecId.Id.ToString());
	}
}

UMediumSpec* UEnvironmentSubsystem::GetMediumSpec(const FMediumSpecId& SpecId) const
{
	if (!SpecId.IsValid())
	{
		return DefaultMediumSpec;
	}

	UMediumSpec* const* FoundSpec = MediumSpecMap.Find(SpecId.Id);
	if (FoundSpec && *FoundSpec)
	{
		return *FoundSpec;
	}

	return DefaultMediumSpec;
}

void UEnvironmentSubsystem::SetDefaultMediumSpec(UMediumSpec* Spec)
{
	DefaultMediumSpec = Spec;
	
	if (Spec)
	{
		UE_LOG(LogTemp, Log, TEXT("Default MediumSpec set to: %s"), *Spec->SpecId.Id.ToString());
	}
}

void UEnvironmentSubsystem::SetGlobalAtmosphereField(UGlobalAtmosphereField* AtmosphereField)
{
	GlobalAtmosphereField = AtmosphereField;
	
	if (AtmosphereField)
	{
		UE_LOG(LogTemp, Log, TEXT("GlobalAtmosphereField set - altitude-based environment now active"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("GlobalAtmosphereField cleared - using DefaultMediumSpec only"));
	}
}

void UEnvironmentSubsystem::RegisterVolume(UEnvironmentVolumeComponent* Volume)
{
	if (Volume && !RegisteredVolumes.Contains(Volume))
	{
		RegisteredVolumes.Add(Volume);
		
		UE_LOG(LogTemp, Verbose, TEXT("Registered EnvironmentVolume with MediumSpec: %s"), 
			*Volume->MediumSpecId.Id.ToString());
	}
}

void UEnvironmentSubsystem::UnregisterVolume(UEnvironmentVolumeComponent* Volume)
{
	RegisteredVolumes.Remove(Volume);
}

//=============================================================================
// INTERNAL
//=============================================================================

UEnvironmentVolumeComponent* UEnvironmentSubsystem::FindVolumeAtLocation(const FVector& WorldLocation) const
{
	UEnvironmentVolumeComponent* BestVolume = nullptr;
	int32 BestPriority = INT_MIN;

	for (UEnvironmentVolumeComponent* Volume : RegisteredVolumes)
	{
		if (!Volume || !IsValid(Volume))
		{
			continue;
		}

		// Check if location is inside this volume
		FVector ClosestPoint;
		float DistSq = Volume->GetSquaredDistanceToCollision(WorldLocation, DistSq, ClosestPoint);
		
		// If distance is 0 or very small, we're inside
		if (DistSq < 1.0f)
		{
			if (Volume->Priority > BestPriority)
			{
				BestPriority = Volume->Priority;
				BestVolume = Volume;
			}
		}
	}

	return BestVolume;
}

FEnvironmentContext UEnvironmentSubsystem::BuildEnvironmentContext(const UMediumSpec* Spec, 
	const UEnvironmentVolumeComponent* Volume) const
{
	FEnvironmentContext Context;
	
	if (!Spec)
	{
		return Context;
	}

	// Copy base properties from spec
	Context.MediumSpecId = Spec->SpecId;
	Context.Density = Spec->Density;
	Context.Viscosity = Spec->ViscosityProxy;
	Context.Pressure = Spec->Pressure;
	Context.Temperature = Spec->TemperatureK;
	Context.Gravity = Spec->GravityVector;
	Context.SpeedOfSound = Spec->SpeedOfSound;
	Context.SoundAttenuation = Spec->SoundAttenuationMultiplier;

	// Apply volume overrides if present
	if (Volume)
	{
		if (Volume->TemperatureOverride > 0.0f)
		{
			Context.Temperature = Volume->TemperatureOverride;
		}
		
		if (Volume->PressureOverride > 0.0f)
		{
			Context.Pressure = Volume->PressureOverride;
		}
		
		Context.WindVelocity = Volume->WindVelocity;
	}

	// Adjust sound attenuation based on density
	if (Context.Density < VacuumDensityThreshold)
	{
		// True vacuum - no sound propagation
		Context.SoundAttenuation = 0.0f;
		Context.SpeedOfSound = 0.0f;
	}
	else if (Context.Density < 0.1f)
	{
		// Thin atmosphere - reduced sound
		float AtmosphereFactor = Context.Density / 1.225f; // Relative to Earth sea level
		Context.SoundAttenuation *= FMath::Sqrt(AtmosphereFactor);
	}

	Context.bIsValid = true;
	return Context;
}

//=============================================================================
// RUNTIME SPEC REGISTRATION (SpecPack / JSON)
//=============================================================================

void UEnvironmentSubsystem::RegisterRuntimeMediumSpec(const FMediumSpecId& Id, const FRuntimeMediumSpec & Spec)
{
	RuntimeMediumSpecs.Add(Id.Id, Spec);
	
	UE_LOG(LogTemp, Verbose, TEXT("Registered runtime MediumSpec: %s"), *Id.Id.ToString());
}

bool UEnvironmentSubsystem::ResolveMediumSpec(const FMediumSpecId& Id, FRuntimeMediumSpec & OutSpec) const
{
	// Resolution order: Runtime registry → DataAsset fallback → Default

	// 1. Check runtime registry first (SpecPack / JSON loaded)
	if (const FRuntimeMediumSpec * RuntimeSpec = RuntimeMediumSpecs.Find(Id.Id))
	{
		OutSpec = *RuntimeSpec;
		return true;
	}

	// 2. Check DataAsset registry (editor workflow)
	// Currently MediumSpecMap maps FName → UMediumSpec*, so ID lookup is available
	// But we can't easily convert UMediumSpec* to FRuntimeMediumSpec  without the struct
	// TODO: Add struct conversion or cache structs from DataAssets

	// 3. Return fallback (never fails)
	OutSpec = FallbackMediumSpec;
	return false;
}

bool UEnvironmentSubsystem::HasRuntimeMediumSpec(const FMediumSpecId& Id) const
{
	return RuntimeMediumSpecs.Contains(Id.Id);
}

TArray<FMediumSpecId> UEnvironmentSubsystem::GetAllRuntimeMediumSpecIds() const
{
	TArray<FMediumSpecId> Result;
	Result.Reserve(RuntimeMediumSpecs.Num());
	
	for (const auto& Pair : RuntimeMediumSpecs)
	{
		FMediumSpecId Id;
		Id.Id = Pair.Key;
		Result.Add(Id);
	}
	
	return Result;
}

void UEnvironmentSubsystem::ClearRuntimeMediumSpecs()
{
	RuntimeMediumSpecs.Empty();
	UE_LOG(LogTemp, Log, TEXT("Cleared all runtime MediumSpecs"));
}

const FRuntimeMediumSpec & UEnvironmentSubsystem::GetFallbackMediumSpec()
{
	return FallbackMediumSpec;
}
