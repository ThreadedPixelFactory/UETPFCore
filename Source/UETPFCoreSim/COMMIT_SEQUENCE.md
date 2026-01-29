# UETPFCoreSim Branch - Commit Sequence

This document outlines the intended commit sequence for the UETPFCoreSim branch.

## Branch Strategy

**Branch Name:** `feature/uetpfcoresim-scientific-simulation`

**Base Branch:** `main` - Maintainers keep branch up to date with main.

**Purpose:** UETPFCoreSim Adds optional scientific simulation module with SPICE integration for aerospace training, educational sims, and validated ephemerides

## Commit Sequence

### 1. Add UETPFCoreSim Module Structure

**Files:**
- `Source/UETPFCoreSim/UETPFCoreSim.Build.cs`
- `Source/UETPFCoreSim/Public/UETPFCoreSim.h`
- `Source/UETPFCoreSim/Private/UETPFCoreSim.cpp`
- `UETPFCore.uproject` (add module entry)

**Commit Message:**
```
feat(sim): Add UETPFCoreSim module structure

- Create optional scientific simulation module
- Configure SPICE library integration in Build.cs
- Add WITH_SPICE conditional compilation
- Platform-specific library linking (Win64/Linux/Mac)
- Module provides IsSpiceAvailable() runtime check

This module is separate from core framework to keep
scientific simulation features optional.
```

**Why This Order:**
Start with module foundation so subsequent commits build on working structure.

---

### 2. Implement SPICE Ephemerides Subsystem

**Files:**
- `Source/UETPFCoreSim/Public/Subsystems/SpiceEphemeridesSubsystem.h`
- `Source/UETPFCoreSim/Private/Subsystems/SpiceEphemeridesSubsystem.cpp`

**Commit Message:**
```
feat(sim): Implement SPICE ephemerides subsystem

- Add USpiceEphemeridesSubsystem game instance subsystem
- Kernel management: Load/Unload SPK, PCK, LSK files
- Ephemeris queries: GetBodyState, GetBodyPosition, GetSunDirection
- Thread-safe SPICE wrapper with FCriticalSection
- Time format conversions (Unix ↔ Julian ↔ Ephemeris Time)
- Automatic unit conversion (SPICE km → UE cm)
- Error handling with SPICE error reporting

Enables validated celestial mechanics via NASA's SPICE Toolkit
for aerospace training and educational simulations in UE.

```

**Why This Order:**
Core functionality complete before documentation. Keeps functional code separate from docs.

---

### 3. Add UETPFCoreSim Documentation

**Files:**
- `Source/UETPFCoreSim/README.md`

**Commit Message:**
```
docs(sim): Add UETPFCoreSim module documentation

- Installation guide for SPICE library and kernels
- Usage examples (C++ and Blueprint)
- Coordinate frame documentation
- Performance considerations
- Accuracy limitations and scope disclaimer
- Contribution guidelines for remaining features

Documents planned features:
- Deterministic physics component (not yet implemented)
- USD import/export for Omniverse (not yet implemented)
- Example content (not yet implemented)

Clearly states module is for educational/training use,
NOT for mission-critical or publishable research.

```

**Why This Order:**
Documentation comes after implementation is complete. Makes it easier to document actual API.

---

### 4. Update Main README with Scientific Sim Link

**Files:**
- `README.md` (root)

**Changes:**
Add section under "Modules" or "Features":


**Commit Message:**
```
docs: Add UETPFCoreSim module to main README

- Link to scientific simulation module documentation
- Clarify optional nature and use cases
- Note educational/training scope

```

**Why This Order:**
Update main README last so it can reference complete module documentation.

---

### 5. Call for contribution for Remaining Features

**Files:**
- `CONTRIBUTING.md` (Read contribution guidelines)

```markdown
## UETPFCoreSim - Priority Contributions

The scientific simulation module has several planned features that need implementation:

### 1. Deterministic Physics Component
**Priority:** High  
**Difficulty:** Medium  
**Skills:** Unreal Engine C++, Chaos physics

Implement `UDeterministicPhysicsComponent` for fixed-timestep simulation with rollback support.

See: [UETPFCoreSim README - To Be Implemented](Source/UETPFCoreSim/README.md#-to-be-implemented)

### 2. USD Import/Export
**Priority:** Medium  
**Difficulty:** High  
**Skills:** USD library, coordinate transformations

Bridge NVIDIA Omniverse (Isaac Sim) with Unreal Engine for hybrid workflows.

### 3. Example Content
**Priority:** High  
**Difficulty:** Low  
**Skills:** Blueprints, content creation

Create tutorial maps demonstrating SPICE integration and solar system visualization.

### 4. Validation Tests
**Priority:** Medium  
**Difficulty:** Medium  
**Skills:** Unit testing, astronomical validation

Validate SPICE wrapper against JPL HORIZONS data.
```

