// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Materials/MaterialCreator.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Factories/MaterialFactoryNew.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "NiagaraComponent.h"
#include "Misc/PackageName.h"

UMaterial* UMaterialCreator::CreateStarMaterial(
	const FString& PackagePath,
	const FName& MaterialName,
	float DefaultBrightness)
{
	// Create package for material
	UPackage* Package = CreateMaterialPackage(PackagePath, MaterialName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialCreator: Failed to create package at %s"), *PackagePath);
		return nullptr;
	}

	// Create material using factory
	UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
	UMaterial* Material = Cast<UMaterial>(
		Factory->FactoryCreateNew(
			UMaterial::StaticClass(),
			Package,
			MaterialName,
			RF_Standalone | RF_Public,
			nullptr,
			GWarn
		)
	);

	if (!Material)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialCreator: Failed to create material %s"), *MaterialName.ToString());
		return nullptr;
	}

	// Configure material settings for Niagara
	ConfigureNiagaraMaterialSettings(Material);

	// Create material graph nodes
	// Node 1: StarColor (Vector Parameter)
	UMaterialExpressionVectorParameter* ColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
	ColorParam->ParameterName = FName("StarColor");
	ColorParam->DefaultValue = FLinearColor::White;
	ColorParam->MaterialExpressionEditorX = -400;
	ColorParam->MaterialExpressionEditorY = 0;
	Material->GetExpressionCollection().AddExpression(ColorParam);

	// Node 2: StarBrightness (Scalar Parameter)
	UMaterialExpressionScalarParameter* BrightnessParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	BrightnessParam->ParameterName = FName("StarBrightness");
	BrightnessParam->DefaultValue = DefaultBrightness;
	BrightnessParam->MaterialExpressionEditorX = -400;
	BrightnessParam->MaterialExpressionEditorY = 150;
	Material->GetExpressionCollection().AddExpression(BrightnessParam);

	// Node 3: Multiply (Color × Brightness)
	UMaterialExpressionMultiply* MultiplyNode = NewObject<UMaterialExpressionMultiply>(Material);
	MultiplyNode->A.Expression = ColorParam;
	MultiplyNode->B.Expression = BrightnessParam;
	MultiplyNode->MaterialExpressionEditorX = -200;
	MultiplyNode->MaterialExpressionEditorY = 75;
	Material->GetExpressionCollection().AddExpression(MultiplyNode);

	// Connect to EmissiveColor output
	Material->GetEditorOnlyData()->EmissiveColor.Expression = MultiplyNode;

	// CRITICAL: Update material BEFORE compile to ensure usage flags are included
	Material->PreEditChange(nullptr);
	Material->PostEditChange();
	
	// Force full recompile to bake in Niagara usage flags
	Material->ForceRecompileForRendering();
	
	Material->MarkPackageDirty();

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(Material);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GWarn;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs))
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ MaterialCreator: Created and saved star material: %s"), *Material->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialCreator: Failed to save material package"));
	}

	return Material;
}

void UMaterialCreator::ApplyMaterialToNiagara(
	UNiagaraComponent* NiagaraComponent,
	UMaterialInterface* Material,
	int32 ElementIndex)
{
	if (!NiagaraComponent || !Material)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialCreator: Invalid NiagaraComponent or Material"));
		return;
	}

	// Set material on Niagara renderer
	NiagaraComponent->SetMaterial(ElementIndex, Material);

	UE_LOG(LogTemp, Warning, TEXT("✅ MaterialCreator: Applied material %s to Niagara component at index %d"),
		*Material->GetName(), ElementIndex);
}

UPackage* UMaterialCreator::CreateMaterialPackage(const FString& PackagePath, const FName& MaterialName)
{
	// Construct full package path
	FString FullPackagePath = PackagePath;
	if (!FullPackagePath.EndsWith("/"))
	{
		FullPackagePath += "/";
	}
	FullPackagePath += MaterialName.ToString();

	// Create package
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		return nullptr;
	}

	Package->FullyLoad();
	return Package;
}

void UMaterialCreator::ConfigureNiagaraMaterialSettings(UMaterial* Material)
{
	if (!Material)
	{
		return;
	}

	// Unlit shading model (no lighting calculations)
	Material->SetShadingModel(MSM_Unlit);

	// Additive blending for emissive glow (stars add to background)
	Material->BlendMode = BLEND_Additive;

	// Two-sided rendering (visible from all angles)
	Material->TwoSided = true;

	// CRITICAL: Enable Niagara sprite usage flag BEFORE adding expressions
	// This ensures the flag is included in the initial material compile
	bool bNeedsRecompile = false;
	Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_NiagaraSprites);
	
	// Force immediate recompile if needed
	if (bNeedsRecompile)
	{
		Material->ForceRecompileForRendering();
	}

	UE_LOG(LogTemp, Warning, TEXT("MaterialCreator: Niagara usage set, NeedsRecompile=%d"), bNeedsRecompile);
}
