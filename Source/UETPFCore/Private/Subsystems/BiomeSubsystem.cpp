// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/BiomeSubsystem.h"
#include "Subsystems/SurfaceQuerySubsystem.h"
#include "Subsystems/EnvironmentSubsystem.h"
#include "Engine/World.h"
#include "LandscapeComponent.h"
#include "Kismet/KismetSystemLibrary.h"

//=============================================================================
// UBiomeSubsystem
//=============================================================================

UBiomeSubsystem::UBiomeSubsystem()
{
}

void UBiomeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Get references to other subsystems
	SurfaceQuerySubsystem = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
	EnvironmentSubsystem = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();

	UE_LOG(LogTemp, Log, TEXT("BiomeSubsystem initialized for world: %s"), 
		*GetWorld()->GetName());
}

void UBiomeSubsystem::Deinitialize()
{
	BiomeSpecs.Empty();
	DefaultBiomeId = FBiomeId();

	Super::Deinitialize();
}

bool UBiomeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

//=============================================================================
// BIOME QUERIES
//=============================================================================

FBiomeQueryResult UBiomeSubsystem::GetBiomeAtLocation(const FVector& WorldLocation) const
{
	FBiomeQueryResult Result;

	// Calculate terrain properties
	float TerrainHeight = GetTerrainHeightAtLocation(WorldLocation);
	float Altitude = WorldLocation.Z - SeaLevelAltitude;
	float SlopeAngle = GetTerrainSlopeAtLocation(WorldLocation);

	Result.Altitude = Altitude;
	Result.SlopeAngle = SlopeAngle;

	// Try RVT-based lookup first if enabled
	if (bUseRVTBiomeLookup)
	{
		TArray<float> RVTWeights;
		if (SampleRVTAtLocation(WorldLocation, RVTWeights))
		{
			// Find the biome with highest weight
			float MaxWeight = 0.0f;
			float SecondMaxWeight = 0.0f;
			int32 MaxChannel = -1;
			int32 SecondMaxChannel = -1;

			for (int32 i = 0; i < RVTWeights.Num(); ++i)
			{
				if (RVTWeights[i] > MaxWeight)
				{
					SecondMaxWeight = MaxWeight;
					SecondMaxChannel = MaxChannel;
					MaxWeight = RVTWeights[i];
					MaxChannel = i;
				}
				else if (RVTWeights[i] > SecondMaxWeight)
				{
					SecondMaxWeight = RVTWeights[i];
					SecondMaxChannel = i;
				}
			}

			// Map channels to biomes
			if (MaxChannel >= 0)
			{
				for (const auto& Pair : BiomeSpecs)
				{
					if (Pair.Value.RVTChannel == MaxChannel)
					{
						Result.PrimaryBiome = Pair.Value.BiomeId;
						Result.PrimaryWeight = MaxWeight;
						break;
					}
				}

				if (SecondMaxChannel >= 0 && SecondMaxWeight > 0.01f)
				{
					for (const auto& Pair : BiomeSpecs)
					{
						if (Pair.Value.RVTChannel == SecondMaxChannel)
						{
							Result.SecondaryBiome = Pair.Value.BiomeId;
							Result.SecondaryWeight = SecondMaxWeight;
							break;
						}
					}
				}
			}
		}
	}

	// Fall back to rule-based resolution if no RVT result
	if (!Result.PrimaryBiome.IsValid())
	{
		Result.PrimaryBiome = ResolveBiomeFromRules(Altitude, SlopeAngle);
		Result.PrimaryWeight = 1.0f;
	}

	// Look up the spec IDs for the resolved biome
	if (Result.PrimaryBiome.IsValid())
	{
		if (const FBiomeSpec* Spec = GetBiomeSpec(Result.PrimaryBiome))
		{
			Result.SurfaceSpecId = Spec->DefaultSurfaceSpecId;
			Result.MediumSpecId = Spec->DefaultMediumSpecId;
		}
	}
	else if (DefaultBiomeId.IsValid())
	{
		// Use default biome
		Result.PrimaryBiome = DefaultBiomeId;
		if (const FBiomeSpec* Spec = GetBiomeSpec(DefaultBiomeId))
		{
			Result.SurfaceSpecId = Spec->DefaultSurfaceSpecId;
			Result.MediumSpecId = Spec->DefaultMediumSpecId;
		}
	}

	Result.bIsValid = Result.PrimaryBiome.IsValid();
	return Result;
}

FBiomeId UBiomeSubsystem::GetBiomeIdAtLocation(const FVector& WorldLocation) const
{
	FBiomeQueryResult Result = GetBiomeAtLocation(WorldLocation);
	return Result.PrimaryBiome;
}

FSurfaceSpecId UBiomeSubsystem::GetSurfaceSpecForBiome(const FBiomeId& BiomeId) const
{
	if (const FBiomeSpec* Spec = GetBiomeSpec(BiomeId))
	{
		return Spec->DefaultSurfaceSpecId;
	}
	return FSurfaceSpecId();
}

FMediumSpecId UBiomeSubsystem::GetMediumSpecForBiome(const FBiomeId& BiomeId) const
{
	if (const FBiomeSpec* Spec = GetBiomeSpec(BiomeId))
	{
		return Spec->DefaultMediumSpecId;
	}
	return FMediumSpecId();
}

FSurfaceSpecId UBiomeSubsystem::GetSurfaceSpecAtLocation(const FVector& WorldLocation) const
{
	FBiomeQueryResult Result = GetBiomeAtLocation(WorldLocation);
	return Result.SurfaceSpecId;
}

//=============================================================================
// TERRAIN QUERIES
//=============================================================================

