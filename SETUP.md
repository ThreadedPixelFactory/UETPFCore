<!--
Copyright Threaded Pixel Factory. All Rights Reserved.
Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
-->

# Setup Guide - UETPFCore

## Prerequisites

### Required Software
- **Unreal Engine 5.7** (from Epic Games Launcher)
- **Visual Studio 2022** (Community, Professional, or Enterprise)
  - Workload: "Game development with C++"
  - Include: "Unreal Engine installer"
- **Git** (for version control)
- **Windows 10/11** (64-bit)

### Recommended Hardware
- CPU: 8+ cores
- RAM: 32GB minimum (64GB recommended)
- GPU: RTX 3060 or equivalent (6GB+ VRAM)
- Storage: 100GB+ SSD space

## Installation Steps

### 1. Clone the Repository

```bash
git clone <repository-url>
cd UETPFC
```
If you are building your own project update: [DefaultGame.ini](Config/DefaultGame.ini)
with your project information

### 2. Generate Project Files

Right-click on `UETPFCore.uproject` and select:
- "Switch Unreal Engine version..." → Select 5.7
- "Generate Visual Studio project files"

This creates `UETPFCore.sln`.

### 3. Build the Project

**Option A: Using Batch Scripts** (Recommended)
```batch
cd Scripts
build.bat
```

**Option B: Using Visual Studio**
1. Open `UETPFCore.sln`
2. Set build configuration to "Development Editor"
3. Set platform to "Win64"
4. Build Solution (Ctrl+Shift+B)

### 4. Launch the Editor

**Option A: Using Batch Script**
```batch
cd Scripts
editor.bat
```

**Option B: Double-click**
- Double-click `UETPFCore.uproject`

## Batch Script Reference

Located in `Scripts/` directory:

| Script | Purpose |
|--------|---------|
| `vars.bat` | Shared configuration (engine path, version) |
| `build.bat` | Compile the editor target |
| `editor.bat` | Launch Unreal Editor |
| `cook.bat` | Cook content for packaging |
| `test.bat` | Runs automated tests you write  |
| `clean.bat` | Remove build artifacts |

Edit `vars.bat` to configure your engine installation path.

## Verification

After launching the editor, verify the installation:

1. **Check Modules**: Settings → Plugins
   - UETPFCore should be listed
   - GameLauncher should be listed
   - SinglePlayerStoryTemplate should be listed

2. **Open Example Content**: Content Browser
   - Navigate to `Content/TPF/Examples/`
   - Verify example assets load without errors

3. **Check Output Log**: Window → Developer Tools → Output Log
   - Look for module startup messages
   - No red errors should appear

## Common Issues

### "Module not found" Error
- Solution: Regenerate project files (right-click .uproject)
- Verify Build.cs files have correct module names

### Compilation Errors
- Ensure Visual Studio has "Game development with C++" workload
- Verify Unreal Engine 5.7 is correctly installed
- Check Engine path in `Scripts/vars.bat`

### Editor Crashes on Launch
- Delete `Intermediate/`, `Binaries/`, `Saved/` folders
- Regenerate project files
- Rebuild from scratch

### Missing Content
- Content is minimal by design (this is a framework)
- Example content is in `Content/TPF/`
- Create your own game content in `Content/YourGame/`

## Next Steps

1. Read [ARCHITECTURE.md](ARCHITECTURE.md) to understand the system design
2. Follow [IMPLEMENTATIONGUIDE.md](IMPLEMENTATIONGUIDE.md) to integrate into your game
3. Explore example content in `Content/TPF/Examples/` (coming soon)
4. Create your first Data Assets (Medium specs, Surface specs)

## Directory Structure After Setup

```
UETPFC/
├── .git/                           # Git repository
├── .vs/                            # Visual Studio data (gitignored)
├── Binaries/                       # Compiled binaries (gitignored)
│   └── Win64/
├── Content/                        # Game content
│   ├── TPF/                        # Template example content
│   └── (your game folders)
├── Intermediate/                   # Build intermediates (gitignored)
├── Saved/                          # Saved data (gitignored)
├── Scripts/                        # Build automation scripts
├── Source/                         # C++ source code
│   ├── UETPFCore/                  # Core framework module
│   ├── GameLauncher/               # Menu system module
│   └── SinglePlayerStoryTemplate/  # Template game module
├── LICENSE.txt                     # Apache 2.0 License
├── README.md                       # Project overview
├── SETUP.md                        # This file
├── ARCHITECTURE.md                 # System architecture
├── IMPLEMENTATIONGUIDE.md          # Integration guide
├── CONTRIBUTING.md                 # Contribution guidelines
└── UETPFCore.uproject              # Unreal project file
```

## Support

If you encounter issues not covered here:
1. Check the Output Log for detailed error messages
2. Search GitHub Issues
3. Create a new issue with:
   - Error message
   - Steps to reproduce
   - System specs
   - Log files (from `Saved/Logs/`)

---

Ready to build? Continue to [IMPLEMENTATIONGUIDE.md](IMPLEMENTATIONGUIDE.md)
