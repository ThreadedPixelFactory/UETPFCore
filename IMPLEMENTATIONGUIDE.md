<!--
Copyright Threaded Pixel Factory. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# Implementation Guide - UETPFCore

## Integrating UETPFCore into Your Game

This guide shows you how to use UETPFCore as a foundation for your own game project.

## Table of Contents
1. [Project Setup](#project-setup)
2. [Creating Your Game Module](#creating-your-game-module)
3. [Defining Data Assets](#defining-data-assets)
4. [Querying Subsystems](#querying-subsystems)
5. [Implementing Persistence](#implementing-persistence)
6. [Integrating with Physics](#integrating-with-physics)
7. [Best Practices](#best-practices)

---

## Project Setup

### Option A: Fork This Template
1. Fork or clone UETPFCore
2. Rename `SinglePlayerStoryTemplate` to your program name
3. Update `.uproject` with your program module name
4. Update Target.cs files with new module name

### Option B: Add to Existing Project
1. Copy `Source/UETPFCore/` to your project's `Source/` folder
2. Add module dependency in your Build.cs:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
    "UETPFCore",  // Add this line
    // ... other modules
});
```
3. Regenerate project files as noted in [SETUP.md](SETUP.md)

---

## Creating Your Game Module

### 1. Create Module Structure

```
Source/YourGame/
├── YourGame.Build.cs
├── Public/
│   ├── YourGame.h
│   ├── YourGameMode.h
│   └── YourPlayerController.h
└── Private/
    ├── YourGame.cpp
    ├── YourGameMode.cpp
    └── YourPlayerController.cpp
```

### 2. Build.cs Configuration

```csharp
// Source/YourGame/YourGame.Build.cs
using UnrealBuildTool;

public class YourGame : ModuleRules
{
    public YourGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine",
            "UETPFCore",  // Framework dependency
        });
    }
}
```

### 3. Game Mode Setup

```cpp
// YourGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YourGameMode.generated.h"

UCLASS()
class YOURGAME_API AYourGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AYourGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
```

```cpp
// YourGameMode.cpp
#include "YourGameMode.h"

// Include subsystems you need
#include "Subsystems/EnvironmentSubsystem.h"
#include "Subsystems/SurfaceQuerySubsystem.h"
#include "Subsystems/TimeSubsystem.h"

AYourGameMode::AYourGameMode()
{
    // Set your player controller, pawn, etc.
}

void AYourGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // Subsystems are auto-initialized by UE
    // Verify they're available:
    UWorld* World = GetWorld();
    if (UEnvironmentSubsystem* Env = World->GetSubsystem<UEnvironmentSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("EnvironmentSubsystem ready"));
    }
}

void AYourGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Your game initialization here
}
```

---

## Defining Data Assets

### 1. Create Medium Spec (Atmosphere/Fluid)

**In Editor:**
1. Content Browser → Right-click → Miscellaneous → Data Asset
2. Choose `MediumSpec` as parent class
3. Name it `DA_YourMedium` (e.g., `DA_ToxicAtmosphere`)

**Configure Properties:**
```
Display Name: "Toxic Atmosphere"
Density: 1.5 kg/m³
Viscosity: 0.00002
Temperature: 320 K
Pressure: 120000 Pa
Speed Of Sound: 350 m/s
Drag Coefficient: 0.8
Buoyancy Enabled: true
```

**In Code:**
```cpp
// Load and register in your game mode or subsystem
UMediumSpec* ToxicAtmo = LoadObject<UMediumSpec>(nullptr, TEXT("/Game/YourGame/Specs/DA_ToxicAtmosphere"));
if (ToxicAtmo)
{
    UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
    Env->RegisterMedium(FName("ToxicAtmosphere"), ToxicAtmo);
}
```

### 2. Create Surface Spec (Material)

**In Editor:**
1. Create Data Asset → `SurfaceSpec`
2. Name it `DA_YourSurface` (e.g., `DA_AlienRock`)

**Configure Properties:**
```
Display Name: "Alien Rock"
Friction: 0.7
Compliance: 0.05
Hardness: 8.0
Thermal Conductivity: 2.5
Emissivity: 0.9
FX Profile: (reference to your FXProfile asset)
```

**In Code:**
```cpp
USurfaceSpec* AlienRock = LoadObject<USurfaceSpec>(nullptr, TEXT("/Game/YourGame/Specs/DA_AlienRock"));
if (AlienRock)
{
    USurfaceQuerySubsystem* Surface = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
    Surface->RegisterSurface(FName("AlienRock"), AlienRock);
}
```

### 3. JSON Loading Alternative

Use `SpecPackLoader` from SinglePlayerStoryTemplate:

```json
// Content/YourGame/SpecPacks/AlienWorld.json
{
    "Mediums": [
        {
            "Id": "ToxicAtmosphere",
            "DisplayName": "Toxic Atmosphere",
            "Density": 1.5,
            "Temperature": 320,
            "Pressure": 120000
        }
    ],
    "Surfaces": [
        {
            "Id": "AlienRock",
            "DisplayName": "Alien Rock",
            "Friction": 0.7,
            "Compliance": 0.05
        }
    ]
}
```

```cpp
// Load in game mode
USpecPackLoader* Loader = NewObject<USpecPackLoader>();
Loader->LoadSpecPack(TEXT("/Game/YourGame/SpecPacks/AlienWorld.json"));
```

---

## Querying Subsystems

### Environment Queries

```cpp
// Get environment at a location
UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
FEnvironmentContext Context = Env->GetEnvironmentAt(ActorLocation);

