// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "SkyContext.generated.h"

UENUM(BlueprintType)
enum class ECelestialBodyId : uint8
{
    Sun   UMETA(DisplayName="Sun"),
    Earth UMETA(DisplayName="Earth"),
    Moon  UMETA(DisplayName="Moon"),
};

USTRUCT(BlueprintType)
struct UETPFCORE_API FSkyContext
{
    GENERATED_BODY()

    // World-space unit direction TO the sun (used for light direction and moon shading).
    UPROPERTY(BlueprintReadOnly)
    FVector SunDirWorld = FVector(1,0,0);

    // intensity scalar you map into UE light units.
    UPROPERTY(BlueprintReadOnly)
    float SunIntensity = 10.0f;

    // Enable/disable costly features based on body/medium.
    UPROPERTY(BlueprintReadOnly)
    bool bEnableAtmosphere = true;

    UPROPERTY(BlueprintReadOnly)
    bool bEnableClouds = true;

    // Starfield rotation driver (sidereal-like). You can feed into a sky material.
    UPROPERTY(BlueprintReadOnly)
    float StarRotationRad = 0.0f;

    // Useful for tuning visuals (optional).
    UPROPERTY(BlueprintReadOnly)
    double AnchorBodyRadiusKm = 6371.0;

    // For moon phase shading.
    UPROPERTY(BlueprintReadOnly)
    float MoonPhaseRad = 0.0f;
};