**Commit Message:**
```
docs: Add contribution guidelines for UETPFCoreSim features

- Document priority features needing implementation
- Provide skill requirements and difficulty estimates
- Link to detailed specifications in module README

Encourages community contributions for:
- Deterministic physics component
- USD import/export
- Example content
- Validation testing

Related: #[ISSUE_NUMBER]
```

**Why This Order:**
Contribution guidelines come last after all technical work is documented. Gives contributors clear picture of what's done and what's needed.

---

## Pre-Push Checklist

Before pushing branch to remote:

- [ ] All commits follow conventional commit format
- [ ] Code compiles with SPICE library installed
- [ ] Code compiles WITHOUT SPICE library (WITH_SPICE=0 fallback works)
- [ ] Module README includes disclaimer about scope limitations
- [ ] No TODO comments in committed code (move to GitHub issues)
- [ ] License headers present in all source files
- [ ] No debug logging left in release code
- [ ] API documentation complete (Doxygen comments)

## Pull Request Template

When opening PR:

**Title:**
```
feat(sim): Add UETPFCoreSim scientific simulation module
```

**Description:**
```
## Overview
Adds optional UETPFCoreSim module providing validated celestial mechanics via NASA's SPICE Toolkit.

## Features Implemented
- ✅ SPICE Ephemerides Subsystem (thread-safe wrapper)
- ✅ Kernel management (SPK, PCK, LSK files)
- ✅ Ephemeris queries (position, velocity, sun direction)
- ✅ Time format conversions
- ✅ Unit conversions (km → cm)

## Features Planned (Not Implemented)
- ⏳ Deterministic physics component
- ⏳ USD import/export for Omniverse
- ⏳ Example content and tutorial maps
- ⏳ Validation tests

## Target Use Cases
- Aerospace training simulations
- Educational content (planetarium, orbital mechanics)
- Robotics visualization with validated ephemerides
- Hybrid workflows (MATLAB/Isaac Sim → Unreal)

## NOT For
- Mission-critical aerospace operations
- Publishable scientific research
- Safety-critical simulations

## Documentation
- [Module README](Source/UETPFCoreSim/README.md) - Installation and usage
- [CONTRIBUTING.md](CONTRIBUTING.md) - How to implement remaining features

## Testing
- [x] Compiles with SPICE library installed (Win64)
- [x] Compiles WITHOUT SPICE library (graceful fallback)
- [x] Module can be excluded from build (optional)
- [ ] Validated against HORIZONS data (needs contribution)

## Breaking Changes
None. Module is optional and does not affect existing code.

## Related Issues
Closes #[ISSUE_NUMBER]
```

## Post-Merge Tasks

After PR is merged to main:

1. **Create GitHub Issues** for remaining features:
   - Issue: "Implement Deterministic Physics Component"
   - Issue: "Add USD Import/Export for Omniverse"
   - Issue: "Create Solar System Example Map"
   - Issue: "Validate SPICE Wrapper Against HORIZONS"

2. **Tag Contributors** in issues:
   - Label issues with `good first issue`, `help wanted`, `enhancement`
   - Add difficulty labels: `difficulty:low`, `difficulty:medium`, `difficulty:high`
   - Add skill labels: `skill:c++`, `skill:blueprints`, `skill:physics`

3. **Update Project Board**:
   - Move UETPFCoreSim tasks to "Done"
   - Add remaining feature cards to "Backlog"

4. **Announce on Community Channels**:
   - Post on Patreon about new scientific simulation capabilities
   - LinkedIn post targeting aerospace training and robotics visualization
   - Call for contributors with USD/Omniverse experience

5. **Documentation Site**:
   - If you have docs site, add UETPFCoreSim section
   - Include API reference generated from Doxygen comments

## Branch Cleanup

After merge:

```bash
# Delete remote feature branch
git push origin --delete feature/uetpfcoresim-scientific-simulation

# Delete local branch
git branch -d feature/uetpfcoresim-scientific-simulation

# If you had any WIP branches
git branch -D wip/spice-wrapper
```

## Notes for Maintainers

### SPICE Library Not Included
Repository does NOT include SPICE library binaries. Users must download separately.

**Reason:** Licensing clarity (NASA allows redistribution but we keep clean separation)

### Conditional Compilation
Module uses `WITH_SPICE` define. Project compiles without SPICE library installed.

**Test both paths:**
```bash
# With SPICE
# (place library in ThirdParty/CSPICE/)
Build.bat

# Without SPICE
# (remove ThirdParty/CSPICE/)
Build.bat
```

### Platform Testing
Current implementation tested on **Win64 only**.

**Needs testing:**
- Linux (uses .a static library)
- macOS (uses .a static library)

Add CI checks for multi-platform builds if possible.

---

**Questions?** Open a discussion in GitHub Discussions before pushing branch.
