<!--
Copyright Threaded Pixel Factory. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# System Architecture - UETPFCore

## Overview

UETPFCore implements a **subsystem-based architecture** for physics-driven world simulation in Unreal Engine 5.7. The design emphasizes engineering best practices in modularity, data-driven configuration, and clean separation of concerns.

## Core Architectural Principles

### 1. Subsystem Pattern
All major systems are implemented as UE Subsystems:
- Automatic lifecycle management
- Dependency injection via subsystem access
- Clean initialization/shutdown
- Per-world or game-instance scope

### 2. Data-Driven Configuration
Game behavior is defined through Data Assets:
- **Spec Assets**: Define material/environment properties
- **Profile Assets**: Define FX/audio mappings
- Runtime-loadable, hot-reloadable
- Designer-friendly, no code changes needed

### 3. Delta Persistence
World state changes are stored as sparse deltas:
- Only changed state is persisted
- Cell-based spatial partitioning (World Partition aligned)
- Composable delta types (surface, fracture, transform)

### 4. Multi-Scale Coordinates
Seamless handling of vast scale ranges:
- Kilometer-scale canonical coordinates
- Centimeter-scale Unreal world coordinates
- Transformation utilities for both directions

## Module Structure

```
UETPFCore (Core Framework)
    ├── Subsystems/
    ├── Environment/
    ├── Space/
    ├── Materials/
    └── Public Types (SpecTypes, DeltaTypes)

GameLauncher (Menu System)
    ├── Widgets/
    ├── Game Modes/
    └── Level Loading

SinglePlayerStoryTemplate (Game Template)
    ├── Persistence/
    ├── Loading/
    └── Game-Specific Classes
```

### Dependency Flow
```
SinglePlayerStoryTemplate → UETPFCore
GameLauncher → UETPFCore
```

## Key Subsystems

### TimeSubsystem
**Scope**: World  
**Purpose**: Game time progression and dilation

- Configurable time scale (slow motion, fast forward)
- Simulation time vs real time tracking
- Day/night cycle support
- Integrates with other subsystems for time-dependent behavior

**Usage**:
```cpp
UTimeSubsystem* Time = GetWorld()->GetSubsystem<UTimeSubsystem>();
double SimTime = Time->GetSimulationTime();
```

### WorldFrameSubsystem
**Scope**: World  
**Purpose**: Multi-scale coordinate transformation

