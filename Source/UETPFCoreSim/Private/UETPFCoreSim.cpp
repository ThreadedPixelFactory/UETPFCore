// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "UETPFCoreSim.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FUETPFCoreSim"

void FUETPFCoreSim::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("UETPFCoreSim module starting up"));

#if WITH_SPICE
	bSpiceInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("SPICE library available"));
#else
	bSpiceInitialized = false;
	UE_LOG(LogTemp, Warning, TEXT("SPICE library not found. Scientific ephemerides disabled."));
	UE_LOG(LogTemp, Warning, TEXT("To enable: Download CSPICE from https://naif.jpl.nasa.gov/naif/toolkit.html"));
#endif
}

void FUETPFCoreSim::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("UETPFCoreSim module shutting down"));
}

bool FUETPFCoreSim::IsSpiceAvailable()
{
#if WITH_SPICE
	return true;
#else
	return false;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUETPFCoreSim, UETPFCoreSim)
