# UETPFCoreSim - Scientific Simulation Extensions

> **Optional Module for Validated Ephemerides and Deterministic Physics**

## Overview

UETPFCoreSim extends the UETPFCore framework with components designed for **educational simulations, aerospace training, and robotics visualization** where validated celestial mechanics and deterministic behavior are required.

**This module is NOT intended for:**
- Publishable scientific research
- Production aerospace/spaceflight operations  
- Safety-critical simulations

**This module IS useful for:**
- Aerospace training simulations with real ephemerides
- Educational content (planetarium applications, orbital mechanics demos)
- Robotics visualization with repeatable physics
- Hybrid workflows bridging MATLAB/Isaac Sim → Unreal Engine
- Content creation requiring scientifically-plausible celestial positions

## Features

### ✅ Implemented

#### SPICE Ephemerides Integration
- **USpiceEphemeridesSubsystem**: Thread-safe wrapper around NASA's SPICE Toolkit
- Load SPK (ephemerides), PCK (orientation), LSK (leap seconds) kernels
- Query celestial body positions and velocities with validated data
- Automatic unit conversion (SPICE km → UE cm)
- Support for aberration corrections (light time, stellar aberration)
- Time format conversions (Unix time ↔ Julian dates ↔ Ephemeris Time)

### 🚧 To Be Implemented

The following components are **planned but not yet implemented**. Contributions welcome!

#### 1. Deterministic Physics Component
**Goal**: Fixed-timestep physics simulation for reproducible results

**What's Needed:**
- `UDeterministicPhysicsComponent` actor component
- Fixed substep accumulator (target: 240Hz physics rate)
- State snapshot/restore for rollback
- Deterministic floating-point mode configuration
- Integration with Chaos physics solver settings

**Use Cases:**  
- Record/replay of simulations
- Validation against external simulation tools
- Training data generation for ML pipelines

**References:**
- Unreal Engine Physics Settings (Project Settings → Physics)
- Chaos physics documentation

#### 2. USD Import/Export for Omniverse Pipeline
**Goal**: Bridge between NVIDIA Omniverse (Isaac Sim) and Unreal Engine

**What's Needed:**
- Import USD scenes as UE actors
- Export UE scenes to USD format
- Coordinate system conversion (UE Z-up → USD Y-up)
- Material translation (USD Preview Surface ↔ UE materials)
- Integration with NVIDIA's Isaac Sim for physics validation

**Use Cases:**
- Import robot models from Isaac Sim
- Export UE environments to Omniverse for rendering
- Hybrid simulation workflows

**References:**
- NVIDIA USD for Unreal Engine plugin
- Pixar USD documentation
- Isaac Sim connector examples

#### 3. Example Content and Blueprints
**What's Needed:**
- Example map showing solar system with real ephemerides
- Blueprint function library for common queries
- Sample SPICE kernel set (DE440 ephemerides)
- Tutorial content demonstrating integration

#### 4. Validation and Testing
**What's Needed:**
- Unit tests for SPICE wrapper
- Validation against JPL HORIZONS data
- Performance benchmarks
- Documentation of accuracy limitations

## Installation

### 1. Install SPICE Toolkit

Download CSPICE library from NASA: https://naif.jpl.nasa.gov/naif/toolkit_C.html

**Windows:**
```
Extract to: Source/UETPFCoreSim/ThirdParty/cspice/
Required files:
  - include/Win64/SpiceUsr.h
  - lib/Win64/cspice.lib
```

**Linux:**
```
Extract to: Source/UETPFCoreSim/ThirdParty/cspice/
Required files:
  - include/Linux/SpiceUsr.h
  - lib/Linux/libcspice.a
```

**macOS:**
```
Extract to: Source/UETPFCoreSim/ThirdParty/cspice/
Required files:
  - include/Mac/SpiceUsr.h
  - lib/Mac/libcspice.a
```

### 2. Download SPICE Kernels

Kernels provide actual ephemerides data. Download from:  
https://naif.jpl.nasa.gov/pub/naif/generic_kernels/

**Minimum Required:**
- **LSK (Leap Seconds):** `naif0012.tls`
- **SPK (Ephemerides):** `de440.bsp` (planets) or `de440s.bsp` (small file version)
- **PCK (Physical Constants):** `pck00011.tpc`

Place kernels in: `Content/SPICE/Kernels/`

### 3. Enable Module

Add to your `.uproject`:
```json
{
  "Name": "UETPFCoreSim",
  "Type": "Runtime",
  "LoadingPhase": "Default"
}
```

### 4. Load Kernels at Runtime

**C++ Example:**
```cpp
#include "Subsystems/SpiceEphemeridesSubsystem.h"

void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    
    auto* SpiceSubsystem = GetGameInstance()->GetSubsystem<USpiceEphemeridesSubsystem>();
    if (SpiceSubsystem && SpiceSubsystem->IsSpiceAvailable())
    {
        SpiceSubsystem->LoadKernel(TEXT("SPICE/Kernels/naif0012.tls"));
        SpiceSubsystem->LoadKernel(TEXT("SPICE/Kernels/de440s.bsp"));
        SpiceSubsystem->LoadKernel(TEXT("SPICE/Kernels/pck00011.tpc"));
    }
}
```

**Blueprint:**  
Use `Get Game Instance` → `Get Subsystem` (USpiceEphemeridesSubsystem) → `Load Kernel`

## Usage Examples

### Query Planet Position

