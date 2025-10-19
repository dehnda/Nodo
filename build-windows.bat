@echo off
REM Windows Build Script for NodeFluxEngine (Batch version)
REM For users who prefer .bat over PowerShell
REM Run from "Developer Command Prompt for VS 2022"

setlocal enabledelayedexpansion

REM Configuration
set BUILD_TYPE=Release
set BUILD_DIR=build-windows
set CLEAN_BUILD=0

REM Parse arguments
:parse_args
if "%1"=="" goto end_parse
if /i "%1"=="--clean" set CLEAN_BUILD=1
if /i "%1"=="--debug" set BUILD_TYPE=Debug
if /i "%1"=="--release" set BUILD_TYPE=Release
shift
goto parse_args
:end_parse

set PRESET_NAME=conan-%BUILD_TYPE%

echo ========================================
echo NodeFluxEngine Windows Build Script
echo ========================================
echo Build Type: %BUILD_TYPE%
echo Preset: %PRESET_NAME%
echo.

REM Check prerequisites
echo Checking prerequisites...

where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found. Please install CMake.
    exit /b 1
)
echo [OK] CMake found

where conan >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Conan not found. Please install: pip install conan
    exit /b 1
)
echo [OK] Conan found
echo.

REM Clean if requested
if %CLEAN_BUILD%==1 (
    echo Cleaning build directories...
    if exist %BUILD_DIR% (
        rmdir /s /q %BUILD_DIR%
        echo [OK] Removed %BUILD_DIR%
    )
    if exist out (
        rmdir /s /q out
        echo [OK] Removed out/
    )
    if exist CMakePresets.json (
        del CMakePresets.json
        echo [OK] Removed CMakePresets.json
    )
    echo.
)

REM Step 1: Install Conan dependencies
echo Step 1: Installing Conan dependencies...
echo This may take 15-30 minutes on first run...

conan install . --output-folder=%BUILD_DIR% --build=missing -s build_type=%BUILD_TYPE%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Conan install failed
    exit /b 1
)
echo [OK] Conan dependencies installed
echo.

REM Step 2: Configure CMake
echo Step 2: Configuring CMake...

cmake --preset %PRESET_NAME%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)
echo [OK] CMake configured
echo.

REM Step 3: Build
echo Step 3: Building project...

cmake --build --preset %PRESET_NAME% --parallel

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed
    exit /b 1
)
echo [OK] Build completed successfully
echo.

REM Step 4: Create compile_commands.json copy
echo Step 4: Creating compile_commands.json...
if exist out\build\%PRESET_NAME%\compile_commands.json (
    copy /Y out\build\%PRESET_NAME%\compile_commands.json compile_commands.json >nul
    echo [OK] Copied compile_commands.json
) else (
    echo [WARNING] compile_commands.json not found
)
echo.

REM Summary
echo ========================================
echo Build Summary
echo ========================================
echo Build Type: %BUILD_TYPE%
echo Binary Directory: out\build\%PRESET_NAME%
echo.
echo Executables:
if exist out\build\%PRESET_NAME%\nodeflux_studio\nodeflux_studio.exe (
    echo   Studio: out\build\%PRESET_NAME%\nodeflux_studio\nodeflux_studio.exe
) else (
    echo   Studio: Not built
)
if exist out\build\%PRESET_NAME%\tests\nodeflux_tests.exe (
    echo   Tests:  out\build\%PRESET_NAME%\tests\nodeflux_tests.exe
) else (
    echo   Tests:  Not built
)
echo.
echo Build completed successfully!
echo.
echo Usage examples:
echo   # Clean and rebuild in Release mode
echo   build-windows.bat --clean --release
echo.
echo   # Build in Debug mode
echo   build-windows.bat --debug

endlocal
