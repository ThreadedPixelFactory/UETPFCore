// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/TimeSubsystem.h"
#include "Engine/World.h"
#include "Misc/ScopeLock.h"

void UTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ClampAndValidate();
}

void UTimeSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTimeSubsystem::SetPaused(bool bInPaused)
{
	bPaused = bInPaused;
}

void UTimeSubsystem::SetTimeScale(double InTimeScale)
{
	if (!bAllowNegativeTimeScale)
	{
		InTimeScale = FMath::Max(0.0, InTimeScale);
	}
	TimeScale = InTimeScale;
	ClampAndValidate();
}

void UTimeSubsystem::SetAllowNegativeTimeScale(bool bAllow)
{
	bAllowNegativeTimeScale = bAllow;
	if (!bAllowNegativeTimeScale)
	{
		TimeScale = FMath::Max(0.0, TimeScale);
	}
}

void UTimeSubsystem::SetClockMode(ESimClockMode InMode)
{
	ClockMode = InMode;
	Accumulator = 0.0;
}

void UTimeSubsystem::SetFixedStepSeconds(double InFixedStepSeconds)
{
	FixedStepSeconds = FMath::Clamp(InFixedStepSeconds, 1.0 / 240.0, 1.0); // 240Hz..1Hz sync guard
}

void UTimeSubsystem::ClampAndValidate()
{
	// Keep sane bounds; you can widen later.
	if (!bAllowNegativeTimeScale)
	{
		TimeScale = FMath::Clamp(TimeScale, 0.0, 1000.0);
	}
	else
	{
		TimeScale = FMath::Clamp(TimeScale, -1000.0, 1000.0);
	}
}

void UTimeSubsystem::Advance(double RealDeltaSeconds)
{
	if (bPaused || FMath::IsNearlyZero(TimeScale))
	{
		LastStepSeconds = 0.0;
		return;
	}

	// Scaled simulation delta
	const double ScaledDelta = RealDeltaSeconds * TimeScale;

	if (ClockMode == ESimClockMode::RealTime)
	{
		SimTimeSeconds += ScaledDelta;
		LastStepSeconds = ScaledDelta;
		OnSimTimeAdvanced.Broadcast(SimTimeSeconds);
		return;
	}

	// Fixed-step deterministic advancement:
	Accumulator += ScaledDelta;

	// Handle negative time scale (rewind) if allowed:
	if (Accumulator >= 0.0)
	{
		while (Accumulator >= FixedStepSeconds)
		{
			SimTimeSeconds += FixedStepSeconds;
			Accumulator -= FixedStepSeconds;
			LastStepSeconds = FixedStepSeconds;
			OnSimTimeAdvanced.Broadcast(SimTimeSeconds);
		}
	}
	else
	{
		while (Accumulator <= -FixedStepSeconds)
		{
			SimTimeSeconds -= FixedStepSeconds;
			Accumulator += FixedStepSeconds;
			LastStepSeconds = -FixedStepSeconds;
			OnSimTimeAdvanced.Broadcast(SimTimeSeconds);
		}
	}
}
