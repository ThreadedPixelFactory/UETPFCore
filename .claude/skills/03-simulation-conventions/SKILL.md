---
name: 03-simulation-conventions
description: UETPFCore C++ conventions — subsystem access, data-driven specs, delta persistence, coordinate frames, UniversalSkyActor manager pattern, and the anti-patterns that get PRs rejected. Use when writing, reviewing, or refactoring any C++ in this repo.
---

# Simulation Conventions

These are the load-bearing walls. Everything else is negotiable.

## 1. Subsystems are the architecture

All major systems are UE World/GameInstance subsystems. Access per-use:

```cpp
UEnvironmentSubsystem* Env = GetWorld()->GetSubsystem<UEnvironmentSubsystem>();
if (!Env) return;
```

Never cache a subsystem as `UPROPERTY` — lifetimes don't match and it *will*
dangle across world transitions. Never bypass subsystems with
statics/singletons.

World: Time, Environment, SurfaceQuery, Biome, PhysicsIntegration.
GameInstance: SolarSystem, WorldFrame, StarCatalog.

## 2. Behavior is data (specs)

`UMediumSpec` (fluids/atmospheres), `USurfaceSpec` (contact materials),
`UBiomeSpec` (regions). Load and register **once** in
`InitGame`/`BeginPlay`; never in Tick, never mid-gameplay. Lookups are
O(1) TMap after registration. Hardcoding a material property in C++ instead
of a spec is a design bug even when the number is right.

## 3. One coordinate authority

- **Canonical frame**: km, double precision — physics truth.
- **UE world frame**: cm — rendering/gameplay.

```cpp
UWorldFrameSubsystem* Frame = GetWorld()->GetSubsystem<UWorldFrameSubsystem>();
FVector WorldPos = Frame->CanonicalToWorld(KmPosition);
```

`* 100000.0` by hand misses the frame offset and produces bugs that only
appear far from origin. There are no exceptions to this rule.

## 4. Deltas are sparse

Persistence stores *changes* (FSurfaceDelta, FFractureDelta,
FTransformDelta), cell-partitioned to match World Partition. Emit a delta on
significant state change only — a delta per frame means the design is wrong,
not the budget.

## 5. Rendering managers reference, they don't own

`UniversalSkyActor` is the template: it holds **references** to separately
placed SkyAtmosphere / DirectionalLight / SkyLight actors and orchestrates
them, driven by `TimeSubsystem::OnTimeAdvanced` (event, not Tick). Owning
those components in one actor breaks UE's atmospheric rendering registration.
Any future manager over engine rendering systems follows this pattern.

## Anti-pattern quick list

- `UPROPERTY()` cached subsystem pointer
- `LoadObject` / spec registration in Tick
- manual km↔cm math
- per-frame delta emission
- component-owning rendering managers
- static/singleton access to anything the subsystem pattern covers

## Threading

Game thread by default; Chaos runs on the physics thread; use
`AsyncPhysicsTickComponent` for per-actor physics callbacks. Details and
Task Graph guidance: `docs/threading-guidelines.md`.
