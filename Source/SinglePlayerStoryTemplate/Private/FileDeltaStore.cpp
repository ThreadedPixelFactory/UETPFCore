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

#include "FileDeltaStore.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Tasks/Task.h"
#include "Tasks/Pipe.h"

UFileDeltaStore::UFileDeltaStore()
{
	BaseSaveDirectory = FPaths::ProjectSavedDir() / TEXT("GameSaveData");
}

bool UFileDeltaStore::Initialize(const FString& WorldName)
{
	CurrentWorldName = WorldName;
	FString WorldDir = BaseSaveDirectory / WorldName;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	if (!PlatformFile.DirectoryExists(*WorldDir))
	{
		if (!PlatformFile.CreateDirectoryTree(*WorldDir))
		{
			UE_LOG(LogTemp, Error, TEXT("FileDeltaStore: Failed to create directory: %s"), *WorldDir);
			return false;
		}
	}

	bIsInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("FileDeltaStore initialized for world: %s"), *WorldName);
	return true;
}

// IDeltaStore: Append methods
void UFileDeltaStore::AppendSurfaceDelta(const FSurfaceTileDelta& Delta)
{
	SurfaceDeltas.FindOrAdd(Delta.CellKey).Add(Delta);
	DirtyCells.Add(Delta.CellKey);
}

void UFileDeltaStore::AppendFractureDelta(const FFractureDelta& Delta)
{
	FractureDeltas.FindOrAdd(Delta.CellKey).Add(Delta);
	DirtyCells.Add(Delta.CellKey);
}

void UFileDeltaStore::AppendTransformDelta(const FTransformDelta& Delta)
{
	TransformDeltas.FindOrAdd(Delta.CellKey).Add(Delta);
	DirtyCells.Add(Delta.CellKey);
}

void UFileDeltaStore::AppendSpawnDelta(const FSpawnDelta& Delta)
{
	SpawnDeltas.FindOrAdd(Delta.CellKey).Add(Delta);
	DirtyCells.Add(Delta.CellKey);
}

void UFileDeltaStore::AppendRemoveDelta(const FRemoveDelta& Delta)
{
	RemoveDeltas.FindOrAdd(Delta.CellKey).Add(Delta);
	DirtyCells.Add(Delta.CellKey);
}

void UFileDeltaStore::AppendAssemblyDelta(const FAssemblyDelta& Delta)
{
	AssemblyDeltas.FindOrAdd(Delta.CellKey).Add(Delta);
	DirtyCells.Add(Delta.CellKey);
}

// IDeltaStore: Get methods
TArray<FSurfaceTileDelta> UFileDeltaStore::GetSurfaceDeltas(const FWorldCellKey& CellKey) const
{
	if (const TArray<FSurfaceTileDelta>* Found = SurfaceDeltas.Find(CellKey))
	{
		return *Found;
	}
	return TArray<FSurfaceTileDelta>();
}

TArray<FFractureDelta> UFileDeltaStore::GetFractureDeltas(const FWorldCellKey& CellKey) const
{
	if (const TArray<FFractureDelta>* Found = FractureDeltas.Find(CellKey))
	{
		return *Found;
	}
	return TArray<FFractureDelta>();
}

TArray<FTransformDelta> UFileDeltaStore::GetTransformDeltas(const FWorldCellKey& CellKey) const
{
	if (const TArray<FTransformDelta>* Found = TransformDeltas.Find(CellKey))
	{
		return *Found;
	}
	return TArray<FTransformDelta>();
}

TArray<FSpawnDelta> UFileDeltaStore::GetSpawnDeltas(const FWorldCellKey& CellKey) const
{
	if (const TArray<FSpawnDelta>* Found = SpawnDeltas.Find(CellKey))
	{
		return *Found;
	}
	return TArray<FSpawnDelta>();
}

TArray<FRemoveDelta> UFileDeltaStore::GetRemoveDeltas(const FWorldCellKey& CellKey) const
{
	if (const TArray<FRemoveDelta>* Found = RemoveDeltas.Find(CellKey))
	{
		return *Found;
	}
	return TArray<FRemoveDelta>();
}

