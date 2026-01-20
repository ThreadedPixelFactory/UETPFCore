// Copyright 2026 Threaded Pixel Factory
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * File Delta Store - Single-Player World State Persistence
 * 
 * Purpose:
 * Implements IDeltaStore using local JSON files for single-player world persistence.
 * Stores all world changes (surface modifications, destruction, moved objects) to disk.
 * 
 * Architecture:
 * - In-memory cache of deltas per cell (TMap<FWorldCellKey, TArray<Delta>>)
 * - Dirty tracking for modified cells
 * - Flush() writes dirty cells to JSON files
 * - Lazy loading: cells loaded on first query
 * 
 * File Structure:
 *   SaveGames/
 *     └── [WorldName]/
 *         ├── cell_0_0_0.json      (Surface/Transform deltas for cell 0,0)
 *         ├── cell_1_0_0.json      (Next cell over)
 *         └── fractures.json       (All fracture deltas, indexed by cell)
 * 
 * Delta Types:
 * - SurfaceTileDelta: Snow, wetness, compaction changes
 * - FractureDelta: Broken geometry (GeometryCollection chunks)
 * - TransformDelta: Moved/settled physics objects
 * - SpawnDelta: PCG instances promoted to actors
 * - RemoveDelta: Destroyed/removed objects
 * - AssemblyDelta: Machine/vehicle state + damage
 * 
 * Usage:
 * \code{.cpp}
 *   UFileDeltaStore* Store = NewObject<UFileDeltaStore>();
 *   Store->Initialize(TEXT("MyWorld"));
 *   
 *   // Append a delta
 *   FSurfaceTileDelta Delta;
 *   Delta.CellKey = FWorldCellKey(0, 0);
 *   Delta.Channel = ESurfaceDeltaChannel::SnowDepth;
 *   Delta.Value = 10.0f; // 10cm snow
 *   Store->AppendSurfaceDelta(Delta);
 *   
 *   // Save to disk
 *   Store->Flush();
 *   
 *   // Later: Query deltas
 *   TArray<FSurfaceTileDelta> Deltas = Store->GetSurfaceDeltas(FWorldCellKey(0, 0));
 * \endcode
 * 
 * Performance:
 * - Append operations: O(1) in-memory
 * - Query operations: O(1) hash lookup + O(n) array scan
 * - Flush: O(dirty cells) file writes
 * - Lazy loading: Only loads cells as queried
 * 
 * Multiplayer Notes:
 * - This implementation is single-player only
 * - For multiplayer, implement IDeltaStore with server/database backend
 * - Delta format is the same, only storage mechanism changes
 * 
 * @see IDeltaStore for interface definition
 * @see DeltaTypes.h for delta data structures
 */

#pragma once

#include "CoreMinimal.h"
#include "DeltaTypes.h"
#include "FileDeltaStore.generated.h"

/**
 * File-based delta store for single-player persistence.
 * 
 * Implements IDeltaStore using local JSON files for world state persistence.
 * Suitable for single-player games where world state is saved locally.
 * 
 * Lifecycle:
 * 1. Initialize(WorldName) - Sets up save directory structure
 * 2. Append*Delta() - Add deltas to in-memory cache (marks cell dirty)
 * 3. Get*Deltas() - Query deltas for a cell (lazy loads from disk)
 * 4. Flush() - Write all dirty cells to disk
 * 5. Repeat 2-4 during gameplay
 * 
 * Thread Safety:
 * - NOT thread-safe - all operations must be on game thread
 * - File I/O is synchronous (consider async for production)
 * 
 * Optimization Opportunities:
 * - Async file I/O for Flush()
 * - Delta compression for large cell data
 * - Binary format (CBOR) instead of JSON for smaller files
 * - Periodic auto-flush based on time/delta count
 * 
 * @note For multiplayer, replace with server-authoritative delta store
 * @see IDeltaStore for interface contract
 */
UCLASS(BlueprintType)
class SINGLEPLAYERSTORYTEMPLATE_API UFileDeltaStore : public UObject, public IDeltaStore
{
	GENERATED_BODY()

public:
	UFileDeltaStore();

	// IDeltaStore Interface
	virtual void AppendSurfaceDelta(const FSurfaceTileDelta& Delta) override;
	virtual void AppendFractureDelta(const FFractureDelta& Delta) override;
	virtual void AppendTransformDelta(const FTransformDelta& Delta) override;
	virtual void AppendSpawnDelta(const FSpawnDelta& Delta) override;
	virtual void AppendRemoveDelta(const FRemoveDelta& Delta) override;
	virtual void AppendAssemblyDelta(const FAssemblyDelta& Delta) override;
	
	virtual TArray<FSurfaceTileDelta> GetSurfaceDeltas(const FWorldCellKey& CellKey) const override;
	virtual TArray<FFractureDelta> GetFractureDeltas(const FWorldCellKey& CellKey) const override;
	virtual TArray<FTransformDelta> GetTransformDeltas(const FWorldCellKey& CellKey) const override;
	virtual TArray<FSpawnDelta> GetSpawnDeltas(const FWorldCellKey& CellKey) const override;
	virtual TArray<FRemoveDelta> GetRemoveDeltas(const FWorldCellKey& CellKey) const override;
	virtual TArray<FAssemblyDelta> GetAssemblyDeltas(const FWorldCellKey& CellKey) const override;
	
	virtual void ClearCellDeltas(const FWorldCellKey& CellKey) override;
	virtual void Flush() override;

	/**
	 * Initialize the delta store for a specific world.
	 * Must be called before any Append/Get operations.
	 * 
	 * Initialization Steps:
	 * 1. Sets CurrentWorldName
	 * 2. Creates save directory: SaveGames/{WorldName}/
	 * 3. Clears in-memory caches
	 * 4. Marks as initialized
	 * 
	 * @param WorldName - Unique identifier for this world (e.g., "MyWorld", "Level_01")
	 *                    Used as subdirectory name under SaveGames/
	 * 
	 * @return true if initialization succeeded, false on error (e.g., directory creation failed)
	 * 
	 * @note Safe to call multiple times (will reinitialize)
	 * @note Does not load existing deltas - loading is lazy on first query
	 * @note WorldName should be filesystem-safe (no special characters)
	 * 
	 * Example:
	 * \code{.cpp}
	 *   UFileDeltaStore* Store = NewObject<UFileDeltaStore>();
	 *   if (!Store->Initialize(TEXT("Sandbox_World"))) {
	 *     UE_LOG(LogTemp, Error, TEXT("Failed to initialize delta store"));
	 *   }
	 * \endcode
	 */
	UFUNCTION(BlueprintCallable, Category = "DeltaStore")
	bool Initialize(const FString& WorldName);

	UFUNCTION(BlueprintCallable, Category = "DeltaStore")
	bool IsInitialized() const { return bIsInitialized; }

private:
	FString CurrentWorldName;
	FString BaseSaveDirectory;
	bool bIsInitialized = false;

	// In-memory caches keyed by cell
	TMap<FWorldCellKey, TArray<FSurfaceTileDelta>> SurfaceDeltas;
	TMap<FWorldCellKey, TArray<FFractureDelta>> FractureDeltas;
	TMap<FWorldCellKey, TArray<FTransformDelta>> TransformDeltas;
	TMap<FWorldCellKey, TArray<FSpawnDelta>> SpawnDeltas;
	TMap<FWorldCellKey, TArray<FRemoveDelta>> RemoveDeltas;
	TMap<FWorldCellKey, TArray<FAssemblyDelta>> AssemblyDeltas;
	
	TSet<FWorldCellKey> DirtyCells;
};
