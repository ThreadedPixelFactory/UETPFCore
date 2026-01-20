// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Space/Subsystems/StarCatalogSubsystem.h"
#include "Space/Subsystems/CelestialMathLibrary.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

static bool ParseCsvLineFast(const FString& Line, TArray<FString>& OutCols)
{
	OutCols.Reset();
	Line.ParseIntoArray(OutCols, TEXT(","), /*CullEmpty*/ false);
	return OutCols.Num() > 0;
}

void UStarCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Don’t auto-load here if you want faster startup; call EnsureLoaded from sky/level on demand.
}

void UStarCatalogSubsystem::Deinitialize()
{
	StarsInternal.Reset();
	Stars.Reset();
	bLoaded = false;
	Super::Deinitialize();
}

bool UStarCatalogSubsystem::EnsureLoaded()
{
	if (bLoaded && StarsInternal.Num() > 0)
	{
		return true;
	}

	const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/UETPFCore/Resources/SpecPacks/starmap_milkyway.csv"));
	bLoaded = LoadFromCsv(FullPath);
	if (bLoaded && StarsInternal.Num() > 0)
	{
		ConvertInternalToOutput();
		return true;
	}
	return false;
}

bool UStarCatalogSubsystem::Reload()
{
	StarsInternal.Reset();
	Stars.Reset();
	bLoaded = false;
	return EnsureLoaded();
}

bool UStarCatalogSubsystem::LoadFromCsv(const FString& FullPath)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FullPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StarCatalog] Failed to load CSV: %s"), *FullPath);
		return false;
	}

	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, /*CullEmpty*/ true);
	if (Lines.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StarCatalog] CSV empty/too small: %s"), *FullPath);
		return false;
	}

	// Header expectations (from your generator):
	// id,name,proper,bf,ra,dec,mag,ci,dist,x,y,z,spect,con
	const int32 HeaderIdx = 0;
	const FString& Header = Lines[HeaderIdx];
	if (!Header.Contains(TEXT("ra")) 
	|| !Header.Contains(TEXT("dec")) 
	|| !Header.Contains(TEXT("mag"))
	|| !Header.Contains(TEXT("ci")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StarCatalog] CSV header unexpected. Using positional parse anyway."));
	}

	StarsInternal.Reserve(FMath::Min(MaxStars, Lines.Num()));

	TArray<FString> Cols;
	for (int32 i = 1; i < Lines.Num(); ++i)
	{
		if (StarsInternal.Num() >= MaxStars) break;

		if (!ParseCsvLineFast(Lines[i], Cols)) continue;
		if (Cols.Num() < 8) continue; // need at least ra,dec,mag,ci

		// Column positions per your out_cols:
		// 0:id 1:name 2:proper 3:bf 4:ra 5:dec 6:mag 7:ci 8:dist 9:x 10:y 11:z 12:spect 13:con
		const int32 StarId = FCString::Atoi(*Cols[0]);
		const FString Name = Cols[1];
		const FString ProperName = Cols[2];
		const FString BayerFlamsteed = Cols[3];
		const double RaHours = FCString::Atod(*Cols[4]);
		const double DecDeg = FCString::Atod(*Cols[5]);
		const float Mag = (float)FCString::Atof(*Cols[6]);
		const float CI = (float)FCString::Atof(*Cols[7]);
		const float DistParsecs = (Cols.Num() > 8) ? (float)FCString::Atof(*Cols[8]) : 1000.0f;
		const FString SpectralType = (Cols.Num() > 12) ? Cols[12] : TEXT("");
		const FString Constellation = (Cols.Num() > 13) ? Cols[13] : TEXT("");

		if (!FMath::IsFinite(RaHours) || !FMath::IsFinite(DecDeg) || !FMath::IsFinite(Mag))
		{
			continue;
		}

		const FVector3d Dir = UCelestialMathLibrary::EquatorialDir_FromRaDec(RaHours, DecDeg);
		if (Dir.IsNearlyZero()) continue;

		FStarRecordInternal R;
		R.Id = StarId;
		R.Name = Name;
		R.ProperName = ProperName;
		R.BayerFlamsteed = BayerFlamsteed;
		R.DirEquatorial = Dir;
		R.Mag = Mag;
		R.CI = CI;
		R.DistanceParsecs = DistParsecs;
		R.SpectralType = SpectralType;
		R.Constellation = Constellation;
		StarsInternal.Add(R);
	}

	UE_LOG(LogTemp, Log, TEXT("[StarCatalog] Loaded %d stars from %s"), StarsInternal.Num(), *FullPath);
	return StarsInternal.Num() > 0;
}

void UStarCatalogSubsystem::ConvertInternalToOutput()
{
	Stars.Reset();
	Stars.Reserve(StarsInternal.Num());
	for (const FStarRecordInternal& Internal : StarsInternal)
	{
		FStarRecord R;
		R.Id = Internal.Id;
		R.Name = Internal.Name;
		R.ProperName = Internal.ProperName;
		R.BayerFlamsteed = Internal.BayerFlamsteed;
		R.DirEquatorial = FVector3f(Internal.DirEquatorial.X, Internal.DirEquatorial.Y, Internal.DirEquatorial.Z);
		R.Mag = Internal.Mag;
		R.CI = Internal.CI;
		R.DistanceParsecs = Internal.DistanceParsecs;
		R.SpectralType = Internal.SpectralType;
		R.Constellation = Internal.Constellation;
		Stars.Add(R);
	}
}

const TArray<FStarRecord>& UStarCatalogSubsystem::GetStars() const
{
	return Stars;
}
