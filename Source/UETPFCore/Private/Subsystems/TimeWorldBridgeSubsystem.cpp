// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/TimeWorldBridgeSubsystem.h"
#include "Subsystems/TimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DECLARE_CYCLE_STAT(TEXT("TimeWorldBridgeSubsystem::Tick"), STAT_TimeWorldBridgeTick, STATGROUP_Game);

bool UTimeWorldBridgeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create in game worlds (PIE, Standalone, etc.) - not in editor preview worlds
	if (UWorld* World = Cast<UWorld>(Outer))
	{
		const EWorldType::Type WorldType = World->WorldType;
		return WorldType == EWorldType::Game || 
		       WorldType == EWorldType::PIE || 
		       WorldType == EWorldType::GamePreview;
	}
	return false;
}

void UTimeWorldBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIsInitialized = true;
	UE_LOG(LogUETPFCore, Log, TEXT("TimeWorldBridgeSubsystem: Initialized"));
}

void UTimeWorldBridgeSubsystem::Deinitialize()
{
	bIsInitialized = false;
	Super::Deinitialize();
}

TStatId UTimeWorldBridgeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeWorldBridgeSubsystem, STATGROUP_Tickables);
}

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
	SCOPE_CYCLE_COUNTER(STAT_TimeWorldBridgeTick);

	// DeltaTime comes from the engine tick — no manual wall-clock tracking needed.
	// FTickableGameObject receives the same delta as AActor::Tick, clamped by engine max delta.
	//
	// TASK GRAPH NOTE: Time::Advance broadcasts OnSimTimeAdvanced synchronously. If downstream
	// subscriber work grows (e.g. per-body orbital updates, biome blending), consider dispatching
	// those as UE::Tasks::Launch tasks here and joining before the physics tick group.
	// For now all subscribers are cheap O(1) operations and game-thread-only is correct.

	if (UTimeSubsystem* Time = GetTime())
	{
		// Startup diagnostics — member variable resets per world/PIE session.
		if (AdvanceDiagnosticCount < 3)
		{
			UE_LOG(LogUETPFCore, Log, TEXT("TimeWorldBridgeSubsystem::Tick #%d - DeltaTime: %.4f"),
				AdvanceDiagnosticCount, DeltaTime);
			AdvanceDiagnosticCount++;
		}

		Time->Advance(DeltaTime);
	}
	else
	{
		if (!bErrorLogged)
		{
			UE_LOG(LogUETPFCore, Error, TEXT("TimeWorldBridgeSubsystem: TimeSubsystem not found - time will not advance"));
			bErrorLogged = true;
		}
	}
}
