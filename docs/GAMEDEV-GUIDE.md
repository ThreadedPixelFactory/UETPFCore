<!--
Copyright Threaded Pixel Factory LLC. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# Game Developer's Guide — Building Games on UETPFCore's Simulation Systems

The UETPFCoreSim project branch is used to build serious simulations. This guide is for the other audience: **game
developers** who want to turn the same machinery — energy systems, weather
events, economics, multi-scale worlds — into gameplay. City builders,
survival games, disaster-management sims, and space colonies all run on the
same loop: *generate, store, spend, survive the event, repair, expand.*

## The core idea: your game design lives in Data Assets

UETPFCore separates *rules* (C++ subsystems) from *content* (spec Data
Assets). As a game developer you mostly author specs:

| You want | You author | You do NOT write |
|---|---|---|
| A new power source (fusion cell, hamster wheel) | a `UGenerationModuleSpec` asset | battery/actor C++ |
| A harsher winter for hard mode | `UWeatherEventSpec` + `UBiomeSpec` variants | weather code |
| Different prices per district | `UTariffSpec` / economy spec assets | economy code |
| A new planet's atmosphere | `UMediumSpec` | physics code |

Balance passes are data edits. Mods are data packs. Difficulty modes are
spec swaps. This is deliberate — protect it.

## Wiring the simulation into a game loop

### 1. Register once, at game start

```cpp
void AMyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    // Load spec packs once; register with subsystems. Never in Tick.
}
```

`SinglePlayerStoryTemplate` shows the full pattern: JSON spec loading
(`SpecPackLoader`) and local persistence (`FileDeltaStore`).

### 2. Read state through subsystems, per use

```cpp
if (UEnergyGridSubsystem* Grid = GetWorld()->GetSubsystem<UEnergyGridSubsystem>())
{
    HUD->SetBlackoutRisk(Grid->GetUnservedDemandRatio());
}
```

Same rule as everywhere in this framework: query per-use, never cache
subsystem pointers as `UPROPERTY`.

### 3. Drive gameplay from events, not polling

The simulation publishes delegates — `TimeSubsystem::OnTimeAdvanced`,
`OnGridStateChanged`, weather event begin/end. Bind quests, alarms, UI, and
AI reactions to those. A blackout mission trigger is one delegate binding,
not a Tick that watches a float.

### 4. Persistence is already solved

Player-visible world changes (a repaired turbine, a depleted battery, storm
damage) are deltas. Use the delta store for your save system instead of
inventing one; you get sparse, cell-partitioned saves aligned with World
Partition streaming for free.

## Game-design levers the simulation gives you

- **Time dilation** (`TimeSubsystem`): run a year of seasons in an hour of
  play, or slow to real-time during a crisis set piece.
- **Weather events as encounters**: a `UWeatherEventSpec` is a boss fight —
  telegraph (forecast), pressure (demand spike/damage rolls), aftermath
  (repair queues, budget hits). The damage/repair economics from the case
  study are a survival-game loop out of the box.
- **Scale as progression**: the study's S1→S4 ladder (building → block →
  district → borough) is a tech tree. Each tier changes the numbers via
  specs, not new code.
- **Honest physics as difficulty**: heat→electricity conversion is lossy
  (~20–30%) in reality — keeping it lossy in-game creates genuine strategic
  tension between heat uses and electric uses. The realism *is* the game.
- **The skyline as UI**: with MegaLights (UE 5.8, production-ready), which
  buildings are lit is readable at borough scale — outage state needs no
  minimap overlay.

## Performance ground rules (non-negotiable at 60 fps)

1. Simulation integrates on time events; **nothing energy-related ticks per
   frame**. If your design needs per-frame feedback, interpolate visuals
   client-side between simulation steps.
2. Deltas on significant change only; a delta per frame is a design bug.
3. Rendering: prefer **Lumen Lite** for large scenes, MegaLights for many
   dynamic lights; leave cinematic features (Accumulation DoF) out of
   interactive play.
4. Big worlds: World Partition + the canonical/world frame split are already
   there — use `UWorldFrameSubsystem` for every coordinate conversion and
   your gameplay code survives a move to planet scale unchanged.

## Where to go next

- Architecture rationale: [ARCHITECTURE.md](ARCHITECTURE.md)
- Integration walkthrough: [IMPLEMENTATIONGUIDE.md](IMPLEMENTATIONGUIDE.md)
- The serious-simulation counterpart to this guide:
  [case-studies/nyc-grid-resilience.md](case-studies/nyc-grid-resilience.md)
- Agent-assisted content workflows (Unreal MCP, UE 5.8):
  [.claude/skills/02-unreal-mcp-driving](../.claude/skills/02-unreal-mcp-driving/SKILL.md)
