# UETPFCoreSim Quick Reference

Quick reference for using the SPICE Ephemerides subsystem.

## Getting the Subsystem

```cpp
#include "Subsystems/SpiceEphemeridesSubsystem.h"

auto* Spice = GetGameInstance()->GetSubsystem<USpiceEphemeridesSubsystem>();
```

## Check Availability

```cpp
if (Spice && Spice->IsSpiceAvailable())
{
    // SPICE library present, queries will work
}
else
{
    // Compiled without SPICE or library missing
}
```

## Load Kernels (Once at Startup)

```cpp
// Leap seconds (required)
Spice->LoadKernel(TEXT("SPICE/Kernels/naif0012.tls"));

// Planetary ephemerides
Spice->LoadKernel(TEXT("SPICE/Kernels/de440s.bsp"));

// Physical constants
Spice->LoadKernel(TEXT("SPICE/Kernels/pck00011.tpc"));
```

## Time Conversions

```cpp
// From Unix timestamp
double UnixTime = FDateTime::Now().ToUnixTimestamp();
FEphemerisTime ET = FEphemerisTime::FromUnixTime(UnixTime);

// From Julian Date
FEphemerisTime ET = FEphemerisTime::FromJulianDate(2451545.0); // J2000 epoch

// To Unix timestamp
double Unix = ET.ToUnixTime();

// To Julian Date
double JD = ET.ToJulianDate();
```

## Query Position Only

```cpp
FEphemerisTime ET = FEphemerisTime::FromUnixTime(FDateTime::Now().ToUnixTimestamp());
FVector PosKm = Spice->GetBodyPosition(TEXT("MARS"), TEXT("EARTH"), ET);

// Returns position in kilometers (SPICE units)
double DistanceKm = PosKm.Size();
```

## Query Position + Velocity

```cpp
FCelestialStateVector State = Spice->GetBodyState(
    TEXT("MOON"),           // Target
    TEXT("EARTH"),          // Observer
    ET,                     // Time
    TEXT("J2000"),          // Reference frame
    TEXT("LT+S")            // Aberration correction
);

if (State.bIsValid)
{
    FVector PosKm = State.PositionKm;
    FVector VelKmS = State.VelocityKmS;
    double LightTime = State.LightTimeSeconds;
}
```

## Get Sun Direction (for Lighting)

```cpp
FVector SunDir = Spice->GetSunDirection(TEXT("EARTH"), ET);

// Use for directional light rotation
if (DirectionalLight)
{
    DirectionalLight->SetActorRotation(SunDir.Rotation());
}
```

## Common Body Names

| Body | SPICE Name |
|------|------------|
| Sun | `SUN` |
| Mercury | `MERCURY` |
| Venus | `VENUS` |
| Earth | `EARTH` |
| Moon | `MOON` |
| Mars | `MARS` |
| Jupiter | `JUPITER` |
| Saturn | `SATURN` |
| Uranus | `URANUS` |
| Neptune | `NEPTUNE` |
| Pluto | `PLUTO` |

## Common Reference Frames

| Frame | Description |
|-------|-------------|
| `J2000` | Inertial (Earth mean equator/equinox at J2000) |
| `IAU_EARTH` | Earth body-fixed (rotates with Earth) |
| `IAU_MARS` | Mars body-fixed |
| `ECLIPJ2000` | Ecliptic coordinates |

## Aberration Corrections

| Code | Description |
|------|-------------|
| `NONE` | No corrections (geometric positions) |
| `LT` | Light time correction only |
| `LT+S` | Light time + stellar aberration |
| `CN` | Converged Newtonian light time |
| `CN+S` | Converged Newtonian + stellar aberration |

**Recommendation:** Use `LT+S` for realistic visual results.

## Unit Conversions

```cpp
// SPICE uses kilometers, Unreal uses centimeters

// SPICE km → UE cm
FVector PositionCm = PositionKm * 100000.0f;

// UE cm → SPICE km
FVector PositionKm = PositionCm / 100000.0f;
```