// Use the context
float Density = Context.Density;  // kg/m³
float Temperature = Context.Temperature;  // Kelvin
float Pressure = Context.Pressure;  // Pascals

// Calculate drag force
FVector DragForce = -Velocity * Velocity.Size() * Density * DragCoefficient * Area / 2.0f;
AddForce(DragForce);
```

### Surface Queries

```cpp
// Query surface at location (e.g., under a character's feet)
USurfaceQuerySubsystem* Surface = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
FSurfaceState State = Surface->QuerySurface(ActorLocation);

// Use surface properties
float Friction = State.Spec->Friction;
float Temperature = State.TemperatureK;
float SnowDepth = State.SnowDepthCm;

// Apply to movement
MovementComponent->GroundFriction = Friction;
```

### Time Queries

```cpp
UTimeSubsystem* Time = GetWorld()->GetSubsystem<UTimeSubsystem>();

// Get simulation time
double SimTime = Time->GetSimulationTime();

// Set time dilation (slow motion, fast forward)
Time->SetTimeDilation(0.5f);  // Half speed

// Get day/night cycle info (if configured)
float TimeOfDay = Time->GetTimeOfDay();  // 0-24 hours
```

### World Frame Queries

```cpp
UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();

// Get gravity at location
FVector GravityDir = Frame->GetGravityDirection(ActorLocation);
float GravityMagnitude = Frame->GetGravityMagnitude(ActorLocation);
FVector GravityAccel = GravityDir * GravityMagnitude;

// Apply gravity
AddForce(GravityAccel * Mass);

// Get altitude
double AltitudeKm = Frame->GetAltitude(ActorLocation);
```

---

## Implementing Persistence

### Using FileDeltaStore (Single-Player)

```cpp
// In your game mode or save system

#include "FileDeltaStore.h"

// Create delta store
UFileDeltaStore* DeltaStore = NewObject<UFileDeltaStore>();
DeltaStore->Initialize(TEXT("SaveSlot1"));

// Save a surface delta
FSurfaceDelta Delta;
Delta.Position = FIntVector(128, 256, 0);  // Cell coordinates
Delta.SnowDepthCm = 5.0f;
Delta.Wetness = 0.3f;

DeltaStore->StoreSurfaceDelta(Delta);

// Load deltas for a cell
TArray<FSurfaceDelta> Deltas = DeltaStore->LoadSurfaceDeltas(FIntVector(128, 256, 0));
```

### Custom Persistence Implementation

Implement `IDeltaStore` interface for your storage backend:

```cpp
// YourDeltaStore.h
#pragma once

#include "CoreMinimal.h"
#include "DeltaTypes.h"
#include "YourDeltaStore.generated.h"

UCLASS()
class YOURGAME_API UYourDeltaStore : public UObject
{
    GENERATED_BODY()

public:
    // Store deltas (to database, cloud, etc.)
    void StoreSurfaceDelta(const FSurfaceDelta& Delta);
    
    // Load deltas
    TArray<FSurfaceDelta> LoadSurfaceDeltas(const FIntVector& CellCoord);
    
    // Implement your storage logic here
};
```

---

## Integrating with Physics

### Applying Environmental Forces

```cpp
// In your actor or component tick

UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
FEnvironmentContext Context = Env->GetEnvironmentAt(GetActorLocation());

// Calculate drag
FVector DragForce = -GetVelocity() * GetVelocity().Size() * Context.Density * 0.5f;

// Calculate buoyancy
float Volume = 1.0f;  // m³ (compute from actor bounds)
FVector BuoyancyForce = FVector(0, 0, Context.Density * Volume * 9.8f);

// Apply forces
if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(RootComponent))
{
    Prim->AddForce(DragForce);
    Prim->AddForce(BuoyancyForce);
}
```

### Surface Interaction

```cpp
// On collision or ground contact

USurfaceQuerySubsystem* Surface = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
FSurfaceState State = Surface->QuerySurface(HitLocation);

// Apply friction
float Friction = State.Spec->Friction;
MovementComponent->SetGroundFriction(Friction);

// Spawn collision FX
if (State.Spec->FXProfile)
{
    // Play sound from FXProfile
    // Spawn particles from FXProfile
}