- Manages current coordinate frame (which body we're orbiting)
- km ↔ cm transformations
- Gravity direction and magnitude
- Altitude calculations

**Key Concepts**:
- **Canonical Frame**: km-scale coordinates (physics truth)
- **UE World Frame**: cm-scale coordinates (rendering/gameplay)

**Usage**:
```cpp
UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();
FVector WorldPos = Frame->CanonicalToWorld(KmPosition);
```

### EnvironmentSubsystem
**Scope**: World  
**Purpose**: Query environmental conditions

- Medium specs (air, water, vacuum)
- Density, pressure, temperature
- Drag, buoyancy forces
- Sound propagation properties

**Data**: `UMediumSpec` Data Assets

**Usage**:
```cpp
UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
FEnvironmentContext Context = Env->GetEnvironmentAt(Location);
float Drag = Context.Density * Velocity;
```

### SurfaceQuerySubsystem
**Scope**: World  
**Purpose**: Query surface material properties

- Surface specs (rock, soil, ice, metal)
- Friction, compliance (deformation)
- Thermal properties
- FX/audio profiles

**Data**: `USurfaceSpec` Data Assets

**Usage**:
```cpp
USurfaceQuerySubsystem* Surface = GetWorld()->GetSubsystem<USurfaceQuerySubsystem>();
FSurfaceState State = Surface->QuerySurface(Location);
float Friction = State.Spec->Friction;
```

### BiomeSubsystem
**Scope**: World  
**Purpose**: Environmental region management

- Biome definitions (desert, tundra, ocean, etc.)
- Blends multiple medium/surface specs
- Supports procedural generation (PCG integration)
- Temperature, humidity, wind patterns

**Data**: `UBiomeSpec` Data Assets

### PhysicsIntegrationSubsystem
**Scope**: World  
**Purpose**: Bridge between specs and Chaos physics

- Applies environmental forces (drag, buoyancy)
- Updates physical materials based on surface state
- Handles deformation, fracture triggers
- MetaSound integration for procedural sound ie. wind or collision audio

### SolarSystemSubsystem
**Scope**: GameInstance  
**Purpose**: Celestial body management

- Sun/moon positioning
- Light direction and intensity
- Celestial body definitions (radius, atmosphere)
- Simplified orbital mechanics - can be extended

**Usage**: Drive sky rendering, time of day, tides

## Data Asset Types

### SpecTypes (UETPFCore/Public/SpecTypes.h)

#### UMediumSpec
Defines fluid/atmosphere properties:
```cpp
- DisplayName
- Density (kg/m³)
- Viscosity
- Temperature (K)
- Pressure (Pa)
- SpeedOfSound (m/s)
- DragCoefficient
- BuoyancyEnabled
```

#### USurfaceSpec
Defines contact material properties:
```cpp
- DisplayName
- Friction (static, dynamic)
- Compliance (deformation response)
- Hardness
- ThermalConductivity
- Emissivity
- FXProfile (collision sounds, particles)
```

#### UDamageSpec
Defines fracture/destruction behavior:
```cpp
- YieldStrength
- UltimateTensileStrength
- Toughness
- FracturePattern
- DebrisGeneration
```

### DeltaTypes (UETPFCore/Public/DeltaTypes.h)

#### FSurfaceDelta
Sparse surface state changes:
```cpp
- Position (cell coordinate)
- SnowDepth
- Wetness
- Temperature
- DeformationDepth
```

#### FFractureDelta
Destruction state:
```cpp
- ActorGUID
- FracturedPieces
- Timestamp
- Force
```

#### FTransformDelta
Dynamic object positions:
```cpp
- ActorGUID
- Transform
- Velocity
- AngularVelocity
```

## Data Flow

### Initialization Flow
```
1. Engine starts → GameInstance created
2. GameInstance subsystems initialize (SolarSystemSubsystem)
3. World loaded → World subsystems initialize
   - TimeSubsystem
   - WorldFrameSubsystem
   - EnvironmentSubsystem
   - SurfaceQuerySubsystem
   - BiomeSubsystem
   - PhysicsIntegrationSubsystem
4. Specs loaded from Data Assets or JSON (SpecPackLoader)
5. Subsystems register specs
6. Ready for gameplay
```

### Runtime Query Flow
```
Gameplay Code
    ↓
Query Subsystem (e.g., EnvironmentSubsystem)
    ↓
Subsystem reads registered specs
    ↓
Returns computed context (FEnvironmentContext)
    ↓
Gameplay applies physics forces
```

### Persistence Flow
```
World State Changes
    ↓
Generate Delta (FSurfaceDelta, FFractureDelta, etc.)
    ↓
IDeltaStore interface
    ↓
Implementation (FileDeltaStore, NetworkDeltaStore, etc.)
    ↓
Storage (JSON files, database, network packets)
```

## Coordinate Systems

### Canonical Frame (km-scale)
- Used for large-scale positioning
- Celestial body coordinates
- Orbital mechanics
- Double-precision FVector

### UE World Frame (cm-scale)
- Standard Unreal coordinates
- Rendering, gameplay, physics
- Large World Coordinates (LWC) enabled
- Single-precision at local scale

### Transformation
```cpp
// Canonical (km) → World (cm)
FVector WorldPos = CanonicalKm * 100000.0;  // km to cm

// World (cm) → Canonical (km)
FVector CanonicalKm = WorldPos / 100000.0;  // cm to km
```

Use `WorldFrameSubsystem` for proper transformations accounting for frame offsets.

## Threading Model

### Subsystems
- All subsystems run on game thread by default
- Heavy computations should use async tasks. Extensible to Task System.

### Physics Integration
- Chaos physics runs on physics thread
- PhysicsIntegrationSubsystem bridges game ↔ physics thread
- Use `AsyncPhysicsTickComponent` for per-actor physics callbacks

### Recommendations
- Keep subsystem ticks lightweight
- Offload spatial queries to async tasks
- Use task graph for parallelizable work (biome blending, etc.)

## World Partition Integration

### Cell-Based Deltas
Deltas are stored per World Partition cell:
```
SaveData/
    └── CellDelta_X128_Y256.json  (surface deltas for cell)
    └── CellDelta_X129_Y256.json
    ...
```

### Streaming
- As cells stream in → load deltas
- As cells stream out → save dirty deltas
- Subsystems apply deltas to runtime state

## Extension Points

### Adding a New Spec Type
1. Create `UYourSpec : public UPrimaryDataAsset`
2. Add properties for your domain
3. Create subsystem to manage/query specs
4. Register specs in subsystem's Initialize()

### Adding a New Delta Type
1. Define `FYourDelta` struct in DeltaTypes.h
2. Implement serialization (JSON or binary)
3. Add to `IDeltaStore` interface
4. Implement storage in concrete store classes

### Adding a New Subsystem
1. Inherit from `UWorldSubsystem` or `UGameInstanceSubsystem`
2. Override `Initialize()`, `Deinitialize()`
3. Expose query/command functions
4. Register in module startup if needed

## Performance Considerations

### Subsystem Tick
- Not all subsystems need to tick
- Use `ShouldCreateSubsystem()` to conditionally create
- Tick only when necessary (e.g., TimeSubsystem needs tick)

### Spec Lookups
- Specs are cached in TMap by ID
- O(1) lookup performance
- Avoid per-frame spec registration

### Delta Storage
- Sparse storage (only changed state)
- Spatial partitioning (cell-based)
- Incremental saving (dirty cell tracking)

### Multi-Scale Transforms
- Cache frame transforms when stable
- Batch coordinate conversions
- Use double precision only where needed

## Integration with UE Systems

### Chaos Physics
- Physical materials driven by SurfaceSpec
- Apply forces via `UPrimitiveComponent::AddForce()`
- Use `AsyncPhysicsTickCallback` for per-tick updates

### Niagara VFX
- Feed environment context to Niagara parameters
- Drive particle behavior by medium density/temperature
- Spawn emitters based on FXProfile

### MetaSounds
- Feed surface/medium properties to audio parameters
- Dynamic sound propagation based on medium
- Material-specific collision sounds from FXProfile

### PCG (Procedural Content Generation)
- Use BiomeSubsystem to drive PCG attributes
- Generate surface detail based on specs
- Procedural placement respects biome regions

### RVT (Runtime Virtual Texturing)
- Store biome masks in RVT
- Sample RVT to determine surface type
- Update RVT with surface deltas (snow, wetness)

## Testing Architecture - Implemented by users based on usecase

### Unit Tests
- Test coordinate transformations
- Test spec registration/lookup
- Test delta serialization

### Integration Tests
- Test subsystem initialization order
- Test spec → physics pipeline
- Test delta persistence round-trip

### Gameplay Tests
- Spawn actors in various biomes
- Verify correct environmental forces
- Validate surface interaction feedback

## Summary

UETPFCore provides a layered architecture:
- **Foundation**: Subsystems, specs, deltas
- **Integration**: Physics, VFX, audio pipelines
- **Gameplay**: Query APIs, persistence

This separation allows:
- **Core framework** (UETPFCore) to remain game-agnostic
- **Game modules** (SinglePlayerStoryTemplate) to implement specific gameplay
- **Shared systems** (GameLauncher) to be reused across games

---

Continue to [IMPLEMENTATIONGUIDE.md](IMPLEMENTATIONGUIDE.md) for practical integration steps.