## Coordinate System Conversion

SPICE uses astronomical conventions (X=vernal equinox, Y=90° east, Z=north pole).
Unreal uses Z-up left-handed coordinates.

```cpp
// Example conversion (adjust for your world orientation)
FVector SpiceToUE(const FVector& SpiceJ2000)
{
    // This is illustrative - your conversion depends on world setup
    return FVector(SpiceJ2000.X, -SpiceJ2000.Y, SpiceJ2000.Z);
}
```

## Error Handling

```cpp
FCelestialStateVector State = Spice->GetBodyState(...);

if (!State.bIsValid)
{
    FString Error = Spice->GetLastError();
    UE_LOG(LogTemp, Warning, TEXT("SPICE query failed: %s"), *Error);
}
```

## Performance Tips

- **Load kernels once** at startup (not per-frame)
- **Cache results** if querying same time repeatedly
- **Batch queries** when possible (subsystem is thread-safe)
- **Use light queries** (`GetBodyPosition`) if velocity not needed

## Blueprint Usage

All functions are Blueprint-callable:

1. Get Game Instance → Get Subsystem (USpiceEphemeridesSubsystem)
2. Is Spice Available → Branch
3. Load Kernel → (path to kernel file)
4. Get Body Position → (target, observer, ephemeris time)

Time conversion nodes:
- **From Unix Time** → FEphemerisTime
- **From Julian Date** → FEphemerisTime
- **To Unix Time** / **To Julian Date**

## Example: Real-Time Mars Position

```cpp
void AMyActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!SpiceSubsystem || !SpiceSubsystem->IsSpiceAvailable())
        return;
    
    // Update once per second
    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate < 1.0f)
        return;
    TimeSinceLastUpdate = 0.0f;
    
    // Get current time
    FEphemerisTime ET = FEphemerisTime::FromUnixTime(FDateTime::Now().ToUnixTimestamp());
    
    // Query Mars position
    FVector MarsPosKm = SpiceSubsystem->GetBodyPosition(TEXT("MARS"), TEXT("EARTH"), ET);
    
    // Scale for visualization (1 km = 1 cm in world)
    FVector WorldPos = MarsPosKm * 100000.0f; // km to cm
    
    // Apply to actor
    SetActorLocation(WorldPos);
}
```

## Example: Solar System at Specific Date

```cpp
void ASpaceSimulator::SetDateJulian(double JulianDate)
{
    FEphemerisTime ET = FEphemerisTime::FromJulianDate(JulianDate);
    
    TArray<FString> Bodies = {TEXT("MERCURY"), TEXT("VENUS"), TEXT("MARS"), 
                               TEXT("JUPITER"), TEXT("SATURN")};
    
    for (const FString& Body : Bodies)
    {
        FVector PosKm = SpiceSubsystem->GetBodyPosition(Body, TEXT("SUN"), ET);
        
        // Update planet actor position
        if (PlanetActors.Contains(Body))
        {
            FVector WorldPos = PosKm * VisualizationScale;
            PlanetActors[Body]->SetActorLocation(WorldPos);
        }
    }
}
```

## Troubleshooting

**Compilation Errors:**
- Ensure SPICE library is in `ThirdParty/CSPICE/<Platform>/lib/`
- Verify include headers in `ThirdParty/CSPICE/<Platform>/include/`

**Runtime Errors:**
- Check kernel files exist in `Content/SPICE/Kernels/`
- Verify kernel paths are relative to Content directory
- Check log output for SPICE error messages

**Invalid Results:**
- Ensure kernels cover time range being queried
- Verify body names are correct (case-sensitive)
- Check reference frame is valid for query

## References

- [Full Documentation](README.md)
- [SPICE Toolkit Docs](https://naif.jpl.nasa.gov/naif/documentation.html)
- [NAIF Body ID Codes](https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/naif_ids.html)
- [Kernel Download](https://naif.jpl.nasa.gov/pub/naif/generic_kernels/)
