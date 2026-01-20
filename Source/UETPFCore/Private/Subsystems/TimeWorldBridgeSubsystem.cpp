// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/TimeWorldBridgeSubsystem.h"
#include "Subsystems/TimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UTimeSubsystem* UTimeWorldBridgeSubsystem::GetTime() const
{
	if (CachedTime.IsValid())
	{
		return CachedTime.Get();
	}

	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UTimeSubsystem* Time = GI ? GI->GetSubsystem<UTimeSubsystem>() : nullptr;

	CachedTime = Time;
	return Time;
}

void UTimeWorldBridgeSubsystem::Tick(float DeltaTime)
{
	if (UTimeSubsystem* Time = GetTime())
	{
		Time->Advance(DeltaTime);
	}
}
