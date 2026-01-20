<!--
Copyright Threaded Pixel Factory. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# Template Physics Factory (TPF) Content - (In progress)

This directory will contain example content demonstrating the UETPFCore framework.

## Directory Structure

### Examples/
Example Data Assets demonstrating the spec system:
- Medium specs (air, water environments)
- Surface specs (rock, soil materials)
- Example levels showing system integration

### Materials/
Example materials demonstrating:
- Atmosphere rendering
- Procedural terrain materials
- Physics-driven material effects

### Blueprints/
Example Blueprint implementations:
- BP_ExampleProxyActor - LOD proxy pattern
- BP_ExampleSimulationActor - High-detail simulation pattern
- Example gameplay actors

## Usage

These are to be **examples only** - customize or replace them for your game.

The spec system (MediumSpec, SurfaceSpec, etc.) is designed to be data-driven.
Create your own Data Assets inheriting from these base types to define your game's physics behavior.

## Implementation Guide

See the root-level documentation:
- ARCHITECTURE.md - System architecture overview
- IMPLEMENTATIONGUIDE.md - Step-by-step integration guide
- SETUP.md - Initial setup instructions
