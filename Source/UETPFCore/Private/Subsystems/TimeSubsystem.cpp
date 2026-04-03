// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "Subsystems/TimeSubsystem.h"
#include "Engine/World.h"

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
	// Startup diagnostics — verifies time is flowing correctly. Member variable resets per GameInstance.
	if (AdvanceDiagnosticCount < 3)
	{
		UE_LOG(LogUETPFCore, Log, TEXT("TimeSubsystem::Advance #%d - RealDelta: %.4f, Paused: %d, TimeScale: %.2f"),
			AdvanceDiagnosticCount, RealDeltaSeconds, bPaused, TimeScale);
		AdvanceDiagnosticCount++;
	}

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

	// Fixed-step deterministic advancement.
	//
	// TASK GRAPH NOTE: OnSimTimeAdvanced subscribers (SolarSystemSubsystem, UniversalSkyActor, etc.)
	// currently run synchronously here on the game thread. If per-step work becomes expensive,
	// each broadcast could dispatch a UE::Tasks::Launch task instead, with a join before the
	// next physics tick. Keep subscriber work cheap until that becomes necessary.
	Accumulator += ScaledDelta;

	// Guard against spiral-of-death: cap steps per frame to prevent cascade
	// when frame time spikes (e.g. loading, debugging, editor stalls).
	static constexpr int32 MaxStepsPerFrame = 8;
	int32 StepCount = 0;

	if (Accumulator >= 0.0)
	{
		while (Accumulator >= FixedStepSeconds && StepCount < MaxStepsPerFrame)
		{
			SimTimeSeconds += FixedStepSeconds;
			Accumulator -= FixedStepSeconds;
			LastStepSeconds = FixedStepSeconds;
			OnSimTimeAdvanced.Broadcast(SimTimeSeconds);
			++StepCount;
		}
		// If capped, discard excess accumulation to prevent deferred catch-up.
		if (StepCount == MaxStepsPerFrame)
		{
			Accumulator = 0.0;
		}
	}
	else
	{
		while (Accumulator <= -FixedStepSeconds && StepCount < MaxStepsPerFrame)
		{
			SimTimeSeconds -= FixedStepSeconds;
			Accumulator += FixedStepSeconds;
			LastStepSeconds = -FixedStepSeconds;
			OnSimTimeAdvanced.Broadcast(SimTimeSeconds);
			++StepCount;
		}
		if (StepCount == MaxStepsPerFrame)
		{
			Accumulator = 0.0;
		}
	}
}
