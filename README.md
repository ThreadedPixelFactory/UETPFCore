<!--
Copyright Threaded Pixel Factory LLC. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# UETPFCore - Unreal Engine Threaded Physics Factory Core

## A Multi-Scale Simulation Framework for Unreal Engine 5.7

UETPFCore is an Apache 2.0-licensed forward looking framework leveraging experimental engine features and aims to become a simple production-ready, framework for building physics-driven programs and scientific simulations in Unreal Engine 5.7+.

It provides cognitive structure for projects with a robust **ready-to-build** subsystem architecture inside a minimal project aimed at high fidelity multi-scale simulations, supporting everything from planetary-scale coordinate systems down to centimeter-precision physics interactions.

## Key Features

### 🌍 Multi-Scale Coordinate System
- Large World Coordinates (LWC) integration
- Seamless km ↔ cm transformations
- Coordinate frame management for planetary bodies
- Precision handling across vast scales

### ⚙️ Subsystem Architecture
- **TimeSubsystem**: Configurable time dilation and progression
- **WorldFrameSubsystem**: Multi-scale coordinate transformations
- **EnvironmentSubsystem**: Medium specs (air, water, vacuum)
- **SurfaceQuerySubsystem**: Material behavior (friction, deformation)
- **BiomeSubsystem**: Environmental region management
- **PhysicsIntegrationSubsystem**: Chaos physics integration

### 🎯 Dual-Domain Physics
- **MediumSpec**: Fluid/atmosphere physics (drag, buoyancy, sound)
- **SurfaceSpec**: Contact material behavior (friction, compliance, FX)
- Data-driven specs via UE Data Assets
- Runtime-loadable configurations

### 💾 Delta Persistence System
- Sparse delta storage for world state changes
- Surface deltas (snow, wetness, deformation)
- Fracture/destruction state
- Transform deltas for dynamic objects
- Assembly state for complex objects

### 🌌 Celestial Body Management
- Generic celestial body framework
- LOD proxy/simulation actor patterns
- Orbital mechanics support
- Atmospheric rendering integration

## Quick Start

### Prerequisites
- Unreal Engine 5.7
- Windows (primary support)
- Visual Studio 2022 or later

### Installation

1. Clone or download this repository
2. Run `Scripts/build.bat` to compile the project
3. Run `Scripts/editor.bat` to launch the editor
4. Explore the example content in `Content/TPF/Examples/`

See [SETUP.md](SETUP.md) for detailed setup instructions.

## Module Structure

### UETPFCore (Runtime)
The core physics framework providing subsystems and base classes.

### GameLauncher (Runtime)
Generic menu system for launching game modules. Demonstrates:
- Main menu architecture
- Enhanced Input integration
- Level loading patterns

### SinglePlayerStoryTemplate (Runtime)
Template module demonstrating single-player game patterns:
- Local file-based persistence (FileDeltaStore)
- JSON spec loading (SpecPackLoader)
- Story game mode and player controller examples

## Documentation

- **[SETUP.md](SETUP.md)** - Step-by-step setup guide
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture deep dive
- **[IMPLEMENTATIONGUIDE.md](IMPLEMENTATIONGUIDE.md)** - Integration guide for your game
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines

## Example Content (coming soon)

Located in `Content/TPF/`:
- Example Data Assets (air, water, rock specs)
- Example materials (atmosphere, terrain)
- Example Blueprints (proxy/simulation actors)
- Minimal demo level

## Use Cases

This framework is ideal for:
- Space exploration games
- Planetary surface games
- Multi-scale simulation games
- Physics-driven story games
- Flight simulators
- Vehicle simulations

## Epic Games Mega Grant

This project is being submitted for the Epic Games Mega Grant program. The framework demonstrates production-quality systems architecture using Unreal Engine best practices and is designed to benefit the Unreal Engine community by providing a solid foundation for physics-based games and simulations. Helpful feedback that can help the platform succeed is welcomed.

The project also aims to benefit the larger developer community and the general public at large. We aim for the project to expand beyond games to facilitate cities to build robust simulations that can help iterate through climate resilience solutions affordably. We are currently using approximations for performance, however the project is built to be extensible to high fidelity implementations.

We are available to facilitate them.

## License

This project is licensed under the Apache License, Version 2.0 - see [LICENSE.txt](LICENSE.txt) for details.

Copyright 2026 Threaded Pixel Factory LLC

## Support This Project

UETPFCore is free and open source, developed and maintained by [Threaded Pixel Factory](https://www.tpf.studio).

### 🐛 Community Support
- **Issues**: Report bugs via [GitHub Issues](https://github.com/ThreadedPixelFactory/UETPFCore/issues)
- **Discussions**: Community discussion via [GitHub Discussions](https://github.com/ThreadedPixelFactory/UETPFCore/discussions)
- **Contributing**: See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines

### 💖 Financial Support

Monthly or one-time support helps us maintain and improve this framework:

- 💖 **[GitHub Sponsors](https://github.com/sponsors/ThreadedPixelFactory)** - Recurring support helps the platform grow.
- ☕ **[Ko-fi](https://ko-fi.com/threadedpixelfactory)** - One-time tips or additional monthly support
- 🎨 **[Patreon](https://www.patreon.com/ThreadedPixelFactory)** - Supporters get Early access to exclusive development blog posts that are open to feedback before they're published. Your contributions help the platform grow!
- ⭐ **Star this repo** - Free way to help others discover the project

### 🏢 Professional Services

- **Enterprise Support** - Priority bug fixes and technical consultation for commercial projects
- **Custom Development** - Advanced features and integration work for your game
- **Training & Workshops** - Team training on framework architecture and best practices coming soon.

[Contact us](mailto:support@tpf.studio) for professional service inquiries.

## Acknowledgments

Built with:
- Unreal Engine 5.7 by Epic Games
- Chaos Physics System
- World Partition streaming
- PCG (Procedural Content Generation)
- Niagara VFX
- MetaSounds audio

## Roadmap

- [ ] 📦 Additional example content
- [ ] 🎥 Video tutorials
- [ ] 🔷 Blueprint-only workflow examples
- [ ] 📱 Mobile platform support
- [ ] 🐧 Linux/Mac support

---

**Built for the Unreal Engine community by Threaded Pixel Factory** 🎮