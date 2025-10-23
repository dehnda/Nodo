# Verify Release: Build & Test gate for Windows
# Usage: pwsh -File tools/buildscripts/windows/verify-release.ps1
# Optional switches:
#   -Configuration Release|Debug (default: Release)
#   -WithDeploy (switch)       (default: no deploy/run)
#
# This script:
# 1) Installs Conan deps with tests enabled
# 2) Configures CMake with the Conan toolchain & presets
# 3) Builds and runs tests
# 4) Builds the Studio app (and optionally deploys/runs)

param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [switch]$WithDeploy
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Resolve repo root from script location (tools/buildscripts/windows)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir '..' '..' '..')
Push-Location $RepoRoot
try {
  Write-Host "[*] Conan install ($Configuration) with tests enabled..." -ForegroundColor Cyan
  conan install . --output-folder=build/windows -s build_type=$Configuration -o nodefluxengine/*:with_tests=True --build=missing | Out-Host

  Write-Host "[*] CMake configure (preset: conan-default)..." -ForegroundColor Cyan
  cmake --preset conan-default | Out-Host

  Write-Host "[*] Build and run tests (target: verify-release)..." -ForegroundColor Cyan
  cmake --build --preset "conan-$($Configuration.ToLower())" --target verify-release -- /m:1 | Out-Host

  Write-Host "[*] Build Studio (nodeflux_studio)..." -ForegroundColor Cyan
  cmake --build --preset "conan-$($Configuration.ToLower())" --target nodeflux_studio -- /m:1 | Out-Host

  if ($WithDeploy) {
      Write-Host "[*] Deploying and running Studio (deploy-windows + run-studio-deploy)..." -ForegroundColor Cyan
      cmake --build --preset "conan-$($Configuration.ToLower())" --target deploy-windows -- /m:1 | Out-Host
      cmake --build --preset "conan-$($Configuration.ToLower())" --target run-studio-deploy -- /m:1 | Out-Host
  }

  Write-Host "[✓] Verify release completed." -ForegroundColor Green
}
finally {
  Pop-Location
}
