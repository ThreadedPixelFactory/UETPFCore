// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * World Frame Subsystem - Per-World Coordinate Anchoring
 * 
 * Purpose:
 * Handles coordinate transforms between canonical solar system frame and world space.
 * Each loaded world chooses its anchor body (Earth, Moon, or spacecraft).
 * 
 * Architecture:
 * - Per-world subsystem (one instance per UWorld)
 * - Subscribes to SolarSystemSubsystem for canonical positions
 * - Converts km-scale solar positions to cm-scale world positions
 * - Anchoring: World origin = Anchor body center in canonical frame
 * 
 * Coordinate Transform:
 * ```
 * Canonical Frame (km):    Solar system, Earth at origin
 * World Frame (cm):        UE world space, Anchor body at origin
 * 
 * Transform:
 *   WorldPos_cm = (CanonicalPos_km - AnchorPos_km) * 100000.0
 * ```
 * 
 * Example Scenarios:
 * - Earth Map: Anchor = Earth, Moon appears 384,400 km away in world
 * - Moon Map: Anchor = Moon, Earth appears 384,400 km away
 * - Space Map: Anchor = Spacecraft, both Earth and Moon visible
 * 
 * Usage:
 * \code{.cpp}
 *   // In Earth level BeginPlay:
 *   UWorldFrameSubsystem* Frame = World->GetSubsystem<UWorldFrameSubsystem>();
 *   Frame->SetAnchorBody(ECelestialBodyId::Earth);
 *   
 *   // Query Moon position in world space:
 *   FVector MoonWorldPos = Frame->GetMoonWorldCm();
 * \endcode
 * 
 * Integration:
 * - Driven by USolarSystemSubsystem for canonical positions
 * - Consumed by AUniversalSkyActor for sun/moon rendering
 * - Used by gameplay code for celestial object placement
 * 
 * @see USolarSystemSubsystem for canonical astronomy
 * @see UInterplanetaryTravelSubsystem for map transitions
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Environment/SkyContext.h"
#include "WorldFrameSubsystem.generated.h"

class USolarSystemSubsystem;

/**
 * Per-world anchoring + transform utilities.
 * Each loaded map chooses its anchor body (Earth for Lvl_Earth, Moon for Lvl_Moon).
 * Converts canonical km positions into world cm relative to anchor.
 */
UCLASS()
class UETPFCORE_API UWorldFrameSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Frame")
    void SetAnchorBody(ECelestialBodyId InAnchor);

    UFUNCTION(BlueprintCallable, Category="Frame")
    ECelestialBodyId GetAnchorBody() const { return AnchorBody; }

    // Convert canonical solar-frame position (km) into this world's cm position.
    UFUNCTION(BlueprintCallable, Category="Frame")
    FVector CanonicalKmToWorldCm(const FVector& CanonicalPosKm) const;

    // Convenience: get Moon world position in this world.
    UFUNCTION(BlueprintCallable, Category="Frame")
    FVector GetMoonWorldCm() const;

    // Build sky context for the current world anchor.
    UFUNCTION(BlueprintCallable, Category="Frame")
    FSkyContext BuildSkyContext() const;

private:
    USolarSystemSubsystem* GetSolar() const;

    static constexpr double KmToCm = 100000.0;

    ECelestialBodyId AnchorBody = ECelestialBodyId::Earth;
};