```cpp
#include "Subsystems/SpiceEphemeridesSubsystem.h"

// Get Mars position relative to Earth at specific time
auto* SpiceSubsystem = GetGameInstance()->GetSubsystem<USpiceEphemeridesSubsystem>();

FEphemerisTime ET = FEphemerisTime::FromUnixTime(FDateTime::Now().ToUnixTimestamp());
FVector MarsPos = SpiceSubsystem->GetBodyPosition(TEXT("MARS"), TEXT("EARTH"), ET, TEXT("J2000"));

// Position in kilometers (SPICE units)
UE_LOG(LogTemp, Log, TEXT("Mars distance from Earth: %.2f million km"), MarsPos.Size() / 1000000.0);
```

### Get Sun Direction for Lighting

```cpp
// Use for dynamic directional light rotation
FEphemerisTime ET = FEphemerisTime::FromGameTime(GetWorld()->GetTimeSeconds());
FVector SunDir = SpiceSubsystem->GetSunDirection(TEXT("EARTH"), ET);

if (DirectionalLight)
{
    FRotator SunRotation = SunDir.Rotation();
    DirectionalLight->SetActorRotation(SunRotation);
}
```

### Query Full State Vector

```cpp
// Get position + velocity for orbital mechanics
FCelestialStateVector State = SpiceSubsystem->GetBodyState(
    TEXT("MOON"),           // Target
    TEXT("EARTH"),          // Observer
    ET,                     // Time
    TEXT("J2000"),          // Reference frame
    TEXT("LT+S")            // Light time + stellar aberration correction
);

if (State.bIsValid)
{
    UE_LOG(LogTemp, Log, TEXT("Moon position: %s km"), *State.PositionKm.ToString());
    UE_LOG(LogTemp, Log, TEXT("Moon velocity: %s km/s"), *State.VelocityKmS.ToString());
    UE_LOG(LogTemp, Log, TEXT("Light time: %.3f seconds"), State.LightTimeSeconds);
}
```

## Coordinate Frames

SPICE uses standard astronomical coordinate systems:

- **J2000**: Inertial frame (Earth mean equator and equinox at J2000 epoch)
- **IAU_EARTH**: Earth body-fixed frame (rotates with Earth)
- **ECLIPJ2000**: Ecliptic coordinates

Unreal Engine uses Z-up left-handed coordinates. You'll need to transform:

```cpp
// SPICE J2000 → UE world coordinates (example conversion)
FVector SpiceToUE(const FVector& SpicePos)
{
    // SPICE: X=vernal equinox, Y=90° east, Z=north pole
    // UE: X=forward, Y=right, Z=up
    return FVector(SpicePos.X, -SpicePos.Y, SpicePos.Z);
}
```

**Note:** Proper coordinate transformations depend on your world setup. The above is illustrative only.

## Common Body Names

SPICE uses NAIF ID codes and names:

| Body | SPICE Name | NAIF ID |
|------|------------|---------|
| Sun | `SUN` | 10 |
| Mercury | `MERCURY` | 199 |
| Venus | `VENUS` | 299 |
| Earth | `EARTH` | 399 |
| Moon | `MOON` | 301 |
| Mars | `MARS` | 499 |
| Jupiter | `JUPITER` | 599 |
| Saturn | `SATURN` | 699 |

Full list: https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/naif_ids.html

## Performance Considerations

- **Kernel Loading:** Do this at startup, not per-frame (file I/O)
- **Query Frequency:** SPICE queries are fast (~microseconds) but call only when needed
- **Threading:** USpiceEphemeridesSubsystem is thread-safe, but batch queries when possible
- **Kernel Size:** DE440 full ephemerides = ~3GB. Use DE440s (small, ~100MB) for planets only

## Accuracy and Limitations

### What SPICE Provides
- **Positional Accuracy:** ~1 km for planets (DE440 ephemerides)
- **Time Range:** 1550 AD to 2650 AD (DE440)
- **Validated Data:** Used by NASA/JPL mission planning

### What This Module Does NOT Provide
- **N-body Simulation**: SPICE gives pre-computed trajectories, not dynamic propagation
- **Atmospheric Models**: No atmospheric drag, solar radiation pressure, etc.
- **Spacecraft Dynamics**: No attitude dynamics, propulsion, or disturbance modeling
- **Gravitational Perturbations**: Ephemerides are pre-computed; no live perturbation calculations

**For Real Mission Planning:** Use GMAT, STK, or FreeFlyer  
**For This Module:** Visual plausibility and educational accuracy

## Contributing

We welcome contributions! Priority areas:

1. **Deterministic Physics Component** - See section above
2. **USD Import/Export** - Omniverse integration
3. **Example Content** - Tutorial maps and blueprints
4. **Testing** - Validation against HORIZONS data
5. **Documentation** - More usage examples

### Contribution Guidelines

- Follow UE coding standards
- Include unit tests where applicable
- Document public APIs with Doxygen comments
- Keep scientific simulation code separate from game-specific code
- Be clear about accuracy limitations in documentation

### How to Contribute

1. Fork the repository
2. Create feature branch: `git checkout -b feature/deterministic-physics`
3. Commit changes: `git commit -am 'Add deterministic physics component'`
4. Push branch: `git push origin feature/deterministic-physics`
5. Open pull request with description of changes

## License

Same as UETPFCore project: Apache License 2.0

SPICE Toolkit is provided by NASA/JPL and is not included in this repository.  
See: https://naif.jpl.nasa.gov/naif/rules.html

## Support

- **Issues:** Use GitHub issue tracker
- **Discussions:** Use GitHub discussions for questions
- **Documentation:** See SPICE documentation at https://naif.jpl.nasa.gov/naif/documentation.html

## Credits

- **SPICE Toolkit:** NASA Navigation and Ancillary Information Facility (NAIF)
- **UETPFCore:** Threaded Pixel Factory
- **Contributors:** See CONTRIBUTORS.md

---

**Remember:** This module bridges game engine workflows with scientific data, but it's not a replacement for validated simulation tools in mission-critical applications.
