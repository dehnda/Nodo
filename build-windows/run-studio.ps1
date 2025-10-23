# Runs nodeflux_studio.exe with the Conan Run Environment so Qt DLLs are found
param(
    [ValidateSet('Release','Debug')]
    [string]$Configuration = 'Release'
)

$root = $PSScriptRoot
$buildDir = Join-Path $PSScriptRoot 'build'
$genDir = Join-Path $buildDir 'generators'
$exe = Join-Path $buildDir 'nodeflux_studio' | Join-Path -ChildPath 'nodeflux_studio.exe'

if (!(Test-Path $exe)) {
    Write-Error "Executable not found: $exe. Build the project first."
    exit 1
}

# Pick the right run env script
$runBat = Join-Path $genDir 'conanrun.bat'
if ($Configuration -eq 'Debug') {
    $debugRun = Join-Path $genDir 'conanrunenv-debug-x86_64.bat'
    if (Test-Path $debugRun) { $runBat = $debugRun }
}

if (!(Test-Path $runBat)) {
    Write-Host "Run environment script not found. Generating VirtualRunEnv..."
    Push-Location $root
    try {
        if ($Configuration -eq 'Debug') {
            conan install . --output-folder=$root -s build_type=Debug -g VirtualRunEnv
        } else {
            conan install . --output-folder=$root -s build_type=Release -g VirtualRunEnv
        }
    } finally {
        Pop-Location
    }
}

if (!(Test-Path $runBat)) {
    Write-Error "Run environment script still not found: $runBat"
    exit 1
}

Write-Host "Launching Studio with Conan Run Env ($Configuration)..."
# Run in a single cmd session so PATH from the .bat applies to the exe
$cmd = '"' + $runBat + '" && "' + $exe + '"'
cmd /c $cmd
