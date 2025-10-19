# Windows Build Script for NodeFluxEngine
# This script automates the build process on Windows
# Run from "Developer PowerShell for VS 2022"

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",

    [Parameter(Mandatory=$false)]
    [switch]$Clean,

    [Parameter(Mandatory=$false)]
    [switch]$RunTests,

    [Parameter(Mandatory=$false)]
    [string]$Target = "all"
)

# Configuration
$ErrorActionPreference = "Stop"
$BuildDir = "build-windows"
$PresetName = "conan-$($BuildType.ToLower())"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "NodeFluxEngine Windows Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow
Write-Host "Preset: $PresetName" -ForegroundColor Yellow
Write-Host ""

# Check prerequisites
Write-Host "Checking prerequisites..." -ForegroundColor Green

# Check CMake
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "[OK] CMake: $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] CMake not found. Please install CMake." -ForegroundColor Red
    exit 1
}

# Check Conan
try {
    $conanVersion = conan --version
    Write-Host "[OK] Conan: $conanVersion" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Conan not found. Please install: pip install conan" -ForegroundColor Red
    exit 1
}

# Check if we're in a VS Developer environment
if (-not $env:VSINSTALLDIR) {
    Write-Host "[WARNING] Not in Visual Studio Developer environment." -ForegroundColor Yellow
    Write-Host "          Please run from 'Developer PowerShell for VS 2022'" -ForegroundColor Yellow
    Write-Host ""
}

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning build directories..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "[OK] Removed $BuildDir" -ForegroundColor Green
    }
    if (Test-Path "out") {
        Remove-Item -Recurse -Force "out"
        Write-Host "[OK] Removed out/" -ForegroundColor Green
    }
    if (Test-Path "CMakePresets.json") {
        Remove-Item "CMakePresets.json"
        Write-Host "[OK] Removed CMakePresets.json" -ForegroundColor Green
    }
    Write-Host ""
}

# Step 1: Install Conan dependencies
Write-Host "Step 1: Installing Conan dependencies..." -ForegroundColor Green
Write-Host "This may take 15-30 minutes on first run..." -ForegroundColor Yellow

$conanArgs = @(
    "install", ".",
    "--output-folder=$BuildDir",
    "--build=missing",
    "-s", "build_type=$BuildType"
)

Write-Host "Running: conan $($conanArgs -join ' ')" -ForegroundColor Cyan
& conan @conanArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Conan install failed" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Conan dependencies installed" -ForegroundColor Green
Write-Host ""

# Step 2: Configure CMake
Write-Host "Step 2: Configuring CMake..." -ForegroundColor Green
Write-Host "Running: cmake --preset $PresetName" -ForegroundColor Cyan

& cmake --preset $PresetName

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake configuration failed" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] CMake configured" -ForegroundColor Green
Write-Host ""

# Step 3: Build
Write-Host "Step 3: Building project..." -ForegroundColor Green

if ($Target -eq "all") {
    Write-Host "Running: cmake --build --preset $PresetName --parallel" -ForegroundColor Cyan
    & cmake --build --preset $PresetName --parallel
} else {
    Write-Host "Running: cmake --build --preset $PresetName --target $Target --parallel" -ForegroundColor Cyan
    & cmake --build --preset $PresetName --target $Target --parallel
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Build completed successfully" -ForegroundColor Green
Write-Host ""

# Step 4: Create compile_commands.json symlink
Write-Host "Step 4: Creating compile_commands.json symlink..." -ForegroundColor Green
$sourceFile = "out\build\$PresetName\compile_commands.json"
$targetFile = "compile_commands.json"

if (Test-Path $sourceFile) {
    if (Test-Path $targetFile) {
        Remove-Item $targetFile -Force
    }
    # Create a junction/symlink (requires admin) or just copy
    try {
        New-Item -ItemType SymbolicLink -Path $targetFile -Target $sourceFile -Force | Out-Null
        Write-Host "[OK] Created symlink to compile_commands.json" -ForegroundColor Green
    } catch {
        Copy-Item $sourceFile $targetFile -Force
        Write-Host "[OK] Copied compile_commands.json (symlink requires admin)" -ForegroundColor Yellow
    }
} else {
    Write-Host "[WARNING] compile_commands.json not found" -ForegroundColor Yellow
}
Write-Host ""

# Step 5: Run tests if requested
if ($RunTests) {
    Write-Host "Step 5: Running tests..." -ForegroundColor Green
    $testExe = "out\build\$PresetName\tests\nodeflux_tests.exe"

    if (Test-Path $testExe) {
        & $testExe
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] Tests failed" -ForegroundColor Red
            exit 1
        }
        Write-Host "[OK] All tests passed" -ForegroundColor Green
    } else {
        Write-Host "[WARNING] Test executable not found at $testExe" -ForegroundColor Yellow
    }
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Type: $BuildType" -ForegroundColor Green
Write-Host "Binary Directory: out\build\$PresetName" -ForegroundColor Green
Write-Host ""
Write-Host "Executables:" -ForegroundColor Yellow

$studioExe = "out\build\$PresetName\nodeflux_studio\nodeflux_studio.exe"
$testExe = "out\build\$PresetName\tests\nodeflux_tests.exe"

if (Test-Path $studioExe) {
    Write-Host "  Studio: $studioExe" -ForegroundColor Green
} else {
    Write-Host "  Studio: Not built (check CMake options)" -ForegroundColor Yellow
}

if (Test-Path $testExe) {
    Write-Host "  Tests:  $testExe" -ForegroundColor Green
} else {
    Write-Host "  Tests:  Not built (check CMake options)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host ""

# Usage examples
Write-Host "Usage examples:" -ForegroundColor Cyan
Write-Host "  # Clean and rebuild in Release mode" -ForegroundColor Gray
Write-Host "  .\build-windows.ps1 -Clean -BuildType Release" -ForegroundColor Gray
Write-Host ""
Write-Host "  # Build in Debug mode and run tests" -ForegroundColor Gray
Write-Host "  .\build-windows.ps1 -BuildType Debug -RunTests" -ForegroundColor Gray
Write-Host ""
Write-Host "  # Build only the Studio application" -ForegroundColor Gray
Write-Host "  .\build-windows.ps1 -Target nodeflux_studio" -ForegroundColor Gray
