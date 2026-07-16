# UETPFCore Skills Library

These skills encode how this
project is actually built and maintained — read them in order to understand them,
lean on them individually afterward. Claude Code loads each skill on demand
when its description semantically matches the task at hand.

## Reading order

| # | Skill | When it fires |
|---|-------|---------------|
| 1 | [01-project-onboarding](01-project-onboarding/SKILL.md) | First contact: what UETPFCore is, build/test/editor loop |
| 2 | [02-unreal-mcp-driving](02-unreal-mcp-driving/SKILL.md) | Any editor automation through the UE 5.8 Unreal MCP server |
| 3 | [03-simulation-conventions](03-simulation-conventions/SKILL.md) | Writing or reviewing C++ against the framework |


## House rules that outrank everything else

1. This is a **published, Apache 2.0, public repository**. Every commit is
   read by strangers. Conventional commits, no generated noise, no secrets
   (the AndroidFileServer `SecurityToken` block in `DefaultEngine.ini` is
   auto-generated — never commit it).
2. The framework's value is its **discipline**: subsystems, data-driven specs,
   sparse deltas, one coordinate authority. When a shortcut fights one of
   those, the shortcut loses.
3. Docs live in `docs/`, agent context in `CLAUDE.md`, and this library.
   When behavior and docs disagree, fix whichever is wrong — never let them
   drift apart silently.
