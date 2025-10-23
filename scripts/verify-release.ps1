Write-Host "[DEPRECATED] This script moved to tools/buildscripts/windows/verify-release.ps1" -ForegroundColor Yellow
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$NewScript = Join-Path $ScriptDir "../tools/buildscripts/windows/verify-release.ps1"
if (Test-Path $NewScript) {
    Write-Host "Forwarding to: $NewScript" -ForegroundColor Yellow
    & $NewScript @args
    exit $LASTEXITCODE
} else {
    Write-Host "New script not found at $NewScript. Please pull latest or check repository." -ForegroundColor Red
    exit 1
}
