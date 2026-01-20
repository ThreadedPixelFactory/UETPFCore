// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeWorldBridgeSubsystem.generated.h"

class UTimeSubsystem;

/**
 * Per-world bridge to game instance TimeSubsystem.
 * Ticks each frame and forwards delta time to the central simulation clock.
 */
UCLASS()
class UETPFCORE_API UTimeWorldBridgeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeWorldBridgeSubsystem, STATGROUP_Tickables); }

	UTimeSubsystem* GetTime() const;

private:
	// Cache optional (safe; GI persists across maps)
	mutable TWeakObjectPtr<UTimeSubsystem> CachedTime;
};