float UBiomeSubsystem::GetTerrainHeightAtLocation(const FVector& WorldLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	// Perform a line trace downward to find terrain
	FVector Start = FVector(WorldLocation.X, WorldLocation.Y, WorldLocation.Z + 100000.0f);
	FVector End = FVector(WorldLocation.X, WorldLocation.Y, WorldLocation.Z - 100000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	// Trace for landscape/terrain
	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		// Check if we hit a landscape
		if (HitResult.Component.IsValid())
		{
			return HitResult.Location.Z;
		}
	}

	return 0.0f;
}

float UBiomeSubsystem::GetTerrainSlopeAtLocation(const FVector& WorldLocation) const
{
	FVector Normal = GetTerrainNormalAtLocation(WorldLocation);
	
	// Calculate angle from vertical (Z-up)
	float DotProduct = FVector::DotProduct(Normal, FVector::UpVector);
	float AngleRadians = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

FVector UBiomeSubsystem::GetTerrainNormalAtLocation(const FVector& WorldLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::UpVector;
	}

	// Perform a line trace downward to find terrain normal
	FVector Start = FVector(WorldLocation.X, WorldLocation.Y, WorldLocation.Z + 100000.0f);
	FVector End = FVector(WorldLocation.X, WorldLocation.Y, WorldLocation.Z - 100000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true; // Need complex for accurate normal

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		return HitResult.ImpactNormal;
	}

	return FVector::UpVector;
}

//=============================================================================
// BIOME REGISTRATION
//=============================================================================

void UBiomeSubsystem::RegisterBiomeSpec(const FBiomeSpec& Spec)
{
	if (Spec.BiomeId.IsValid())
	{
		BiomeSpecs.Add(Spec.BiomeId.Id, Spec);

		UE_LOG(LogTemp, Verbose, TEXT("Registered BiomeSpec: %s"), *Spec.BiomeId.Id.ToString());
	}
}

const FBiomeSpec* UBiomeSubsystem::GetBiomeSpec(const FBiomeId& BiomeId) const
{
	if (!BiomeId.IsValid())
	{
		return nullptr;
	}

	return BiomeSpecs.Find(BiomeId.Id);
}

void UBiomeSubsystem::SetDefaultBiome(const FBiomeId& BiomeId)
{
	DefaultBiomeId = BiomeId;

	UE_LOG(LogTemp, Log, TEXT("Default biome set to: %s"), *BiomeId.Id.ToString());
}

TArray<FBiomeId> UBiomeSubsystem::GetAllBiomeIds() const
{
	TArray<FBiomeId> Result;
	Result.Reserve(BiomeSpecs.Num());

	for (const auto& Pair : BiomeSpecs)
	{
		Result.Add(Pair.Value.BiomeId);
	}

	return Result;
}

//=============================================================================
// INTERNAL
//=============================================================================

FBiomeId UBiomeSubsystem::ResolveBiomeFromRules(float Altitude, float SlopeAngle) const
{
	// Score each biome based on how well it matches the rules
	float BestScore = -1.0f;
	FBiomeId BestBiome;

	for (const auto& Pair : BiomeSpecs)
	{
		const FBiomeSpec& Spec = Pair.Value;
		float Score = 0.0f;
		bool bMatches = true;

		// Check altitude range
		if (bUseAltitudeRules)
		{
			if (Altitude < Spec.AltitudeRange.X || Altitude > Spec.AltitudeRange.Y)
			{
				bMatches = false;
			}
			else
			{
				// Score based on how centered we are in the range
				float AltitudeRange = Spec.AltitudeRange.Y - Spec.AltitudeRange.X;
				if (AltitudeRange > 0.0f)
				{
					float AltitudeCenter = (Spec.AltitudeRange.X + Spec.AltitudeRange.Y) * 0.5f;
					float AltitudeDistance = FMath::Abs(Altitude - AltitudeCenter) / (AltitudeRange * 0.5f);
					Score += 1.0f - AltitudeDistance; // Higher score for being closer to center
				}
			}
		}

		// Check slope range
		if (bMatches && bUseSlopeRules)
		{
			if (SlopeAngle < Spec.SlopeRange.X || SlopeAngle > Spec.SlopeRange.Y)
			{
				bMatches = false;
			}
			else
			{
				// Score based on how centered we are in the range
				float SlopeRange = Spec.SlopeRange.Y - Spec.SlopeRange.X;
				if (SlopeRange > 0.0f)
				{
					float SlopeCenter = (Spec.SlopeRange.X + Spec.SlopeRange.Y) * 0.5f;
					float SlopeDistance = FMath::Abs(SlopeAngle - SlopeCenter) / (SlopeRange * 0.5f);
					Score += 1.0f - SlopeDistance;
				}
			}
		}

		if (bMatches && Score > BestScore)
		{
			BestScore = Score;
			BestBiome = Spec.BiomeId;
		}
	}

	// Return best match or default
	if (BestBiome.IsValid())
	{
		return BestBiome;
	}

	return DefaultBiomeId;
}

bool UBiomeSubsystem::SampleRVTAtLocation(const FVector& WorldLocation, TArray<float>& OutWeights) const
{
	// TODO: Implement RVT sampling when RVT system is set up
	// This would sample the Runtime Virtual Texture at the given location
	// and return the biome mask weights from the R, G, B, A channels
	
	// For now, return false to indicate RVT sampling is not available
	// The system will fall back to rule-based resolution
	return false;
}

ULandscapeComponent* UBiomeSubsystem::FindLandscapeAtLocation(const FVector& WorldLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Perform a line trace to find landscape component
	FVector Start = FVector(WorldLocation.X, WorldLocation.Y, WorldLocation.Z + 100000.0f);
	FVector End = FVector(WorldLocation.X, WorldLocation.Y, WorldLocation.Z - 100000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		return Cast<ULandscapeComponent>(HitResult.Component.Get());
	}

	return nullptr;
}
