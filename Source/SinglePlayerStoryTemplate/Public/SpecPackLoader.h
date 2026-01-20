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

/**
 * SpecPack Loader - Runtime Spec Loading System
 * 
 * Purpose:
 * Implements runtime-first architecture for loading game specs from JSON/CBOR files.
 * Enables modding, hotfixes, and data-driven content without C++ recompilation.
 * 
 * Architecture:
 * - SpecPacks are JSON files containing arrays of specs (Surface, Medium, Biome, etc.)
 * - Loaded at runtime and registered with appropriate subsystems
 * - Supports validation, versioning, and hash verification
 * - Multiplayer-ready: clients can verify they match server specs
 * 
 * Directory Structure:
 *   Content/SpecPacks/
 *     ├── Core.json           (default specs - engine fundamentals)
 *     ├── Biomes.json         (biome definitions for world)
 *     ├── manifest.json       (pack metadata, version, hash)
 *     └── [Custom].json       (mod/override packs)
 * 
 * JSON Format Example:
 * \code{.json}
 * {
 *   "manifest": {
 *     "packId": "CoreSpecs",
 *     "version": 1,
 *     "engineCompat": "5.7"
 *   },
 *   "surfaceSpecs": [
 *     {
 *       "specId": "Concrete",
 *       "staticFriction": 0.8,
 *       "dynamicFriction": 0.6,
 *       ...
 *     }
 *   ],
 *   "mediumSpecs": [ ... ]
 * }
 * \endcode
 * 
 * Usage:
 * \code{.cpp}
 *   USpecPackLoader* Loader = NewObject<USpecPackLoader>();
 *   FSpecPackLoadResult Result = Loader->LoadSpecPack(TEXT("/Game/SpecPacks/Core.json"));
 *   if (Result.bSuccess) {
 *     UE_LOG(LogTemp, Log, TEXT("Loaded %d surface specs"), Result.SurfaceSpecsLoaded);
 *   }
 * \endcode
 * 
 * Multiplayer Flow:
 * 1. Server loads SpecPack(s) at startup
 * 2. Server announces SpecPackHash to clients
 * 3. Client verifies local cache matches
 * 4. If mismatch: client downloads from server or rejects connection
 * 
 * Modding Support:
 * - Mods provide additional SpecPack JSON files
 * - Later packs can override earlier specs by ID
 * - Validation ensures no breaking changes
 * 
 * @see FRuntimeSurfaceSpec for spec data format
 * @see USurfaceQuerySubsystem for spec registration
 * @see UEnvironmentSubsystem for medium spec registration
 */

#pragma once

#include "CoreMinimal.h"
#include "SpecTypes.h"
#include "SpecPackLoader.generated.h"

/**
 * Manifest entry for a SpecPack.
 * Tracks version, hash, and contents for validation and caching.
 */
USTRUCT(BlueprintType)
struct SINGLEPLAYERSTORYTEMPLATE_API FSpecPackManifest
{
	GENERATED_BODY()

	/** Unique identifier for this pack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecPack")
	FString PackId;

	/** Version number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecPack")
	int32 Version = 1;

	/** Content hash (for cache validation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecPack")
	FString ContentHash;

	/** Engine compatibility version */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecPack")
	FString EngineCompat = TEXT("5.7");

	/** Timestamp when pack was generated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecPack")
	FDateTime Timestamp;

	/** List of spec types contained */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecPack")
	TArray<FString> ContainedSpecTypes;
};

/**
 * Result of loading a SpecPack.
 */
USTRUCT(BlueprintType)
struct SINGLEPLAYERSTORYTEMPLATE_API FSpecPackLoadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SpecPack")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "SpecPack")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly, Category = "SpecPack")
	FSpecPackManifest Manifest;

	UPROPERTY(BlueprintReadOnly, Category = "SpecPack")
	int32 SurfaceSpecsLoaded = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SpecPack")
	int32 MediumSpecsLoaded = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SpecPack")
	int32 BiomeSpecsLoaded = 0;
};

/**
 * Runtime SpecPack loader.
 * 
 * Loads specs from JSON files at runtime per the runtime-first architecture.
 * 
 * Directory Structure:
 *   Content/SpecPacks/
 *     ├── Core.json           (default specs)
 *     ├── Biomes.json         (biome definitions)
 *     ├── manifest.json       (pack metadata)
 *     └── [Custom].json       (mod/override packs)
 * 
 * Usage:
 *   USpecPackLoader* Loader = NewObject<USpecPackLoader>();
 *   FSpecPackLoadResult Result = Loader->LoadSpecPack(TEXT("Content/SpecPacks/Core.json"));
 *   if (Result.bSuccess) { ... }
 * 
 * Multiplayer:
 *   Server announces SpecPackHash
 *   Client verifies local cache matches or downloads
 */
