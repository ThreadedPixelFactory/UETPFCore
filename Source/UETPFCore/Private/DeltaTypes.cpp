// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#include "DeltaTypes.h"

//=============================================================================
// FWorldCellKey
//=============================================================================

FWorldCellKey FWorldCellKey::FromWorldLocation(const FVector& Location, float CellSize, int32 LOD)
{
	// Apply LOD scaling to cell size
	float EffectiveCellSize = CellSize * FMath::Pow(2.0f, static_cast<float>(LOD));
	
	// Compute cell coordinates (floor division for negative values)
	int32 CellX = FMath::FloorToInt(Location.X / EffectiveCellSize);
	int32 CellY = FMath::FloorToInt(Location.Y / EffectiveCellSize);
	
	return FWorldCellKey(CellX, CellY, LOD);
}

FBox FWorldCellKey::GetWorldBounds(float CellSize) const
{
	// Apply LOD scaling to cell size
	float EffectiveCellSize = CellSize * FMath::Pow(2.0f, static_cast<float>(LOD));
	
	// Compute world-space bounds
	FVector MinPoint(
		static_cast<float>(X) * EffectiveCellSize,
		static_cast<float>(Y) * EffectiveCellSize,
		-HALF_WORLD_MAX
	);
	
	FVector MaxPoint(
		static_cast<float>(X + 1) * EffectiveCellSize,
		static_cast<float>(Y + 1) * EffectiveCellSize,
		HALF_WORLD_MAX
	);
	
	return FBox(MinPoint, MaxPoint);
}