// Generate surface delta (footprint, deformation)
FSurfaceDelta Delta;
Delta.Position = WorldPartitionCellCoord;
Delta.DeformationDepthCm = 0.5f;
DeltaStore->StoreSurfaceDelta(Delta);
```

### Physics Material Updates

```cpp
// Update physical material based on surface state

UPhysicalMaterial* PhysMat = DynamicMaterialInstance->GetPhysicalMaterial();
if (PhysMat)
{
    PhysMat->Friction = State.Spec->Friction;
    PhysMat->Restitution = 1.0f - State.Spec->Compliance;
}
```

---

## Best Practices

### 1. Subsystem Access Pattern

**DO:**
```cpp
UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
if (Env)
{
    // Use subsystem
}
```

**DON'T:**
```cpp
// Store subsystem pointer long-term (can become invalid)
UPROPERTY()
UEnvironmentSubsystem* CachedEnv;  // Bad!
```

### 2. Data Asset Loading

**DO:**
```cpp
// Load once in BeginPlay or InitGame
UMediumSpec* Spec = LoadObject<UMediumSpec>(nullptr, TEXT("/Game/Specs/DA_Air"));
RegisterSpec(Spec);
```

**DON'T:**
```cpp
// Load every frame
void Tick(float DeltaTime)
{
    UMediumSpec* Spec = LoadObject<UMediumSpec>(...);  // Bad!
}
```

### 3. Delta Generation

**DO:**
```cpp
// Generate deltas sparingly (on significant changes)
if (SnowDepthChanged > Threshold)
{
    FSurfaceDelta Delta;
    Delta.SnowDepthCm = NewSnowDepth;
    StoreIn(Delta);
}
```

**DON'T:**
```cpp
// Generate delta every frame
void Tick(float DeltaTime)
{
    FSurfaceDelta Delta;  // Bad!
    StoreDelta(Delta);
}
```

### 4. Coordinate Conversions

**DO:**
```cpp
// Use WorldFrameSubsystem for conversions
UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();
FVector WorldPos = Frame->CanonicalToWorld(KmPosition);
```

**DON'T:**
```cpp
// Manual conversion without frame awareness
FVector WorldPos = KmPosition * 100000.0f;  // Missing frame offset!
```

### 5. Spec Registration

**DO:**
```cpp
// Register specs once at game start
void AYourGameMode::InitGame(...)
{
    LoadAndRegisterSpecs();
}
```

**DON'T:**
```cpp
// Register specs during gameplay
void SomeActor::Tick(float DeltaTime)
{
    RegisterSpec(...);  // Bad!
}
```

---

## Common Patterns

### Pattern: Environmental Zone

```cpp
// Create a volume that defines an environmental zone

UCLASS()
class AEnvironmentalZone : public AVolume
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    UMediumSpec* MediumSpec;

    virtual void ActorBeginOverlap(AActor* Other) override
    {
        // Apply medium to actor
        if (UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>())
        {
            // Env->SetMediumForActor(Other, MediumSpec);
        }
    }
};
```

### Pattern: Surface Deformation

```cpp
// Apply surface change on impact

void OnProjectileHit(const FHitResult& Hit)
{
    USurfaceQuerySubsystem* Surface = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
    FSurfaceState State = Surface->QuerySurface(Hit.Location);

    // Create crater delta
    FSurfaceDelta Delta;
    Delta.Position = WorldPartitionCellCoordFromLocation(Hit.Location);
    Delta.DeformationDepthCm = 10.0f * ImpactForce;

    DeltaStore->StoreSurfaceDelta(Delta);

    // Update visual (RVT, decal, etc.)
    UpdateTerrainVisual(Hit.Location, Delta);
}
```

### Pattern: Weather System

```cpp
// Drive weather changes through environment

UCLASS()
class AWeatherSystem : public AActor
{
    GENERATED_BODY()

public:
    void Tick(float DeltaTime) override
    {
        // Update environmental conditions
        UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
        
        // Change temperature over time
        CurrentTemperature += TemperatureChangeRate * DeltaTime;
        
        // Apply to regions
        // Env->SetRegionTemperature(Region, CurrentTemperature);
    }
};
```

---

## Troubleshooting

### Subsystem Not Found
- Ensure module dependency in Build.cs
- Check subsystem scope (World vs GameInstance)
- Verify world is valid before querying

### Spec Not Loading
- Check asset path is correct
- Verify Data Asset parent class
- Ensure spec is registered before query

### Delta Not Persisting
- Verify delta store is initialized
- Check cell coordinates are correct
- Ensure save directory has write permissions

---

## Next Steps

1. **Create your spec assets** for your game's materials/environments
2. **Set up your game mode** to register specs on init
3. **Query subsystems** in your gameplay code
4. **Implement persistence** using FileDeltaStore or custom store
5. **Test in PIE** (Play In Editor)

For architecture details, see [ARCHITECTURE.md](ARCHITECTURE.md).

For contribution guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

**I hope this helps enhance your projects and Happy building!** 🚀
