// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "TimeWorldBridgeSubsystem.generated.h"

class UTimeSubsystem;

/**
 * Per-world bridge to game instance TimeSubsystem.
 * Uses FTickableGameObject for time advancement - runs once per frame,
 * NOT via FTimerHandle (which accumulates during stalls).
 */
UCLASS()
class UETPFCORE_API UTimeWorldBridgeSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bIsInitialized; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

	UTimeSubsystem* GetTime() const;

private:
	// Cache optional (safe; GI persists across maps)
	mutable TWeakObjectPtr<UTimeSubsystem> CachedTime;

	// Initialization flag for tickable
	bool bIsInitialized = false;

	// Diagnostic logging state (member variables, reset per world/PIE session)
	int32 AdvanceDiagnosticCount = 0;
	bool bErrorLogged = false;
};
