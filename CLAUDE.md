# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

UETPFCore is a multi-scale simulation framework for Unreal Engine 5.7, implementing a subsystem-based architecture for physics-driven world simulation. It targets space exploration, planetary surface games, and multi-scale simulations.

**Engine**: Unreal Engine 5.7
**Platform**: Windows (primary)
**License**: Apache 2.0

## Development Commands

### Building
```batch
# Build the project (Development Editor configuration)
Scripts\build.bat

# Build with specific configuration
Scripts\build.bat DebugGame
Scripts\build.bat Shipping
```

### Running
```batch
# Launch Unreal Editor
Scripts\editor.bat

# Launch with additional flags
Scripts\editor.bat -log -windowed
```

### Testing
```batch
# Run all tests
Scripts\test.bat

# Run specific test filter
Scripts\test.bat UETPFCore
Scripts\test.bat Surface
```

### Cleaning
```batch
# Remove build artifacts
Scripts\clean.bat
```

### Project File Regeneration
After changing module dependencies or Build.cs files:
- Right-click `UETPFCore.uproject`
- Select "Generate Visual Studio project files"

## Module Architecture

### Module Dependency Flow
```
SinglePlayerStoryTemplate → UETPFCore
GameLauncher → UETPFCore
```

### UETPFCore (Core Framework)
**Path**: `Source/UETPFCore/`
**Type**: Runtime module

The core physics framework providing:
- **Subsystems** (`Public/Subsystems/`): TimeSubsystem, EnvironmentSubsystem, SurfaceQuerySubsystem, BiomeSubsystem, PhysicsIntegrationSubsystem
- **Space** (`Public/Space/`): SolarSystemSubsystem, celestial body management
- **Environment** (`Public/Environment/`): UniversalSkyActor (atmospheric rendering manager)
- **Spec Types** (`Public/SpecTypes.h`): UMediumSpec, USurfaceSpec, UDamageSpec data assets
- **Delta Types** (`Public/DeltaTypes.h`): FSurfaceDelta, FFractureDelta, FTransformDelta

**Key Dependencies**: PhysicsCore, Chaos, DataRegistry, Niagara, NiagaraCore

### GameLauncher (Menu System)
**Path**: `Source/GameLauncher/`
**Type**: Runtime module

Generic menu system demonstrating:
- Main menu architecture
- Enhanced Input integration
- Level loading patterns

### SinglePlayerStoryTemplate (Game Template)
**Path**: `Source/SinglePlayerStoryTemplate/`
**Type**: Runtime module

Template demonstrating:
- Local file-based persistence (FileDeltaStore)
- JSON spec loading (SpecPackLoader)
- Story game mode and player controller examples

## Core Architectural Patterns

### 1. Subsystem Pattern

All major systems are UE World Subsystems with automatic lifecycle management. Access via:

```cpp
UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
```

**CRITICAL**: Never cache subsystem pointers as `UPROPERTY`. Always query per-use or in local scope.

**Available Subsystems**:
- **TimeSubsystem** (World): Game time progression, dilation, day/night cycle
- **EnvironmentSubsystem** (World): Medium specs (air, water, vacuum), density, drag, buoyancy
- **SurfaceQuerySubsystem** (World): Surface specs (rock, soil, ice), friction, compliance
- **BiomeSubsystem** (World): Environmental regions, blending multiple specs
- **PhysicsIntegrationSubsystem** (World): Bridge between specs and Chaos physics
- **SolarSystemSubsystem** (GameInstance): Celestial body management, sun/moon positioning

### 2. Data-Driven Configuration via Specs

Game behavior defined through UE Data Assets:
- **UMediumSpec**: Fluid/atmosphere properties (density, viscosity, temperature, pressure)
- **USurfaceSpec**: Contact material properties (friction, compliance, thermal)
- **UBiomeSpec**: Environmental region definitions

**Load once** in InitGame/BeginPlay, register with subsystems. **Never load in Tick**.

### 3. Multi-Scale Coordinate System

Two coordinate frames with seamless transformations:
- **Canonical Frame**: km-scale (physics truth, double precision)
- **UE World Frame**: cm-scale (rendering/gameplay)

Use `UWorldFrameSubsystem` for conversions:
```cpp
FVector WorldPos = Frame->CanonicalToWorld(KmPosition);
FVector CanonicalKm = Frame->WorldToCanonical(WorldPos);
```

**NEVER** manually convert with `* 100000.0` - this misses frame offsets.

### 4. Delta Persistence System

Sparse world state changes stored as deltas:
- Cell-based spatial partitioning (World Partition aligned)
- Only changed state persisted
- Types: FSurfaceDelta (snow, wetness, deformation), FFractureDelta, FTransformDelta

Generate deltas **sparingly** (on significant changes), not every frame.

### 5. UniversalSkyActor - Manager Pattern

**CRITICAL ARCHITECTURE**: UniversalSkyActor is a **data-driven manager**, NOT a component owner.

**Manager Pattern** (Correct):
```cpp
// References to separately-placed actors
UPROPERTY(EditAnywhere)
ADirectionalLight* SunLightActor;  // Reference (not owned)

UPROPERTY(EditAnywhere)
ASkyAtmosphere* SkyAtmosphereActor;  // Reference (not owned)
```

