// Copyright Threaded Pixel Factory. All Rights Reserved.
// Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.

#include "SinglePlayerStoryTemplate.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FSinglePlayerStoryTemplate, SinglePlayerStoryTemplate)

void FSinglePlayerStoryTemplate::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("SinglePlayerStoryTemplate module starting up"));
}

void FSinglePlayerStoryTemplate::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("SinglePlayerStoryTemplate module shutting down"));
}
