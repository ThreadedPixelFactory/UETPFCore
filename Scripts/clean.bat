@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
REM ===========================================================================
REM UETPFCore - Clean Build Artifacts
REM ===========================================================================
REM Removes intermediate and binary files for a fresh build.
REM Usage: clean.bat [full]
REM   full: Also removes Saved/ directory (logs, config cache, cooked)
REM ===========================================================================
setlocal

call "%~dp0vars.bat"

echo.
echo ============================================================
echo Cleaning Build Artifacts
echo ============================================================
echo Project: %PROJECT_DIR%
echo.

REM Remove Binaries
if exist "%BINARIES_DIR%" (
    echo Removing Binaries...
    rmdir /s /q "%BINARIES_DIR%"
)

REM Remove Intermediate
if exist "%INTERMEDIATE_DIR%" (
    echo Removing Intermediate...
    rmdir /s /q "%INTERMEDIATE_DIR%"
)

REM Remove DerivedDataCache
if exist "%PROJECT_DIR%\DerivedDataCache" (
    echo Removing DerivedDataCache...
    rmdir /s /q "%PROJECT_DIR%\DerivedDataCache"
)

REM Full clean: also remove Saved
if /i "%~1"=="full" (
    echo.
    echo Full clean requested - removing Saved directory...
    if exist "%SAVED_DIR%" (
        rmdir /s /q "%SAVED_DIR%"
    )
)

echo.
echo [SUCCESS] Clean completed.
echo.
echo NOTE: Run 'build.bat' to rebuild the project.
if not /i "%~1"=="full" (
    echo       Use 'clean.bat full' to also remove Saved/ directory.
)
endlocal
