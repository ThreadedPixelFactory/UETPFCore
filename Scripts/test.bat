@echo off
REM Copyright Threaded Pixel Factory. All Rights Reserved.
REM Licensed under the Apache License, Version 2.0. See LICENSE.txt in the project root for license information.
REM ===========================================================================
REM UETPFCore - Run Automated Tests
REM ===========================================================================
REM Runs the project's automated test suite.
REM Usage: test.bat [filter]
REM   filter: Test filter pattern (e.g., "UETPFCore", "Surface")
REM           Default: runs all project tests
REM ===========================================================================
setlocal

call "%~dp0vars.bat"

set TEST_FILTER=UETPFCore
if not "%~1"=="" set TEST_FILTER=%~1

echo.
echo ============================================================
echo Running Automated Tests [%TEST_FILTER%]
echo ============================================================
echo Project: %PROJECT_FILE%
echo.

"%UE_CMD%" "%PROJECT_FILE%" -ExecCmds="Automation RunTests %TEST_FILTER%;Quit" -unattended -nopause -NullRHI -nosplash -nosound -log

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Tests failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] Tests completed.
endlocal