UCLASS(BlueprintType)
class SINGLEPLAYERSTORYTEMPLATE_API USpecPackLoader : public UObject
{
	GENERATED_BODY()

public:
	USpecPackLoader();

	/**
	 * Load a SpecPack from a JSON file.
	 * Registers specs with the appropriate subsystems automatically.
	 * 
	 * Loading Pipeline:
	 * 1. Read and parse JSON file
	 * 2. Validate format and version
	 * 3. Parse manifest metadata
	 * 4. Parse spec arrays (surface, medium, biome)
	 * 5. Register with subsystems (SurfaceQuery, Environment, Biome)
	 * 6. Return result with counts and any errors
	 * 
	 * @param FilePath - Absolute or Content-relative path to JSON file
	 *                   Examples: 
	 *                   - "/Game/SpecPacks/Core.json" (Content-relative)
	 *                   - "C:/MyProject/Content/SpecPacks/Core.json" (Absolute)
	 * 
	 * @return FSpecPackLoadResult with success status, error messages, and loaded counts
	 * 
	 * @note Registered specs override any previous specs with same ID
	 * @note On parse error, returns partial results (some specs may be loaded)
	 * @note Safe to call multiple times (idempotent per unique spec ID)
	 * @note Automatically finds and registers with world subsystems
	 * 
	 * Error Handling:
	 * - File not found: bSuccess=false, ErrorMessage set
	 * - Invalid JSON: bSuccess=false, ErrorMessage with parse error
	 * - Missing required fields: Skips invalid specs, logs warnings
	 * - Invalid values: Clamps to valid ranges, logs warnings
	 * 
	 * Example:
	 * \code{.cpp}
	 *   USpecPackLoader* Loader = NewObject<USpecPackLoader>();
	 *   FSpecPackLoadResult Result = Loader->LoadSpecPack("/Game/SpecPacks/Core.json");
	 *   if (!Result.bSuccess) {
	 *     UE_LOG(LogTemp, Error, TEXT("Failed to load: %s"), *Result.ErrorMessage);
	 *   }
	 * \endcode
	 */
	UFUNCTION(BlueprintCallable, Category = "SpecPack")
	FSpecPackLoadResult LoadSpecPack(const FString& FilePath);

	/**
	 * Load all SpecPacks from a directory.
	 */
	UFUNCTION(BlueprintCallable, Category = "SpecPack")
	TArray<FSpecPackLoadResult> LoadSpecPacksFromDirectory(const FString& DirectoryPath);

	/**
	 * Load the default embedded SpecPack.
	 */
	UFUNCTION(BlueprintCallable, Category = "SpecPack")
	FSpecPackLoadResult LoadDefaultSpecPack();

	/**
	 * Validate a SpecPack file without loading it.
	 */
	UFUNCTION(BlueprintCallable, Category = "SpecPack")
	bool ValidateSpecPack(const FString& FilePath, FString& OutErrorMessage);

	/**
	 * Get hash of a SpecPack file (for cache/sync validation).
	 */
	UFUNCTION(BlueprintCallable, Category = "SpecPack")
	FString GetSpecPackHash(const FString& FilePath) const;

	/**
	 * Get the manifest from a loaded pack.
	 */
	UFUNCTION(BlueprintCallable, Category = "SpecPack")
	bool GetManifest(const FString& FilePath, FSpecPackManifest& OutManifest) const;

protected:
	/** Parse SurfaceSpec entries from JSON */
	int32 ParseSurfaceSpecs(const TSharedPtr<FJsonObject>& RootObject);

	/** Parse MediumSpec entries from JSON */
	int32 ParseMediumSpecs(const TSharedPtr<FJsonObject>& RootObject);

	/** Parse BiomeSpec entries from JSON */
	int32 ParseBiomeSpecs(const TSharedPtr<FJsonObject>& RootObject);

	/** Parse manifest from JSON */
	bool ParseManifest(const TSharedPtr<FJsonObject>& RootObject, FSpecPackManifest& OutManifest);

	/** Register a SurfaceSpec with the runtime registry */
	void RegisterSurfaceSpec(const FSurfaceSpecId& Id, const FRuntimeSurfaceSpec& Spec);

	/** Register a MediumSpec with the runtime registry */
	void RegisterMediumSpec(const FMediumSpecId& Id, const FRuntimeMediumSpec & Spec);

private:
	/** Default SpecPack path (embedded in build) */
	FString DefaultSpecPackPath;

	/** Cached manifests by file path */
	TMap<FString, FSpecPackManifest> CachedManifests;
};
