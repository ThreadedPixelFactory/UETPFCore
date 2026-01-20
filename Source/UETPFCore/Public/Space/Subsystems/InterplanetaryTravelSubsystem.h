// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

/**
 * Interplanetary Travel Subsystem - Map Transition Management
 * 
 * Purpose:
 * Manages seamless transitions between Earth, Moon, and Space maps
 * while maintaining continuous time and astronomical state.
 * 
 * Architecture:
 * - GameInstance subsystem (persists across level loads)
 * - Does NOT handle anchoring (that's WorldFrameSubsystem's job)
 * - Preserves TimeSubsystem state across transitions
 * - Preserves SolarSystemSubsystem state (sun/moon positions)
 * 
 * Travel Flow:
 * 1. Player initiates travel (e.g., clicks "Travel to Moon")
 * 2. TravelToMoon() called
 * 3. Current map unloads
 * 4. Moon map loads
 * 5. Moon map's WorldFrameSubsystem sets AnchorBody = Moon
 * 6. Time continues, astronomy remains consistent
 * 7. Player sees Earth in Moon sky at correct position
 * 
 * Map Configuration:
 * Each map should set its anchor in BeginPlay:
 * ```cpp
 * // In Moon map's GameMode::BeginPlay()
 * UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();
 * Frame->SetAnchorBody(ECelestialBodyId::Moon);
 * ```
 * 
 * Time Continuity:
 * - TimeSubsystem (GameInstance) survives map transition
 * - SimTimeSeconds continues without reset
 * - Sun/moon positions remain accurate
 * - No jarring visual discontinuity
 * 
 * Usage:
 * \code{.cpp}
 *   UInterplanetaryTravelSubsystem* Travel = GameInstance->GetSubsystem<UInterplanetaryTravelSubsystem>();
 *   Travel->TravelToMoon(); // Loads moon map
 * \endcode
 * 
 * @note Does not handle anchoring - each map sets its own anchor
 * @note Preserves all GameInstance subsystems across transitions
 * @see UWorldFrameSubsystem for per-world anchoring
 * @see UTimeSubsystem for continuous time
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Environment/SkyContext.h"
#include "InterplanetaryTravelSubsystem.generated.h"

/**
 * Owns map transitions between Earth/Travel/Moon.
 * Keeps solar/time continuous while swapping worlds.
 * 
 * Important: Travel subsystem does not do anchoring — anchoring is done in each map via WorldFrameSubsystem.SetAnchorBody().
 */
UCLASS()
class UETPFCORE_API UInterplanetaryTravelSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Travel")
    void TravelToEarth();

    UFUNCTION(BlueprintCallable, Category="Travel")
    void TravelToMoon();

    UFUNCTION(BlueprintCallable, Category="Travel")
    void TravelToSpace();

    // Map names configured in DefaultGame.ini or set here.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
    FName EarthMap = "Lvl_Earth";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
    FName MoonMap = "Lvl_Moon";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
    FName SpaceMap = "Lvl_SpaceTravel";

private:
    void OpenMap(FName Map);
};
