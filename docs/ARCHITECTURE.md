<!--
Copyright Threaded Pixel Factory. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# System Architecture - UETPFCore

## Overview

UETPFCore implements a **subsystem-based architecture** for physics-driven world simulation in Unreal Engine 5.8. The design emphasizes engineering best practices in modularity, data-driven configuration, and clean separation of concerns.

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

- Holds the world's anchor body (Earth, Moon, spacecraft), set once per level via `SetAnchorBody`
- km ↔ cm transformations relative to that anchor
- Builds the sky context for the anchor (sun direction, atmosphere/cloud flags, moon phase)
- *(Planned)* Gravity direction and magnitude derived from the anchor body. Today gravity is a constant vector on the medium spec, served by EnvironmentSubsystem
- *(Planned)* Altitude calculations relative to the anchor body. Today altitude is world Z above sea level in GlobalAtmosphereField

**Key Concepts**:
- **Canonical Frame**: km-scale coordinates (physics truth)
- **UE World Frame**: cm-scale coordinates (rendering/gameplay), anchor body at origin

**Usage**:
```cpp
UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();
FVector WorldPosCm = Frame->CanonicalKmToWorldCm(KmPosition);
FVector CanonicalKm = Frame->WorldCmToCanonicalKm(WorldPosCm);
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

---

## Atmospheric Rendering Architecture

### UniversalSkyActor - Manager Pattern

**Design Philosophy**: UniversalSkyActor is a **data-driven manager**, not a component owner.

#### Architecture Rationale

UE5's atmospheric rendering pipeline has specific architectural requirements:

1. **Component Registration**: Each atmospheric component type (`USkyAtmosphereComponent`, `UDirectionalLightComponent`, `USkyLightComponent`) registers independently with the rendering system
2. **Singleton Queries**: The engine queries for "first of type" via `GetWorld()->GetFirstXXX()` patterns
3. **Separate Update Paths**: Each component has its own update mechanisms and render thread synchronization

**Anti-Pattern: "God Actor"** (Previous approach - DEPRECATED):
```cpp
// ❌ Owned components in one actor breaks rendering pipeline
class AUniversalSkyActor : public AActor {
    UDirectionalLightComponent* SunLight;      // Owned
    USkyAtmosphereComponent* SkyAtmosphere;     // Owned  
    USkyLightComponent* SkyLight;               // Owned
};
// Problems:
// - Breaks UE5's component registration
// - Interferes with atmospheric scattering queries
// - Prevents proper Lumen GI updates
```

**Correct Pattern: Manager with References** (Current approach):
```cpp
// ✅ References to separately-placed actors
class AUniversalSkyActor : public AActor {
    UPROPERTY(EditAnywhere)
    ADirectionalLight* SunLightActor;       // Reference (not owned)
    
    UPROPERTY(EditAnywhere)
    ASkyAtmosphere* SkyAtmosphereActor;     // Reference (not owned)
    
    UPROPERTY(EditAnywhere)
    ASkyLight* SkyLightActor;               // Reference (not owned)
};
// Benefits:
// - Respects UE5 rendering architecture
// - Each actor registers independently
// - Proper Lumen GI integration
// - Manager only updates properties
```

#### Data Flow Architecture

```
┌───────────────────────────────────────────────────────────┐
│                    SUBSYSTEM LAYER                        │
│  ┌───────────────┐  ┌────────────────┐  ┌───────────────┐ │
│  │ SolarSystem   │  │  Environment   │  │     Time      │ │
│  │  Subsystem    │  │   Subsystem    │  │  Subsystem    │ │
│  └───────┬───────┘  └────────┬───────┘  └───────┬───────┘ │
│          │ Sun Direction     │ Atmosphere       │ Events  │
│          │                   │ Properties       │         │
└──────────┼───────────────────┼──────────────────┼─────────┘
           │                   │                  │
           │                   │                  │ OnTimeAdvanced
           │                   │                  ▼
┌──────────▼───────────────────▼───────────────────────────────┐
│              UNIVERSALSKYACTOR (MANAGER)                     │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  Tick() / Event-Driven Updates:                         │ │
│  │  1. Query SolarSystemSubsystem → sun direction          │ │
│  │  2. Query EnvironmentSubsystem → atmosphere properties  │ │
│  │  3. Process → render parameters (intensity, color)      │ │
│  │  4. Call component setters on referenced actors         │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                              │
│  Component Getters (safe reference access):                  │
│  - GetSunLightComponent() → DirectionalLightComponent        │
│  - GetSkyAtmosphereComponent() → SkyAtmosphereComponent      │
│  - GetSkyLightComponent() → SkyLightComponent                │
└───────┬──────┬──────────┬──────────┬──────────┬──────────────┘
        │      │          │          │          │
        │  Setters        │          │          │
        │  SetIntensity() │          │          │
        │  SetRotation()  │          │          │
        ▼      ▼          ▼          ▼          ▼
