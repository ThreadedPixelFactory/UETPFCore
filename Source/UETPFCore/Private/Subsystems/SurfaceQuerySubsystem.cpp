// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/SurfaceQuerySubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

// Static fallback spec - hardcoded realistic defaults that never fail
FRuntimeSurfaceSpec USurfaceQuerySubsystem::FallbackSurfaceSpec = []()
{
	FRuntimeSurfaceSpec Spec;
	Spec.DisplayName = TEXT("Default");
	Spec.StaticFriction = 0.7f;
	Spec.DynamicFriction = 0.5f;
	Spec.Restitution = 0.3f;
	Spec.bIsDeformable = false;
	Spec.bIsSlippery = false;
	Spec.bAffectedByWetness = false;
	Spec.ThermalConductivityWmK = 0.026f; // air baseline
	Spec.HeatCapacityJkgK = 1005.0f; // air baseline
	Spec.Emissivity01 = 0.9f;
	return Spec;
}();

USurfaceQuerySubsystem::USurfaceQuerySubsystem()
	: DefaultSurfaceSpec(nullptr)
	, SurfaceTraceChannel(ECC_Visibility)
{
}

void USurfaceQuerySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Default trace channel for surface queries
	SurfaceTraceChannel = ECC_Visibility;
	
	UE_LOG(LogTemp, Log, TEXT("SurfaceQuerySubsystem initialized for world: %s"), 
		*GetWorld()->GetName());
}

void USurfaceQuerySubsystem::Deinitialize()
{
	SurfaceSpecMap.Empty();
	RuntimeSurfaceSpecs.Empty();
	DefaultSurfaceSpec = nullptr;
	
	Super::Deinitialize();
}

bool USurfaceQuerySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Create for all game worlds, not for editor preview worlds
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

//=============================================================================
// SURFACE QUERIES
//=============================================================================

FSurfaceState USurfaceQuerySubsystem::GetSurfaceStateFromHit(const FHitResult& HitResult) const
{
	FSurfaceState Result;
	
	if (!HitResult.bBlockingHit)
	{
		return Result;
	}

	// Get the physical material from the hit
	UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get();
	
	// Look up the surface spec
	const USurfaceSpec* Spec = GetSurfaceSpecForMaterial(PhysMat);
	if (!Spec)
	{
		Spec = DefaultSurfaceSpec;
	}

	if (Spec)
	{
		Result = BuildSurfaceState(Spec, HitResult.ImpactPoint);
	}

	return Result;
}

FSurfaceState USurfaceQuerySubsystem::GetSurfaceStateAtLocation(const FVector& WorldLocation, float TraceDistance) const
{
	FSurfaceState Result;
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}

	// Perform downward trace
	FHitResult HitResult;
	FVector Start = WorldLocation;
	FVector End = WorldLocation - FVector(0.0f, 0.0f, TraceDistance);
	
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;
	QueryParams.bTraceComplex = false;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, SurfaceTraceChannel, QueryParams))
	{
		return GetSurfaceStateFromHit(HitResult);
	}

	return Result;
}

TArray<FSurfaceState> USurfaceQuerySubsystem::BatchGetSurfaceStates(const TArray<FVector>& WorldLocations, float TraceDistance) const
{
	TArray<FSurfaceState> Results;
	Results.Reserve(WorldLocations.Num());

	for (const FVector& Location : WorldLocations)
	{
		Results.Add(GetSurfaceStateAtLocation(Location, TraceDistance));
	}

	return Results;
}

//=============================================================================
// SPEC REGISTRATION
//=============================================================================

void USurfaceQuerySubsystem::RegisterSurfaceSpec(UPhysicalMaterial* PhysMat, USurfaceSpec* Spec)
{
	if (PhysMat && Spec)
	{
		SurfaceSpecMap.Add(PhysMat, Spec);
		
		UE_LOG(LogTemp, Verbose, TEXT("Registered SurfaceSpec '%s' for PhysicalMaterial '%s'"),
			*Spec->SpecId.Id.ToString(), *PhysMat->GetName());
	}
}

