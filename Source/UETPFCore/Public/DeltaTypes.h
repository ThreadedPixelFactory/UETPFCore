// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "SpecTypes.h"
#include "DeltaTypes.generated.h"

//=============================================================================
// DELTA TYPE IDENTIFIERS
//=============================================================================

/**
 * Types of deltas that can be persisted.
 */
UENUM(BlueprintType)
enum class EDeltaType : uint8
{
	/** Surface tile delta (snow, wetness, compaction, etc.) */
	SurfaceTile,
	/** Fracture state (broken chunks, transforms) */
	Fracture,
	/** Actor transform (settled debris, moved objects) */
	Transform,
	/** Spawn delta (PCG instance promoted to actor) */
	Spawn,
	/** Remove delta (actor destroyed/removed) */
	Remove,
	/** Assembly state (machine/vehicle configuration + damage) */
	Assembly
};

//=============================================================================
// WORLD CELL KEY - Spatial indexing for deltas
//=============================================================================

/**
 * Key for identifying a world partition cell or tile.
 * Used for spatial indexing of deltas.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FWorldCellKey
{
	GENERATED_BODY()

	/** X coordinate in cell grid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
	int32 X = 0;

	/** Y coordinate in cell grid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
	int32 Y = 0;

	/** Level of detail / cell size tier (0 = finest) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
	int32 LOD = 0;

	FWorldCellKey() = default;
	FWorldCellKey(int32 InX, int32 InY, int32 InLOD = 0) : X(InX), Y(InY), LOD(InLOD) {}

	bool operator==(const FWorldCellKey& Other) const 
	{ 
		return X == Other.X && Y == Other.Y && LOD == Other.LOD; 
	}

	bool operator!=(const FWorldCellKey& Other) const 
	{ 
		return !(*this == Other); 
	}

	friend uint32 GetTypeHash(const FWorldCellKey& Key)
	{
		return HashCombine(HashCombine(GetTypeHash(Key.X), GetTypeHash(Key.Y)), GetTypeHash(Key.LOD));
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(%d,%d,LOD%d)"), X, Y, LOD);
	}

	/** Convert world location to cell key */
	static FWorldCellKey FromWorldLocation(const FVector& Location, float CellSize = 6400.0f, int32 LOD = 0);

	/** Get world bounds for this cell */
	FBox GetWorldBounds(float CellSize = 6400.0f) const;
};

//=============================================================================
// SURFACE TILE DELTA - Snow/wetness/compaction changes
//=============================================================================

/**
 * Delta operation type for surface tiles.
 */
UENUM(BlueprintType)
enum class ESurfaceDeltaOperation : uint8
{
	/** Set value directly */
	Set,
	/** Add to existing value */
	Add,
	/** Subtract from existing value */
	Subtract,
	/** Multiply existing value */
	Multiply,
	/** Set to maximum of current and new value */
	Max,
	/** Set to minimum of current and new value */
	Min
};

/**
 * Channel in a surface delta tile.
 */
UENUM(BlueprintType)
enum class ESurfaceDeltaChannel : uint8
{
	/** Snow depth (cm) */
	SnowDepth,
	/** Snow compaction (0-1) */
	SnowCompaction,
	/** Wetness level (0-1) */
	Wetness,
	/** Temperature deviation from ambient (Kelvin) */
	TemperatureDelta,
	/** Toxicity level (0-1) */
	Toxicity,
	/** Custom channel A */
	CustomA,
	/** Custom channel B */
	CustomB
};

/**
 * A single surface tile delta record.
 * Represents a localized change to surface state.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FSurfaceTileDelta
{
	GENERATED_BODY()

	/** World cell containing this tile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FWorldCellKey CellKey;

	/** Local tile index within the cell (for sub-cell resolution) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	int32 TileIndex = 0;

	/** World location (center of affected area) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FVector WorldLocation = FVector::ZeroVector;

	/** Radius of effect (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	float Radius = 50.0f;

	/** Channel being modified */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delta")
	ESurfaceDeltaChannel Channel = ESurfaceDeltaChannel::SnowDepth;

	/** Operation to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delta")
	ESurfaceDeltaOperation Operation = ESurfaceDeltaOperation::Add;

	/** Value for the operation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delta")
	float Value = 0.0f;

	/** Timestamp when this delta was created */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	double Timestamp = 0.0;

	/** Optional: Player who caused this delta */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	FName AuthorPlayerId;

	FSurfaceTileDelta() = default;
};

//=============================================================================
// FRACTURE DELTA - Destruction state
//=============================================================================

/**
 * A fracture delta record.
 * Records which chunks of a GeometryCollection broke and their final states.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FFractureDelta
{
	GENERATED_BODY()

	/** World cell containing this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FWorldCellKey CellKey;

	/** Unique ID of the fractured actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid ActorGuid;

	/** Spec ID for damage behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FDamageSpecId DamageSpecId;

	/** List of broken chunk indices */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	TArray<int32> BrokenChunks;

	/** Whether the debris is currently sleeping (frozen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsSleeping = true;

	/** Timestamp when this delta was created */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	double Timestamp = 0.0;

	FFractureDelta() = default;
};

