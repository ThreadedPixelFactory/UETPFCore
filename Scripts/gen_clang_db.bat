@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0.
REM ===========================================================================
REM gen_clang_db.bat - Generate compile_commands.json for clangd LSP
REM ===========================================================================
REM Generates a Clang compilation database at the project root.
REM clangd reads this file to resolve includes, macros, and type information.
REM
REM Run this:
REM   - After first clone
REM   - After modifying any Build.cs file
REM   - After adding a new module
REM   - After switching to a branch with different source structure
REM
REM Prerequisites: See docs/clangd-setup.md
REM ===========================================================================

call "%~dp0vars.bat"

echo Generating compile_commands.json for %PROJECT_NAME%...
echo Target: %TARGET_EDITOR% %PLATFORM% %CONFIG%
echo.

call "%BUILD_BAT%" ^
    -mode=GenerateClangDatabase ^
    -project="%PROJECT_FILE%" ^
    %TARGET_EDITOR% %PLATFORM% %CONFIG%

REM Capture ERRORLEVEL immediately — any further commands will reset it
set UBT_EXIT=%ERRORLEVEL%

echo.

if %UBT_EXIT% neq 0 (
    echo ERROR: UBT exited with code %UBT_EXIT%
    echo Check that the project compiles first: Scripts\build.bat
    exit /b %UBT_EXIT%
)

REM UBT writes compile_commands.json to the engine root by default.
REM Copy it to the project root so clangd finds it via --compile-commands-dir.
echo Copying compile_commands.json to project root...
copy /Y "%ENGINE_ROOT%\compile_commands.json" "%~dp0..\compile_commands.json"
set COPY_EXIT=%ERRORLEVEL%

if %COPY_EXIT% neq 0 (
    echo ERROR: Copy failed ^(exit code %COPY_EXIT%^)
    echo Source:      %ENGINE_ROOT%\compile_commands.json
    echo Destination: %~dp0..\compile_commands.json
    echo Verify the source file exists and the project directory is writable.
    exit /b %COPY_EXIT%
)

echo Done. Open the project in VS Code - clangd will index in the background.
echo See docs\clangd-setup.md for VS Code extension setup.
