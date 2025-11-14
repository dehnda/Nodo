# Release Script for Nodo (Windows)
# Usage examples:
#   pwsh -File tools/buildscripts/windows/release.ps1                    # Run tests, build Release, no deploy/package
#   pwsh -File tools/buildscripts/windows/release.ps1 -SkipTests         # Build without tests
#   pwsh -File tools/buildscripts/windows/release.ps1 -Deploy            # Run tests, build, deploy Qt next to exe
#   pwsh -File tools/buildscripts/windows/release.ps1 -Deploy -Run       # Same + start the deployed app
#   pwsh -File tools/buildscripts/windows/release.ps1 -Deploy -Package   # Same + zip the deploy folder
#   pwsh -File tools/buildscripts/windows/release.ps1 -Configuration Debug
#
# Flags:
#   -Configuration Debug|Release (default: Release)
#   -SkipTests (switch)        (default: tests enabled)
#   -Deploy (switch)           (default: no deploy)
#   -Run (switch)              (default: don't run)
#   -Package (switch)          (default: don't package)
#   -OutDir <path>             (default: ./out)
#   -PackageName <string>      (default: NodoStudio-<Config>-<yyyyMMdd-HHmm>)

[CmdletBinding()]
param(
  [ValidateSet('Debug','Release')]
  [string]$Configuration = 'Release',
  [switch]$SkipTests,
  [switch]$Deploy,
  [switch]$Run,
  [switch]$Package,
  [string]$OutDir = './out',
  [string]$PackageName
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Resolve repo root (this script is at tools/buildscripts/windows)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir '..' '..' '..')
Push-Location $RepoRoot
try {
  $configLower = $Configuration.ToLower()
  $buildPreset = "conan-$configLower"

  # 1) Conan install (toggle tests via recipe option)
  if ($SkipTests) {
    Write-Host "[*] Conan install ($Configuration) with tests DISABLED" -ForegroundColor Cyan
    conan install . --output-folder=build/windows -s build_type=$Configuration --build=missing | Out-Host
  } else {
    Write-Host "[*] Conan install ($Configuration) with tests ENABLED" -ForegroundColor Cyan
    conan install . --output-folder=build/windows -s build_type=$Configuration -o nodo/*:with_tests=True --build=missing | Out-Host
  }

  # 2) Configure CMake (always via preset)
  Write-Host "[*] CMake configure (preset: conan-default)" -ForegroundColor Cyan
  cmake --preset conan-default | Out-Host

  # 3) Verify (tests) if not skipped
  if (-not $SkipTests) {
    Write-Host "[*] Building and running tests (target: verify-release)" -ForegroundColor Cyan
    cmake --build --preset $buildPreset --target verify-release -- /m:1 | Out-Host
  } else {
    Write-Host "[!] Skipping tests (user request)" -ForegroundColor Yellow
  }

  # 4) Build main app
  Write-Host "[*] Building nodo_studio ($Configuration)" -ForegroundColor Cyan
  cmake --build --preset $buildPreset --target nodo_studio -- /m:1 | Out-Host

  # 5) Deploy (optional)
  $deployDir = Join-Path -Path $RepoRoot -ChildPath "build/windows/build/nodo_studio/$Configuration/deploy/$Configuration"
  if ($Deploy) {
    Write-Host "[*] Deploying Qt runtime (target: deploy-windows)" -ForegroundColor Cyan
    cmake --build --preset $buildPreset --target deploy-windows -- /m:1 | Out-Host

    if ($Run) {
      Write-Host "[*] Launching deployed app (target: run-studio-deploy)" -ForegroundColor Cyan
      cmake --build --preset $buildPreset --target run-studio-deploy -- /m:1 | Out-Host
    }
  } elseif ($Run) {
    # Run via Conan VirtualRunEnv wrapper target
    Write-Host "[*] Launching app via RunEnv (target: run-studio)" -ForegroundColor Cyan
    cmake --build --preset $buildPreset --target run-studio -- /m:1 | Out-Host
  }

  # 6) Package deployed output (zip)
  if ($Package) {
    if (-not (Test-Path $deployDir)) {
      throw "Deploy directory not found: $deployDir. Run with -Deploy first or pass -Deploy along with -Package."
    }
    if (-not $PackageName) {
      $stamp = Get-Date -Format 'yyyyMMdd-HHmm'
      $PackageName = "NodoStudio-$Configuration-$stamp"
    }
    if (-not (Test-Path $OutDir)) {
      New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    }
    $zipPath = Join-Path -Path (Resolve-Path $OutDir) -ChildPath ("$PackageName.zip")
    if (Test-Path $zipPath) { Remove-Item -Force $zipPath }

    Write-Host "[*] Packaging deploy folder -> $zipPath" -ForegroundColor Cyan
    Compress-Archive -Path (Join-Path $deployDir '*') -DestinationPath $zipPath -Force
    Write-Host "[✓] Package created: $zipPath" -ForegroundColor Green
  }

  Write-Host "[✓] Release script completed." -ForegroundColor Green
}
finally {
  Pop-Location
}
