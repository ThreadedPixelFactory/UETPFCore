# SPICE Library Installation

This directory should contain the CSPICE library from NASA/JPL.

## Download

Get the appropriate version for your platform from:
https://naif.jpl.nasa.gov/naif/toolkit_C.html

## Directory Structure

After extraction, your directory should look like:

```
ThirdParty/CSPICE/
├── cspice/
    ├── include/
    │   ├── Win64/
    │   │   └── SpiceUsr.h (and other headers)
    │   ├── Linux/
    │   │   └── SpiceUsr.h
    │   └── Mac/
    │       └── SpiceUsr.h
    └── lib/
        ├── Win64/
        │   └── cspice.lib
        ├── Linux/
        │   └── libcspice.a
        └── Mac/
            └── libcspice.a
```

## Installation Steps

### Windows

1. Download `cspice_win_msvc_64bit.zip` or similar
2. Extract to `ThirdParty/cspice/`
3. Create directory structure:
   - `ThirdParty/cspice/include/Win64/` - Copy all header files here
   - `ThirdParty/cspice/lib/Win64/` - Copy cspice.lib here
4. Verify `ThirdParty/cspice/lib/Win64/cspice.lib` exists
5. Verify `ThirdParty/cspice/include/Win64/SpiceUsr.h` exists

### Linux

1. Download `cspice_linux_gcc_64bit.tar.Z` or similar
2. Extract to `ThirdParty/cspice/`
3. Create directory structure:
   - `ThirdParty/cspice/include/Linux/` - Copy all header files here
   - `ThirdParty/cspice/lib/Linux/` - Copy libcspice.a here
4. Verify `ThirdParty/cspice/lib/Linux/libcspice.a` exists
5. Verify `ThirdParty/cspice/include/Linux/SpiceUsr.h` exists

### macOS

1. Download `cspice_mac_osx_clang_64bit.tar.Z` or similar
2. Extract to `ThirdParty/cspice/`
3. Create directory structure:
   - `ThirdParty/cspice/include/Mac/` - Copy all header files here
   - `ThirdParty/cspice/lib/Mac/` - Copy libcspice.a here
4. Verify `ThirdParty/cspice/lib/Mac/libcspice.a` exists
5. Verify `ThirdParty/cspice/include/Mac/SpiceUsr.h` exists

## Building Without SPICE

If you don't need scientific simulation features, you can build the project without installing SPICE.

The module will compile with `WITH_SPICE=0` and all functions will return invalid results with appropriate warnings.

## License

SPICE Toolkit is provided by NASA/JPL and is not subject to copyright in the United States.

See: https://naif.jpl.nasa.gov/naif/rules.html

**Note:** This repository does NOT include SPICE binaries. You must download them separately.
