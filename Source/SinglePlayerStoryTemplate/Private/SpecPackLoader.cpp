// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "SpecPackLoader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformFileManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "SpecTypes.h"

USpecPackLoader::USpecPackLoader()
{
	DefaultSpecPackPath = FPaths::ProjectContentDir() / TEXT("SpecPacks/Core.json");
}

FSpecPackLoadResult USpecPackLoader::LoadSpecPack(const FString& FilePath)
{
	FSpecPackLoadResult Result;

	// Read file
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
		UE_LOG(LogTemp, Warning, TEXT("SpecPackLoader: %s"), *Result.ErrorMessage);
		return Result;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		Result.ErrorMessage = TEXT("Failed to parse JSON");
		UE_LOG(LogTemp, Warning, TEXT("SpecPackLoader: %s"), *Result.ErrorMessage);
		return Result;
	}

	// Parse manifest
	ParseManifest(RootObject, Result.Manifest);
	Result.Manifest.ContentHash = GetSpecPackHash(FilePath);

	// Parse specs
	Result.SurfaceSpecsLoaded = ParseSurfaceSpecs(RootObject);
	Result.MediumSpecsLoaded = ParseMediumSpecs(RootObject);
	Result.BiomeSpecsLoaded = ParseBiomeSpecs(RootObject);

	// Cache manifest
	CachedManifests.Add(FilePath, Result.Manifest);

	Result.bSuccess = true;
	UE_LOG(LogTemp, Log, TEXT("SpecPackLoader: Loaded %s - %d surface, %d medium, %d biome specs"),
		*FilePath, Result.SurfaceSpecsLoaded, Result.MediumSpecsLoaded, Result.BiomeSpecsLoaded);

	return Result;
}

TArray<FSpecPackLoadResult> USpecPackLoader::LoadSpecPacksFromDirectory(const FString& DirectoryPath)
{
	TArray<FSpecPackLoadResult> Results;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	TArray<FString> FoundFiles;
	PlatformFile.FindFilesRecursively(FoundFiles, *DirectoryPath, TEXT(".json"));

	for (const FString& FilePath : FoundFiles)
	{
		// Skip manifest files
		if (FilePath.Contains(TEXT("manifest")))
		{
			continue;
		}

		Results.Add(LoadSpecPack(FilePath));
	}

	return Results;
}

FSpecPackLoadResult USpecPackLoader::LoadDefaultSpecPack()
{
	return LoadSpecPack(DefaultSpecPackPath);
}

bool USpecPackLoader::ValidateSpecPack(const FString& FilePath, FString& OutErrorMessage)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		OutErrorMessage = FString::Printf(TEXT("Cannot read file: %s"), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutErrorMessage = TEXT("Invalid JSON syntax");
		return false;
	}

	// Check required fields
	if (!RootObject->HasField(TEXT("pack_id")))
	{
		OutErrorMessage = TEXT("Missing required field: pack_id");
		return false;
	}

	if (!RootObject->HasField(TEXT("version")))
	{
		OutErrorMessage = TEXT("Missing required field: version");
		return false;
	}

	// Validate spec arrays if present
	if (RootObject->HasField(TEXT("surface_specs")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SpecArray;
		if (!RootObject->TryGetArrayField(TEXT("surface_specs"), SpecArray))
		{
			OutErrorMessage = TEXT("surface_specs must be an array");
			return false;
		}
	}

	if (RootObject->HasField(TEXT("medium_specs")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SpecArray;
		if (!RootObject->TryGetArrayField(TEXT("medium_specs"), SpecArray))
		{
			OutErrorMessage = TEXT("medium_specs must be an array");
			return false;
		}
	}

	return true;
}

FString USpecPackLoader::GetSpecPackHash(const FString& FilePath) const
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		return FString();
	}

	// SHA256 hash of file content
	FSHAHash Hash;
	FSHA1::HashBuffer(TCHAR_TO_UTF8(*JsonString), JsonString.Len(), Hash.Hash);
	return Hash.ToString();
}

bool USpecPackLoader::GetManifest(const FString& FilePath, FSpecPackManifest& OutManifest) const
{
	if (const FSpecPackManifest* Found = CachedManifests.Find(FilePath))
	{
		OutManifest = *Found;
		return true;
	}
	return false;
}