TArray<FAssemblyDelta> UFileDeltaStore::GetAssemblyDeltas(const FWorldCellKey& CellKey) const
{
	if (const TArray<FAssemblyDelta>* Found = AssemblyDeltas.Find(CellKey))
	{
		return *Found;
	}
	return TArray<FAssemblyDelta>();
}

void UFileDeltaStore::ClearCellDeltas(const FWorldCellKey& CellKey)
{
	SurfaceDeltas.Remove(CellKey);
	FractureDeltas.Remove(CellKey);
	TransformDeltas.Remove(CellKey);
	SpawnDeltas.Remove(CellKey);
	RemoveDeltas.Remove(CellKey);
	AssemblyDeltas.Remove(CellKey);
	DirtyCells.Remove(CellKey);
}

void UFileDeltaStore::Flush()
{
	if (!bIsInitialized || DirtyCells.Num() == 0)
	{
		return;
	}

	// Capture data for async task (copies to avoid race conditions)
	TArray<FWorldCellKey> CellsToFlush = DirtyCells.Array();
	FString SaveDir = BaseSaveDirectory / CurrentWorldName;
	
	// Capture delta arrays for each dirty cell
	TMap<FWorldCellKey, TArray<FSurfaceTileDelta>> SurfaceCopy;
	TMap<FWorldCellKey, TArray<FFractureDelta>> FractureCopy;
	TMap<FWorldCellKey, TArray<FTransformDelta>> TransformCopy;
	TMap<FWorldCellKey, TArray<FSpawnDelta>> SpawnCopy;
	TMap<FWorldCellKey, TArray<FRemoveDelta>> RemoveCopy;
	TMap<FWorldCellKey, TArray<FAssemblyDelta>> AssemblyCopy;

	for (const FWorldCellKey& CellKey : CellsToFlush)
	{
		if (const TArray<FSurfaceTileDelta>* Found = SurfaceDeltas.Find(CellKey)) { SurfaceCopy.Add(CellKey, *Found); }
		if (const TArray<FFractureDelta>* Found = FractureDeltas.Find(CellKey)) { FractureCopy.Add(CellKey, *Found); }
		if (const TArray<FTransformDelta>* Found = TransformDeltas.Find(CellKey)) { TransformCopy.Add(CellKey, *Found); }
		if (const TArray<FSpawnDelta>* Found = SpawnDeltas.Find(CellKey)) { SpawnCopy.Add(CellKey, *Found); }
		if (const TArray<FRemoveDelta>* Found = RemoveDeltas.Find(CellKey)) { RemoveCopy.Add(CellKey, *Found); }
		if (const TArray<FAssemblyDelta>* Found = AssemblyDeltas.Find(CellKey)) { AssemblyCopy.Add(CellKey, *Found); }
	}

	DirtyCells.Empty();

	// Launch async task for file I/O - takes work OFF game thread
	UE::Tasks::Launch(
		UE_SOURCE_LOCATION,
		[SaveDir, CellsToFlush, 
		 SurfaceCopy = MoveTemp(SurfaceCopy),
		 FractureCopy = MoveTemp(FractureCopy),
		 TransformCopy = MoveTemp(TransformCopy),
		 SpawnCopy = MoveTemp(SpawnCopy),
		 RemoveCopy = MoveTemp(RemoveCopy),
		 AssemblyCopy = MoveTemp(AssemblyCopy)]()
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			for (const FWorldCellKey& CellKey : CellsToFlush)
			{
				FString CellDir = SaveDir / CellKey.ToString();
				PlatformFile.CreateDirectoryTree(*CellDir);

				// Build JSON for this cell's deltas
				TSharedRef<FJsonObject> CellData = MakeShared<FJsonObject>();
				CellData->SetStringField(TEXT("cell"), CellKey.ToString());
				CellData->SetNumberField(TEXT("timestamp"), FDateTime::UtcNow().ToUnixTimestamp());

				// Surface deltas
				if (const TArray<FSurfaceTileDelta>* Found = SurfaceCopy.Find(CellKey))
				{
					TArray<TSharedPtr<FJsonValue>> SurfaceArray;
					for (const FSurfaceTileDelta& Delta : *Found)
					{
						TSharedPtr<FJsonObject> DeltaObj = FJsonObjectConverter::UStructToJsonObject(Delta);
						if (DeltaObj.IsValid())
						{
							SurfaceArray.Add(MakeShared<FJsonValueObject>(DeltaObj));
						}
					}
					CellData->SetArrayField(TEXT("surface_deltas"), SurfaceArray);
				}

				// Fracture deltas  
				if (const TArray<FFractureDelta>* Found = FractureCopy.Find(CellKey))
				{
					TArray<TSharedPtr<FJsonValue>> Array;
					for (const FFractureDelta& Delta : *Found)
					{
						TSharedPtr<FJsonObject> DeltaObj = FJsonObjectConverter::UStructToJsonObject(Delta);
						if (DeltaObj.IsValid()) { Array.Add(MakeShared<FJsonValueObject>(DeltaObj)); }
					}
					CellData->SetArrayField(TEXT("fracture_deltas"), Array);
				}

				// Transform deltas
				if (const TArray<FTransformDelta>* Found = TransformCopy.Find(CellKey))
				{
					TArray<TSharedPtr<FJsonValue>> Array;
					for (const FTransformDelta& Delta : *Found)
					{
						TSharedPtr<FJsonObject> DeltaObj = FJsonObjectConverter::UStructToJsonObject(Delta);
						if (DeltaObj.IsValid()) { Array.Add(MakeShared<FJsonValueObject>(DeltaObj)); }
					}
					CellData->SetArrayField(TEXT("transform_deltas"), Array);
				}

				// Spawn deltas
				if (const TArray<FSpawnDelta>* Found = SpawnCopy.Find(CellKey))
				{
					TArray<TSharedPtr<FJsonValue>> Array;
					for (const FSpawnDelta& Delta : *Found)
					{
						TSharedPtr<FJsonObject> DeltaObj = FJsonObjectConverter::UStructToJsonObject(Delta);
						if (DeltaObj.IsValid()) { Array.Add(MakeShared<FJsonValueObject>(DeltaObj)); }
					}
					CellData->SetArrayField(TEXT("spawn_deltas"), Array);
				}

				// Remove deltas
				if (const TArray<FRemoveDelta>* Found = RemoveCopy.Find(CellKey))
				{
					TArray<TSharedPtr<FJsonValue>> Array;
					for (const FRemoveDelta& Delta : *Found)
					{
						TSharedPtr<FJsonObject> DeltaObj = FJsonObjectConverter::UStructToJsonObject(Delta);
						if (DeltaObj.IsValid()) { Array.Add(MakeShared<FJsonValueObject>(DeltaObj)); }
					}
					CellData->SetArrayField(TEXT("remove_deltas"), Array);
				}

				// Assembly deltas
				if (const TArray<FAssemblyDelta>* Found = AssemblyCopy.Find(CellKey))
				{
					TArray<TSharedPtr<FJsonValue>> Array;
					for (const FAssemblyDelta& Delta : *Found)
					{
						TSharedPtr<FJsonObject> DeltaObj = FJsonObjectConverter::UStructToJsonObject(Delta);
						if (DeltaObj.IsValid()) { Array.Add(MakeShared<FJsonValueObject>(DeltaObj)); }
					}
					CellData->SetArrayField(TEXT("assembly_deltas"), Array);
				}

				// Write JSON to file
				FString JsonString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
				FJsonSerializer::Serialize(CellData, Writer);

				FString FilePath = CellDir / TEXT("deltas.json");
				FFileHelper::SaveStringToFile(JsonString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
			}

			UE_LOG(LogTemp, Verbose, TEXT("FileDeltaStore: Async flush complete for %d cells"), CellsToFlush.Num());
		}
	);

	UE_LOG(LogTemp, Verbose, TEXT("FileDeltaStore: Queued async flush for %d cells"), CellsToFlush.Num());
}
