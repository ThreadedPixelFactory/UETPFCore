// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Space/Subsystems/InterplanetaryTravelSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void UInterplanetaryTravelSubsystem::OpenMap(FName Map)
{
    if (UWorld* World = GetWorld())
    {
        UGameplayStatics::OpenLevel(World, Map);
    }
}

void UInterplanetaryTravelSubsystem::TravelToEarth() { OpenMap(EarthMap); }
void UInterplanetaryTravelSubsystem::TravelToMoon()  { OpenMap(MoonMap); }
void UInterplanetaryTravelSubsystem::TravelToSpace() { OpenMap(SpaceMap); }