int32 USpecPackLoader::ParseSurfaceSpecs(const TSharedPtr<FJsonObject>& RootObject)
{
	int32 Count = 0;

	const TArray<TSharedPtr<FJsonValue>>* SpecArray;
	if (!RootObject->TryGetArrayField(TEXT("surface_specs"), SpecArray))
	{
		return 0;
	}

	for (const TSharedPtr<FJsonValue>& Value : *SpecArray)
	{
		const TSharedPtr<FJsonObject>* SpecObj;
		if (!Value->TryGetObject(SpecObj))
		{
			continue;
		}

		// Parse spec ID
		FString IdString;
		if (!(*SpecObj)->TryGetStringField(TEXT("id"), IdString))
		{
			UE_LOG(LogTemp, Warning, TEXT("SpecPackLoader: SurfaceSpec missing 'id' field"));
			continue;
		}

		FSurfaceSpecId SpecId;
		SpecId.Id = FName(*IdString);

		// Parse spec data
		FRuntimeSurfaceSpec Spec;
		
		// Display name
		FString DisplayName;
		if ((*SpecObj)->TryGetStringField(TEXT("display_name"), DisplayName))
		{
			Spec.DisplayName = DisplayName;
		}

		// Friction
		(*SpecObj)->TryGetNumberField(TEXT("static_friction"), Spec.StaticFriction);
		(*SpecObj)->TryGetNumberField(TEXT("dynamic_friction"), Spec.DynamicFriction);
		(*SpecObj)->TryGetNumberField(TEXT("restitution"), Spec.Restitution);

		// Deformation
		(*SpecObj)->TryGetNumberField(TEXT("deformation_rate"), Spec.DeformationRatePerS);
		(*SpecObj)->TryGetNumberField(TEXT("max_deformation_depth"), Spec.MaxDeformationDepthCm);
		(*SpecObj)->TryGetNumberField(TEXT("recovery_rate"), Spec.RecoveryRatePerS);

		// Flags
		(*SpecObj)->TryGetBoolField(TEXT("is_deformable"), Spec.bIsDeformable);
		(*SpecObj)->TryGetBoolField(TEXT("is_slippery"), Spec.bIsSlippery);

		RegisterSurfaceSpec(SpecId, Spec);
		Count++;
	}

	return Count;
}

int32 USpecPackLoader::ParseMediumSpecs(const TSharedPtr<FJsonObject>& RootObject)
{
	int32 Count = 0;

	const TArray<TSharedPtr<FJsonValue>>* SpecArray;
	if (!RootObject->TryGetArrayField(TEXT("medium_specs"), SpecArray))
	{
		return 0;
	}

	for (const TSharedPtr<FJsonValue>& Value : *SpecArray)
	{
		const TSharedPtr<FJsonObject>* SpecObj;
		if (!Value->TryGetObject(SpecObj))
		{
			continue;
		}

		// Parse spec ID
		FString IdString;
		if (!(*SpecObj)->TryGetStringField(TEXT("id"), IdString))
		{
			UE_LOG(LogTemp, Warning, TEXT("SpecPackLoader: MediumSpec missing 'id' field"));
			continue;
		}

		FMediumSpecId SpecId;
		SpecId.Id = FName(*IdString);

		// Parse spec data
		FRuntimeMediumSpec  Spec;
		
		// Display name
		FString DisplayName;
		if ((*SpecObj)->TryGetStringField(TEXT("display_name"), DisplayName))
		{
			Spec.DisplayName = DisplayName;
		}

		// Physical properties
		(*SpecObj)->TryGetNumberField(TEXT("density"), Spec.Density);
		(*SpecObj)->TryGetNumberField(TEXT("drag_coefficient"), Spec.QuadraticDragCoeff);
		(*SpecObj)->TryGetNumberField(TEXT("viscosity"), Spec.Viscosity);

		// Audio properties
		(*SpecObj)->TryGetNumberField(TEXT("speed_of_sound"), Spec.SpeedOfSound);
		(*SpecObj)->TryGetNumberField(TEXT("absorption_coefficient"), Spec.AbsorptionCoefficient);

		RegisterMediumSpec(SpecId, Spec);
		Count++;
	}

	return Count;
}

int32 USpecPackLoader::ParseBiomeSpecs(const TSharedPtr<FJsonObject>& RootObject)
{
	// TODO: Implement biome spec parsing
	// This would register with BiomeSubsystem
	return 0;
}

bool USpecPackLoader::ParseManifest(const TSharedPtr<FJsonObject>& RootObject, FSpecPackManifest& OutManifest)
{
	RootObject->TryGetStringField(TEXT("pack_id"), OutManifest.PackId);
	RootObject->TryGetNumberField(TEXT("version"), OutManifest.Version);
	RootObject->TryGetStringField(TEXT("engine_compat"), OutManifest.EngineCompat);

	// Parse contained spec types
	const TArray<TSharedPtr<FJsonValue>>* TypesArray;
	if (RootObject->TryGetArrayField(TEXT("spec_types"), TypesArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *TypesArray)
		{
			FString TypeName;
			if (Value->TryGetString(TypeName))
			{
				OutManifest.ContainedSpecTypes.Add(TypeName);
			}
		}
	}

	OutManifest.Timestamp = FDateTime::UtcNow();
	return true;
}

void USpecPackLoader::RegisterSurfaceSpec(const FSurfaceSpecId& Id, const FRuntimeSurfaceSpec& Spec)
{
	// TODO: Register with SurfaceQuerySubsystem or a global spec registry
	// For now, just log that we parsed it
	UE_LOG(LogTemp, Verbose, TEXT("SpecPackLoader: Registered SurfaceSpec '%s'"), *Id.Id.ToString());
}

void USpecPackLoader::RegisterMediumSpec(const FMediumSpecId& Id, const FRuntimeMediumSpec & Spec)
{
	// TODO: Register with EnvironmentSubsystem or a global spec registry
	// For now, just log that we parsed it
	UE_LOG(LogTemp, Verbose, TEXT("SpecPackLoader: Registered MediumSpec '%s'"), *Id.Id.ToString());
}