┌────────────────────────────────────────────────────────────┐
│           SEPARATELY PLACED ACTORS (LEVEL)                 │
│  ┌────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │☀️Sun   │ │ 🌍Atmos  │  │🌤️Sky     │ │ ☁️Clouds │      │
│  │Light   │  │phere     │  │Light     │  │          │      │
│  │Actor   │  │Actor     │  │Actor     │  │Actor     │      │
│  └────────┘  └──────────┘  └──────────┘  └──────────┘      │
│  (Component) (Component)  (Component)   (Component)        │
│  registers → UE5 Rendering System                          │
└────────────────────────────────────────────────────────────┘
```

#### Update Mechanisms

**Event-Driven (Primary)**:
```cpp
// TimeSubsystem broadcasts time changes
TimeSubsystem->OnSimTimeAdvanced.Broadcast(NewTime);
    ↓
UniversalSkyActor::OnTimeAdvanced(double Time)
    ↓
ApplyEnvironment(CurrentMedium, CurrentWeather)
    ↓
ApplySun() / ApplyAtmosphere() / ApplySkyLight()
    ↓
Component Setters (SetIntensity, SetRotation, SetScattering)
```

**Tick-Based (Starfield Only)**:
```cpp
// Only starfield rotation updates in Tick (lightweight)
UniversalSkyActor::Tick(DeltaSeconds)
    ↓
UpdateStarfieldRotation()  // GMST-based star rotation
    ↓
NiagaraComponent->SetVariableFloat("User_RotationAngle")
```

#### Component Access Pattern

Safe component getter pattern with nullptr checks:

```cpp
void AUniversalSkyActor::ApplySun(const FRuntimeMediumSpec& Medium, ...)
{
    // Get component via actor reference
    UDirectionalLightComponent* SunLight = GetSunLightComponent();
    
    // Silent return if actor not assigned (no error spam)
    if (!SunLight) { return; }
    
    // Safe to use component
    SunLight->SetIntensity(CalculatedIntensity);
    SunLight->SetWorldRotation(CalculatedRotation);
}
```

**Why Silent Failure?**
- Users may intentionally not use certain atmospheric features (e.g., no sun for underground levels)
- Reduces log spam during development
- Manager gracefully handles partial setup

#### Owned vs Referenced Components

| Component | Ownership | Reason |
|-----------|-----------|--------|
| DirectionalLight | Referenced | Must register independently for atmosphere |
| SkyAtmosphere | Referenced | Queries separate from actor hierarchy |
| SkyLight | Referenced | Capture system requires actor-level registration |
| VolumetricCloud | Referenced | Material-driven, needs separate actor |
| ExponentialHeightFog | Referenced | Global fog queries by type |
| PostProcessVolume | Referenced | Spatial blending requires volume actor |
| NiagaraStarfield | **Owned** | Purely visual, no engine queries |

#### Integration Points

**With SolarSystemSubsystem**:
```cpp
FSolarSystemState SolarState = SolarSys->GetSolarSystemState();
FVector SunDir = SolarState.SunDir_World;
float SunIlluminance = SolarState.SunIlluminanceLux;

// Apply to DirectionalLight via reference
GetSunLightComponent()->SetWorldRotation(MakeRotFromX(-SunDir));
GetSunLightComponent()->SetIntensity(SunIlluminance * CloudDim);
```

**With EnvironmentSubsystem**:
```cpp
FRuntimeMediumSpec Medium = EnvSubsystem->GetMediumSpec("Earth");

// Convert density → scattering coefficients
float Density01 = FMath::Clamp(Medium.Density / 1.225f, 0.0f, 1.0f);
float AtmosStrength = Density01 * 0.3f;

// Apply to SkyAtmosphere via reference
GetSkyAtmosphereComponent()->SetRayleighScatteringScale(AtmosStrength);
GetSkyAtmosphereComponent()->SetMieScatteringScale(AtmosStrength * Humidity01);
```

**With TimeSubsystem**:
```cpp
// Subscribe to time changes (BeginPlay)
TimeSubsystem->OnSimTimeAdvanced.AddUObject(this, &AUniversalSkyActor::OnTimeAdvanced);

