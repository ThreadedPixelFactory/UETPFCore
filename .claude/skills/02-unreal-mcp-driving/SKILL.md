---
name: 02-unreal-mcp-driving
description: Operating the UE 5.8 Unreal MCP server from Claude Code — enabling the plugin, generating client config, discovering toolsets, and the safety rules for agent-driven editor work. Use whenever automating Unreal Editor (spawning actors, editing levels, materials, running tests) via MCP.
---

# Driving Unreal Editor through MCP

UE 5.8 ships an experimental **Unreal MCP** plugin: an embedded MCP server in
the editor that exposes engine operations as tools over local HTTP. This is
the primary way agents build levels and content in this project.

Official docs: <https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor>

## One-time setup (per machine)

1. Edit → Plugins → search "Unreal MCP" → enable → restart editor.
   *Deliberately not enabled in `UETPFCore.uproject` — this is a public
   template and MCP is a local dev tool, not a project dependency. Do not
   "fix" that by committing the plugin flag.*
2. Edit → Editor Preferences → **Model Context Protocol** → enable
   **Auto Start Server**. Default endpoint: `http://127.0.0.1:8000/mcp`.
3. In the editor console:
   `ModelContextProtocol.GenerateClientConfig ClaudeCode`
   This writes `.mcp.json` into the project root. It is **gitignored** —
   per-machine, never committed.
4. Launch Claude Code from the project root so it picks up `.mcp.json`.

## Discovering tools

The server is self-describing. Always start with the meta-tools:

- `list_toolsets` — enumerate available toolsets (SceneTools, ActorTools,
  MaterialInstanceTools, ObjectTools, and experimental ones like
  AttributeSetToolset).
- `describe_toolset` — get the tools and schemas inside one toolset.
- `call_tool` — invoke a specific tool.

Toolsets evolve every engine release; discover, don't memorize.

## Safety rules (non-negotiable)

1. **Serial calls only.** The server executes on the game thread, serially.
   Never issue overlapping/parallel MCP calls — queue them.
2. **Localhost only, no auth.** Never bind it to a non-loopback address,
   never tunnel it. Anyone who can reach the port owns your editor.
3. **Save deliberately.** Agent edits dirty the level/assets like hand edits.
   Review what changed (File → Save All shows the dirty list) before saving;
   don't leave a half-mutated level for the next session.
4. **Verify with automation tests.** The MCP surface can run automation
   tests — after any scripted content build, run the relevant
   `Scripts\test.bat` filter or the in-editor test before declaring success.
5. **Content is binary.** `.umap`/`.uasset` changes can't be diffed in
   review. Prefer regenerable content: PCG graphs, data assets, and scripted
   construction over hand-tuned one-offs, so a reviewer can re-run the recipe.

## Project workflow pattern

For repeatable scene construction (e.g., city blocks):
write the construction as a sequence of MCP calls driven from a checked-in
recipe (JSON/data asset), not as an unrecorded interactive session. The
recipe is the artifact; the level is its output.