**Why**: UE5's atmospheric rendering requires each component type (USkyAtmosphereComponent, UDirectionalLightComponent, USkyLightComponent) to register independently with the rendering system. Creating owned components in one actor breaks the pipeline.

**Setup Workflow**:
1. Place `SkyAtmosphere`, `DirectionalLight`, `SkyLight` actors separately in level
2. Configure DirectionalLight: Mobility: Movable, Atmosphere Sun Light: Enabled
3. Configure SkyLight: Mobility: Movable, Real Time Capture: Enabled
4. Place `UniversalSkyActor` in level
5. Assign actor references in UniversalSkyActor Details panel

**Update Mechanism**: Event-driven via `TimeSubsystem::OnTimeAdvanced` delegate, not tick-based (except starfield rotation).

## Code Organization

### Public Headers Structure
```
Source/UETPFCore/Public/
├── Subsystems/          # All world/game instance subsystems
├── Environment/         # UniversalSkyActor, atmospheric systems
├── Space/               # Celestial bodies, solar system
├── Materials/           # Material-related systems
├── SpecTypes.h          # Data asset definitions (Medium, Surface, Damage)
├── DeltaTypes.h         # Persistence delta structs
└── UETPFCore.h          # Module header
```

### Private Implementation Structure
```
Source/UETPFCore/Private/
├── Subsystems/          # Subsystem implementations
├── Environment/         # Environment system implementations
├── Space/               # Space system implementations
├── Materials/           # Material implementations
├── SpecTypes.cpp        # Spec implementations
├── DeltaTypes.cpp       # Delta serialization
└── Log.h/.cpp           # Logging utilities
```

## Common Development Patterns

### Subsystem Access Pattern (DO)
```cpp
void Tick(float DeltaTime)
{
    UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
    if (Env)
    {
        FEnvironmentContext Context = Env->GetEnvironmentAt(Location);
        // Use context
    }
}
```

### Spec Registration (DO)
```cpp
void AYourGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // Load once at game start
    UMediumSpec* AirSpec = LoadObject<UMediumSpec>(nullptr, TEXT("/Game/Specs/DA_Air"));
    if (UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>())
    {
        Env->RegisterMedium(FName("Air"), AirSpec);
    }
}
```

### Coordinate Conversion (DO)
```cpp
UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();
FVector WorldPos = Frame->CanonicalToWorld(KmPosition);  // Correct
```

### Anti-Patterns (DON'T)

```cpp
// DON'T: Cache subsystem as UPROPERTY
UPROPERTY()
UEnvironmentSubsystem* CachedEnv;  // BAD - can become invalid

// DON'T: Load specs every frame
void Tick(float DeltaTime)
{
    UMediumSpec* Spec = LoadObject<UMediumSpec>(...);  // BAD
}

// DON'T: Manual coordinate conversion
FVector WorldPos = KmPosition * 100000.0f;  // BAD - missing frame offset

// DON'T: Generate deltas every frame
void Tick(float DeltaTime)
{
    FSurfaceDelta Delta;  // BAD - too frequent
    StoreDelta(Delta);
}

// DON'T: Register specs during gameplay
void SomeActor::Tick(float DeltaTime)
{
    RegisterSpec(...);  // BAD - register once in InitGame
}
```

## Enabled Plugins

Critical plugins enabled in UETPFCore.uproject:
- DataRegistry: Spec asset management
- GeometryCollectionPlugin, Dataflow: Fracture/destruction
- PCG: Procedural content generation
- Metasound: Procedural audio
- Niagara: VFX (starfield, particles)
- Water, WaterAdvanced, Buoyancy: Water simulation
- ChaosModularVehicle: Vehicle physics

## Key Files Reference

- [ARCHITECTURE.md](docs/ARCHITECTURE.md): Deep dive on subsystems, specs, atmospheric rendering architecture
- [IMPLEMENTATIONGUIDE.md](docs/IMPLEMENTATIONGUIDE.md): Integration patterns, setup workflows
- [SETUP.md](docs/SETUP.md): Build setup, troubleshooting
- [threading-guidelines.md](docs/threading-guidelines.md): UE5 threading systems reference
- [README.md](README.md): Project overview, use cases

## Performance Guidelines

- **Subsystems**: Not all need to tick. Use `ShouldCreateSubsystem()` conditionally.
- **Spec Lookups**: Cached in TMap (O(1)), register once.
- **Delta Storage**: Sparse, cell-based, incremental saving.
- **Coordinate Transforms**: Cache frame transforms when stable, batch conversions.

## Threading Model

- Subsystems run on game thread by default
- Ensure proper Task Graph models and implementations
- Chaos physics runs on physics thread scale as needed
- Use `AsyncPhysicsTickComponent` for per-actor physics callbacks

## Working with This Codebase

When making changes:
1. Respect the subsystem pattern - don't bypass it with static/singleton hacks
2. Keep specs data-driven - avoid hardcoding material properties
3. Follow the manager pattern for UE rendering systems (see UniversalSkyActor)
4. Use WorldFrameSubsystem for ALL coordinate conversions
5. Generate deltas only on significant state changes
6. Access subsystems per-use, never cache as UPROPERTY
7. After modifying Build.cs dependencies, regenerate project files
