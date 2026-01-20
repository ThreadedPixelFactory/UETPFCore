// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Materials/Material.h"
#include "MaterialCreator.generated.h"

/**
 * WIP: Procedural material creation utility for runtime/editor material generation.
 * Provides type-safe API for creating physically-based materials from real-world values.
 */
UCLASS()
class UETPFCORE_API UMaterialCreator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Creates a Niagara-compatible unlit emissive star material.
	 * 
	 * @param PackagePath Path for material package (e.g., "/Game/Core/Materials/M_StarProcedural")
	 * @param MaterialName Name of the material asset
	 * @param DefaultBrightness Default brightness multiplier for emissive boost
	 * @return Created material, or nullptr on failure
	 */
	static UMaterial* CreateStarMaterial(
		const FString& PackagePath,
		const FName& MaterialName,
		float DefaultBrightness = 1000.0f);

	/**
	 * Applies a procedurally-created material to a Niagara component.
	 * 
	 * @param NiagaraComponent Target component
	 * @param Material Material to apply (must be Niagara-compatible)
	 * @param ElementIndex Renderer element index (default 0)
	 */
	static void ApplyMaterialToNiagara(
		class UNiagaraComponent* NiagaraComponent,
		UMaterialInterface* Material,
		int32 ElementIndex = 0);

private:
	/** Helper: Creates package and handles asset registration */
	static UPackage* CreateMaterialPackage(const FString& PackagePath, const FName& MaterialName);

	/** Helper: Configures material for Niagara sprite rendering */
	static void ConfigureNiagaraMaterialSettings(UMaterial* Material);
};
