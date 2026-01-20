// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "UETPFCore.h"
#include "Modules/ModuleManager.h"

#include "Log.h"

void FUETPFCore::StartupModule()
{
	UE_LOG(LogUETPFCore, Log, TEXT("UETPFCore module starting up"));
}

void FUETPFCore::ShutdownModule()
{
	UE_LOG(LogUETPFCore, Log, TEXT("UETPFCore module shutting down"));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FUETPFCore, UETPFCore, "UETPFCore");
