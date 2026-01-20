@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
REM ===========================================================================
REM UETPFCore - Launch Editor
REM ===========================================================================
REM Launches Unreal Editor with the project.
REM Usage: editor.bat [args]
REM   All arguments are forwarded to the editor.
REM   Common flags: -log, -game, -windowed
REM ===========================================================================
setlocal

call "%~dp0vars.bat"

echo.
echo ============================================================
echo Launching Unreal Editor
echo ============================================================
echo Project: %PROJECT_FILE%
echo.

start "" "%UE_EDITOR%" "%PROJECT_FILE%" -log %*

endlocal
