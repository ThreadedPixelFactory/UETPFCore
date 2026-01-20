// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Space/Subsystems/WorldFrameSubsystem.h"
#include "Space/Subsystems/SolarSystemSubsystem.h"
#include "Engine/GameInstance.h"

void UWorldFrameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UWorldFrameSubsystem::SetAnchorBody(ECelestialBodyId InAnchor)
{
    AnchorBody = InAnchor;
}

USolarSystemSubsystem* UWorldFrameSubsystem::GetSolar() const
{
    if (!GetWorld()) return nullptr;
    UGameInstance* GI = GetWorld()->GetGameInstance();
    return GI ? GI->GetSubsystem<USolarSystemSubsystem>() : nullptr;
}

FVector UWorldFrameSubsystem::CanonicalKmToWorldCm(const FVector& CanonicalPosKm) const
{
    USolarSystemSubsystem* Solar = GetSolar();
    if (!Solar) return FVector::ZeroVector;

    const FVector AnchorKm = Solar->GetBodyState(AnchorBody).PositionKm;
    const FVector RelKm = CanonicalPosKm - AnchorKm; // anchor at world origin
    const FVector RelCm = RelKm * KmToCm;

    return FVector((float)RelCm.X, (float)RelCm.Y, (float)RelCm.Z);
}

FVector UWorldFrameSubsystem::GetMoonWorldCm() const
{
    USolarSystemSubsystem* Solar = GetSolar();
    if (!Solar) return FVector::ZeroVector;

    const FVector3d MoonKm = Solar->GetBodyState(ECelestialBodyId::Moon).PositionKm;
    return CanonicalKmToWorldCm(MoonKm);
}

FSkyContext UWorldFrameSubsystem::BuildSkyContext() const
{
    FSkyContext Ctx;

    USolarSystemSubsystem* Solar = GetSolar();
    if (!Solar) return Ctx;

    const FCelestialBodyDef Def = Solar->GetBodyDef(AnchorBody);

    // Sun dir: canonical == world for now (if you want, rotate per latitude later).
    Ctx.SunDirWorld = Solar->GetSunDirCanonical();

    Ctx.SunIntensity = 10.0f; // map this in UniversalSkyActor into UE units
    Ctx.bEnableAtmosphere = Def.bHasAtmosphere;
    Ctx.bEnableClouds = Def.bHasClouds;
    Ctx.AnchorBodyRadiusKm = Def.RadiusKm;
    Ctx.MoonPhaseRad = Solar->GetMoonPhaseRad();

    return Ctx;
}
