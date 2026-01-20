@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
REM ===========================================================================
REM UETPFCore - Shared Build Variables
REM ===========================================================================
REM This file defines all paths and settings used by build scripts.
REM Modify ENGINE_ROOT if your UE installation differs.
REM ===========================================================================

REM --- Project Settings ---
set PROJECT_NAME=UETPFCore
set PROJECT_DIR=%~dp0..
set PROJECT_FILE=%PROJECT_DIR%\%PROJECT_NAME%.uproject

REM --- Engine Settings --- Update location based on your installation
set ENGINE_VERSION=5.7
set ENGINE_ROOT=C:\Program Files\Epic Games\UE_%ENGINE_VERSION%

REM --- Engine Paths ---
set ENGINE_BINARIES=%ENGINE_ROOT%\Engine\Binaries\Win64
set ENGINE_BUILD=%ENGINE_ROOT%\Engine\Build\BatchFiles
set UE_EDITOR=%ENGINE_BINARIES%\UnrealEditor.exe
set UE_CMD=%ENGINE_BINARIES%\UnrealEditor-Cmd.exe
set BUILD_BAT=%ENGINE_BUILD%\Build.bat
set CLEAN_BAT=%ENGINE_BUILD%\Clean.bat
set REBUILD_BAT=%ENGINE_BUILD%\Rebuild.bat

REM --- Build Configuration ---
set PLATFORM=Win64
set CONFIG=Development
set TARGET_GAME=SinglePlayerStoryTemplate
set TARGET_EDITOR=SinglePlayerStoryTemplateEditor

REM --- Output Directories ---
set BINARIES_DIR=%PROJECT_DIR%\Binaries\%PLATFORM%
set INTERMEDIATE_DIR=%PROJECT_DIR%\Intermediate
set SAVED_DIR=%PROJECT_DIR%\Saved
set COOKED_DIR=%SAVED_DIR%\Cooked

REM --- Logging ---
set LOG_DIR=%SAVED_DIR%\Logs
set BUILD_LOG=%LOG_DIR%\build.log
