@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
REM ===========================================================================
REM UETPFCore - Cook Content
REM ===========================================================================
REM Cooks project content for the target platform.
REM Usage: cook.bat [platform]
REM   platform: WindowsNoEditor (default), Windows, LinuxNoEditor
REM ===========================================================================
setlocal

call "%~dp0vars.bat"

set COOK_PLATFORM=WindowsNoEditor
if not "%~1"=="" set COOK_PLATFORM=%~1

echo.
echo ============================================================
echo Cooking Content [%COOK_PLATFORM%]
echo ============================================================
echo Project: %PROJECT_FILE%
echo.

"%UE_CMD%" "%PROJECT_FILE%" -run=cook -targetplatform=%COOK_PLATFORM% -iterate -unversioned

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Cook failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] Content cooked successfully.
echo Output: %COOKED_DIR%\%COOK_PLATFORM%
endlocal