//=============================================================================
// TRANSFORM DELTA - Settled object positions
//=============================================================================

/**
 * A transform delta record.
 * Records the final resting position of a physics object.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FTransformDelta
{
	GENERATED_BODY()

	/** World cell containing this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FWorldCellKey CellKey;

	/** Unique ID of the actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid ActorGuid;

	/** Final transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FTransform Transform;

	/** Whether physics is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bPhysicsEnabled = false;

	/** Whether the object is sleeping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsSleeping = true;

	/** Timestamp when this delta was created */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	double Timestamp = 0.0;

	FTransformDelta() = default;
};

//=============================================================================
// SPAWN DELTA - PCG instance promotion
//=============================================================================

/**
 * A spawn delta record.
 * Records when a PCG instance was promoted to a real actor.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FSpawnDelta
{
	GENERATED_BODY()

	/** World cell containing this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FWorldCellKey CellKey;

	/** Unique ID of the spawned actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid ActorGuid;

	/** Original PCG instance ID (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	int64 PCGInstanceId = -1;

	/** Class of the spawned actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	TSoftClassPtr<AActor> ActorClass;

	/** Spawn transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FTransform SpawnTransform;

	/** Timestamp when this delta was created */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	double Timestamp = 0.0;

	FSpawnDelta() = default;
};

//=============================================================================
// REMOVE DELTA - Destroyed/removed objects
//=============================================================================

/**
 * A remove delta record.
 * Records when an actor was destroyed or removed.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FRemoveDelta
{
	GENERATED_BODY()

	/** World cell containing this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FWorldCellKey CellKey;

	/** Unique ID of the removed actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid ActorGuid;

	/** Reason for removal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName RemovalReason;

	/** Timestamp when this delta was created */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	double Timestamp = 0.0;

	FRemoveDelta() = default;
};

//=============================================================================
// ASSEMBLY DELTA - Machine/vehicle state
//=============================================================================

/**
 * An assembly delta record.
 * Records state of complex machines/vehicles including damage and configuration.
 */
USTRUCT(BlueprintType)
struct UETPFCORE_API FAssemblyDelta
{
	GENERATED_BODY()

	/** World cell containing this assembly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	FWorldCellKey CellKey;

	/** Unique ID of the assembly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid AssemblyGuid;

	/** Assembly spec ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName AssemblySpecId;

	/** Current transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FTransform Transform;

	/** Whether the assembly is sleeping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsSleeping = true;

	/** List of damaged/disabled part indices */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	TArray<int32> DamagedParts;

	/** Key-value pairs for continuous state variables (fuel, temp, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	TMap<FName, float> StateVariables;

	/** Timestamp when this delta was created */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	double Timestamp = 0.0;

	FAssemblyDelta() = default;
};

//=============================================================================
// DELTA STORE INTERFACE
//=============================================================================

/**
 * Abstract interface for delta storage.
 * Implement this to provide local file storage (single-player) or
 * server/database storage (multiplayer).
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UDeltaStore : public UInterface
{
	GENERATED_BODY()
};

class UETPFCORE_API IDeltaStore
{
	GENERATED_BODY()

public:
	/** Append a surface tile delta */
	virtual void AppendSurfaceDelta(const FSurfaceTileDelta& Delta) = 0;

	/** Append a fracture delta */
	virtual void AppendFractureDelta(const FFractureDelta& Delta) = 0;

	/** Append a transform delta */
	virtual void AppendTransformDelta(const FTransformDelta& Delta) = 0;

	/** Append a spawn delta */
	virtual void AppendSpawnDelta(const FSpawnDelta& Delta) = 0;

	/** Append a remove delta */
	virtual void AppendRemoveDelta(const FRemoveDelta& Delta) = 0;

	/** Append an assembly delta */
	virtual void AppendAssemblyDelta(const FAssemblyDelta& Delta) = 0;

	/** Get all surface deltas for a cell */
	virtual TArray<FSurfaceTileDelta> GetSurfaceDeltas(const FWorldCellKey& CellKey) const = 0;

	/** Get all fracture deltas for a cell */
	virtual TArray<FFractureDelta> GetFractureDeltas(const FWorldCellKey& CellKey) const = 0;

	/** Get all transform deltas for a cell */
	virtual TArray<FTransformDelta> GetTransformDeltas(const FWorldCellKey& CellKey) const = 0;

	/** Get all spawn deltas for a cell */
	virtual TArray<FSpawnDelta> GetSpawnDeltas(const FWorldCellKey& CellKey) const = 0;

	/** Get all remove deltas for a cell */
	virtual TArray<FRemoveDelta> GetRemoveDeltas(const FWorldCellKey& CellKey) const = 0;

	/** Get all assembly deltas for a cell */
	virtual TArray<FAssemblyDelta> GetAssemblyDeltas(const FWorldCellKey& CellKey) const = 0;

	/** Clear all deltas for a cell (for testing/reset) */
	virtual void ClearCellDeltas(const FWorldCellKey& CellKey) = 0;

	/** Flush pending writes to storage */
	virtual void Flush() = 0;
};
