@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
REM ===========================================================================
REM UETPFCore - Build Editor Target
REM ===========================================================================
REM Builds the SinglePlayerStoryTemplateEditor target for development.
REM Usage: build.bat [config]
REM   config: Development (default), DebugGame, Shipping
REM ===========================================================================
setlocal

call "%~dp0vars.bat"

REM Allow config override via argument
if not "%~1"=="" set CONFIG=%~1

echo.
echo ============================================================
echo Building %TARGET_EDITOR% [%PLATFORM%] [%CONFIG%]
echo ============================================================
echo Project: %PROJECT_FILE%
echo.

call "%BUILD_BAT%" %TARGET_EDITOR% %PLATFORM% %CONFIG% "%PROJECT_FILE%" -waitmutex -NoHotReload

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] Build completed successfully.
endlocal