USurfaceSpec* USurfaceQuerySubsystem::GetSurfaceSpecForMaterial(UPhysicalMaterial* PhysMat) const
{
	if (!PhysMat)
	{
		return DefaultSurfaceSpec;
	}

	USurfaceSpec* const* FoundSpec = SurfaceSpecMap.Find(PhysMat);
	if (FoundSpec && *FoundSpec)
	{
		return *FoundSpec;
	}

	return DefaultSurfaceSpec;
}

void USurfaceQuerySubsystem::SetDefaultSurfaceSpec(USurfaceSpec* Spec)
{
	DefaultSurfaceSpec = Spec;
	
	if (Spec)
	{
		UE_LOG(LogTemp, Log, TEXT("Default SurfaceSpec set to: %s"), *Spec->SpecId.Id.ToString());
	}
}

//=============================================================================
// DELTA INTEGRATION (base implementations - override for real delta storage)
//=============================================================================

float USurfaceQuerySubsystem::GetWetnessAtLocation(const FVector& WorldLocation) const
{
	// Base implementation returns 0 (dry)
	// Override in derived class to integrate with SurfaceDeltaSubsystem
	return 0.0f;
}

float USurfaceQuerySubsystem::GetSnowDepthAtLocation(const FVector& WorldLocation) const
{
	// Base implementation returns 0 (no snow)
	// Override in derived class to integrate with SurfaceDeltaSubsystem
	return 0.0f;
}

float USurfaceQuerySubsystem::GetCompactionAtLocation(const FVector& WorldLocation) const
{
	// Base implementation returns 0 (no compaction)
	// Override in derived class to integrate with SurfaceDeltaSubsystem
	return 0.0f;
}

float USurfaceQuerySubsystem::GetTemperatureAtLocation(const FVector& WorldLocation) const
{
	// Base implementation returns Earth standard temperature (288K / 15°C)
	// Override in derived class to integrate with EnvironmentSubsystem
	return 288.0f;
}

//=============================================================================
// INTERNAL
//=============================================================================

FSurfaceState USurfaceQuerySubsystem::BuildSurfaceState(const USurfaceSpec* Spec, const FVector& Location) const
{
	FSurfaceState State;
	
	if (!Spec)
	{
		return State;
	}

	// Copy base properties from spec
	State.SpecId = Spec->SpecId;
	State.FrictionStatic = Spec->FrictionStatic;
	State.FrictionDynamic = Spec->FrictionDynamic;
	State.Restitution = Spec->Restitution;
	State.Compliance = Spec->Compliance;
	State.DeformationStrength = Spec->DeformationStrength;
	State.FXProfileId = Spec->FXProfileId;

	// Get environmental conditions at this location
	State.Wetness = GetWetnessAtLocation(Location);
	State.SnowDepth = GetSnowDepthAtLocation(Location);
	State.Compaction = GetCompactionAtLocation(Location);
	State.Temperature = GetTemperatureAtLocation(Location);

	// Apply environmental modifiers to friction
	ApplyEnvironmentalModifiers(State, Spec);

	State.bIsValid = true;
	return State;
}