// Event callback
void AUniversalSkyActor::OnTimeAdvanced(double NewTime) {
    ApplyEnvironment(CurrentMedium, CurrentWeather);
}
```

#### Usage Workflow (Level Setup)

1. **Place Environment Light Mixer Components**:
   - Place `SkyAtmosphere` actor
   - Place `DirectionalLight` actor (set Mobility: Movable)
   - Place `SkyLight` actor (set Real Time Capture: Enabled)
   - [Optional] Place `VolumetricCloud`, `ExponentialHeightFog`, `PostProcessVolume`

2. **Place UniversalSkyActor**:
   - Drag into level

3. **Assign References**:
   - Select UniversalSkyActor
   - In Details panel, under **Sky|References**, assign all placed actors

4. **Verify**:
   - Hit Play
   - Check Output Log for: `✅ UniversalSkyActor: All required actor references assigned`

#### Performance Characteristics

- **Memory**: Minimal overhead (only actor references)
- **CPU**: Updates only on environment/time changes (event-driven)
- **Starfield**: GPU-compute Niagara system (5000+ stars)
- **Subsystem Queries**: Amortized across all managers

---

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
- Large World Coordinates (LWC): `FVector` is double precision
- Origin is the anchor body's center, fixed for the lifetime of the level

### Transformation
```cpp
// Canonical (km) → World (cm)
WorldPosCm = (CanonicalKm - AnchorKm) * 100000.0;   // UWorldFrameSubsystem::CanonicalKmToWorldCm

// World (cm) → Canonical (km)
CanonicalKm = WorldPosCm / 100000.0 + AnchorKm;     // UWorldFrameSubsystem::WorldCmToCanonicalKm
```

Always call the `WorldFrameSubsystem` functions; a bare `* 100000.0` drops the anchor offset. Both stay in double precision end to end.

Never narrow a converted position to `float`. A float holds about 7 significant digits, so at the Moon's distance from an Earth anchor (3.8e10 cm) it can only represent positions about 41 m apart, and at low Earth orbit (6.8e8 cm) about 64 cm apart.

### Why Two Frames When UE Has LWC

LWC makes `FVector` double, and the engine already renders relative to the camera, so gameplay, rendering and Chaos stay precise across the whole world. LWC does have a range: `UE_LARGE_WORLD_MAX` (`EngineDefines.h`) allows about 44 million km from the origin.

| Distance from an Earth anchor | Fits in the UE world? |
|---|---|
| Moon, about 384,400 km | Yes |
| Mars at its closest, about 55 million km | No |
| Sun, about 150 million km | No |

The **canonical frame** is the source of truth. It holds every body's true position in km doubles; even at the Sun's distance a double resolves to a few centimetres. The **world frame** is a window onto it, centered on the anchor body. Bodies inside the window are placed as actors with `CanonicalKmToWorldCm`. Bodies outside it are shown as directions, not positions (for example `FSkyContext::SunDirWorld`).

Changing the anchor changes where the window is centered. Today that happens once per level, and `UInterplanetaryTravelSubsystem` loads a different level with a different anchor.

### Floating Origin: When a Project Needs It

A floating origin moves the origin to follow the camera, so the numbers near the player stay small. There are two kinds.

**Engine origin rebasing** (`UWorld::SetNewWorldOrigin`) was UE4's answer for large open worlds. Under UE5 it isn't needed for gameplay, rendering or physics, and World Partition doesn't support it. Don't use it in this framework.

**Per-solver local frames** are still useful. LWC only protects double-precision code paths. Code that does its own math in `float` loses precision far from the origin, no matter what LWC does. That includes:
- CPU solvers that store `float` positions or fields
- Niagara and other GPU simulations
- material and shader math (world position offset, procedural noise)
- anything packed into `FVector3f` for bandwidth or SIMD

For those systems, add a third tier under the two frames above:

```
Canonical (km, double)
  -> World (cm, double, LWC; anchor body at origin)
    -> Local frame (float; origin owned by the solver)
```

Sketch of the extension to `UWorldFrameSubsystem`:

```cpp
FLocalFrameHandle RegisterLocalFrame(FName Name, const FVector& OriginWorldCm);
FVector3f WorldToLocal(FLocalFrameHandle Frame, const FVector& WorldPosCm) const;
FVector   LocalToWorld(FLocalFrameHandle Frame, const FVector3f& LocalPos) const;
void      RecenterLocalFrame(FLocalFrameHandle Frame, const FVector& NewOriginWorldCm);
FOnLocalFrameShifted OnLocalFrameShifted;  // (Handle, DeltaCm)
```

Design rules:
- **Recenter on a distance threshold, not every frame.** Recentering moves the frame's origin and broadcasts `OnLocalFrameShifted`, so systems holding local positions can update.
- **Each solver is sized to its own domain.** Its cost depends on its own extent, not on how big the world is. Solvers far from the player can be paused or run at lower resolution based on distance from their frame. This keeps simulation cost bounded as the world grows.
- **Never save or send local coordinates.** Deltas are keyed in canonical km or World Partition cell coordinates. Network state is sent in canonical or world coordinates. Each machine keeps its own local frames.

A related extension is changing `AnchorBody` while the game runs, for example a seamless Earth-to-orbit-to-Moon trip without a level load. That breaks today's assumption that the anchor is set once per level. It would need an `OnAnchorChanged` event, and anything that caches converted world positions would have to convert them again.

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
