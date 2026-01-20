// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "GameLauncher.h"

#define LOCTEXT_NAMESPACE "FGameLauncherModule"

void FGameLauncherModule::StartupModule()
{
	// Module initialization
	UE_LOG(LogTemp, Log, TEXT("GameLauncher module started"));
}

void FGameLauncherModule::ShutdownModule()
{
	// Module cleanup
	UE_LOG(LogTemp, Log, TEXT("GameLauncher module shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLauncherModule, GameLauncher)