void USurfaceQuerySubsystem::ApplyEnvironmentalModifiers(FSurfaceState& State, const USurfaceSpec* Spec) const
{
	if (!Spec)
	{
		return;
	}

	float FrictionMultiplier = 1.0f;

	// Apply wetness response curve
	if (State.Wetness > 0.0f)
	{
		const FRichCurve* WetnessCurve = Spec->WetnessResponseCurve.GetRichCurveConst();
		if (WetnessCurve && WetnessCurve->GetNumKeys() > 0)
		{
			FrictionMultiplier *= WetnessCurve->Eval(State.Wetness);
		}
	}

	// Apply temperature response curve
	{
		const FRichCurve* TempCurve = Spec->TemperatureResponseCurve.GetRichCurveConst();
		if (TempCurve && TempCurve->GetNumKeys() > 0)
		{
			FrictionMultiplier *= TempCurve->Eval(State.Temperature);
		}
	}

	// Apply snow depth effect (deeper snow = more resistance but less friction)
	if (State.SnowDepth > 0.0f)
	{
		// Snow reduces friction but increases compliance
		float SnowFactor = FMath::Clamp(State.SnowDepth / 50.0f, 0.0f, 1.0f); // 50cm = full effect
		FrictionMultiplier *= FMath::Lerp(1.0f, 0.5f, SnowFactor);
		State.Compliance = FMath::Lerp(State.Compliance, 0.8f, SnowFactor);
		State.DeformationStrength = FMath::Lerp(State.DeformationStrength, 0.9f, SnowFactor);
	}

	// Apply compaction effect (compacted surfaces have more friction)
	if (State.Compaction > 0.0f)
	{
		// Compacted snow/sand is more stable
		float CompactionBonus = State.Compaction * 0.3f; // Up to 30% more friction when fully compacted
		FrictionMultiplier *= (1.0f + CompactionBonus);
		State.Compliance *= (1.0f - State.Compaction * 0.5f); // Less compliance when compacted
	}

	// Apply final friction multiplier
	State.FrictionStatic *= FrictionMultiplier;
	State.FrictionDynamic *= FrictionMultiplier;

	// Clamp final values to reasonable ranges
	State.FrictionStatic = FMath::Clamp(State.FrictionStatic, 0.05f, 2.0f);
	State.FrictionDynamic = FMath::Clamp(State.FrictionDynamic, 0.02f, 1.5f);
	State.Compliance = FMath::Clamp(State.Compliance, 0.0f, 1.0f);
	State.DeformationStrength = FMath::Clamp(State.DeformationStrength, 0.0f, 1.0f);
}

//=============================================================================
// RUNTIME SPEC REGISTRATION (SpecPack / JSON)
//=============================================================================

void USurfaceQuerySubsystem::RegisterRuntimeSurfaceSpec(const FSurfaceSpecId& Id, const FRuntimeSurfaceSpec& Spec)
{
	RuntimeSurfaceSpecs.Add(Id.Id, Spec);
	
	UE_LOG(LogTemp, Verbose, TEXT("Registered runtime SurfaceSpec: %s"), *Id.Id.ToString());
}

bool USurfaceQuerySubsystem::ResolveSurfaceSpec(const FSurfaceSpecId& Id, FRuntimeSurfaceSpec& OutSpec) const
{
	// Resolution order: Runtime registry → DataAsset fallback → Default

	// 1. Check runtime registry first (SpecPack / JSON loaded)
	if (const FRuntimeSurfaceSpec* RuntimeSpec = RuntimeSurfaceSpecs.Find(Id.Id))
	{
		OutSpec = *RuntimeSpec;
		return true;
	}

	// 2. Check DataAsset registry (editor workflow)
	// Would need to iterate SurfaceSpecMap to find by ID, but that maps PhysMat -> Spec
	// For now, DataAsset lookup by ID is not supported in the current design
	// TODO: Add TMap<FSurfaceSpecId, USurfaceSpec*> for asset-based ID lookup if needed

	// 3. Return fallback (never fails)
	OutSpec = FallbackSurfaceSpec;
	return false;
}

bool USurfaceQuerySubsystem::HasRuntimeSpec(const FSurfaceSpecId& Id) const
{
	return RuntimeSurfaceSpecs.Contains(Id.Id);
}

TArray<FSurfaceSpecId> USurfaceQuerySubsystem::GetAllRuntimeSpecIds() const
{
	TArray<FSurfaceSpecId> Result;
	Result.Reserve(RuntimeSurfaceSpecs.Num());
	
	for (const auto& Pair : RuntimeSurfaceSpecs)
	{
		FSurfaceSpecId Id;
		Id.Id = Pair.Key;
		Result.Add(Id);
	}
	
	return Result;
}

void USurfaceQuerySubsystem::ClearRuntimeSpecs()
{
	RuntimeSurfaceSpecs.Empty();
	UE_LOG(LogTemp, Log, TEXT("Cleared all runtime SurfaceSpecs"));
}

const FRuntimeSurfaceSpec& USurfaceQuerySubsystem::GetFallbackSpec()
{
	return FallbackSurfaceSpec;
}
