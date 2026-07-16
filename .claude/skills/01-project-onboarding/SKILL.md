---
name: 01-project-onboarding
description: Orientation for UETPFCore — project purpose, module layout, build/test/editor commands, clangd setup, and where the authoritative docs live. Use when starting work in this repo, setting up a machine, or answering "how do I build/run/test this".
---

# Project Onboarding

UETPFCore is a multi-scale simulation framework for **Unreal Engine 5.8**
(Windows-first, Apache 2.0). It exists to make physics-driven world
simulation — from planetary scale down to centimeter contact physics —
buildable by small teams, and to let cities iterate on climate-resilience
simulations affordably (see README "Mega Grant" section; that sentence is the
mission, not marketing).

## Modules

```
UETPFCore                 ← core framework (subsystems, specs, deltas)
├─ GameLauncher           ← generic menu/launch module
└─ SinglePlayerStoryTemplate ← reference game module (persistence, spec loading)
```

`UETPFCoreSim` (branch, not on main) adds the high-fidelity simulation module
(SPICE ephemerides, sim foundations). See skill 04 before touching it.

## Daily loop

```batch
Scripts\build.bat            # Development Editor build
Scripts\editor.bat           # launch editor (add -log for console)
Scripts\test.bat [filter]    # automation tests, e.g. test.bat UETPFCore
Scripts\clean.bat            # nuke build artifacts
```

- Engine path/version live in `Scripts\vars.bat` (`ENGINE_VERSION=5.8`).
- After editing any `*.Build.cs`: right-click `UETPFCore.uproject` →
  "Generate Visual Studio project files", then `Scripts\gen_clang_db.bat`
  so clangd stays truthful. `docs/clangd-setup.md` covers first-time setup.

## Where truth lives

| Question | Source |
|----------|--------|
| Architecture rationale | `docs/ARCHITECTURE.md` |
| How to integrate a game | `docs/IMPLEMENTATIONGUIDE.md` |
| Machine setup | `docs/SETUP.md` |
| Threading rules | `docs/threading-guidelines.md` |
| Agent-facing summary | `CLAUDE.md` |

## First-week checklist

1. Build and launch the editor; confirm all three modules load (Output Log).
2. Read `docs/ARCHITECTURE.md` end to end — it is short on purpose.
3. Read skill 03 (simulation conventions) before writing any C++.
4. Enable Unreal MCP (skill 02) — most editor work here is agent-driven.
