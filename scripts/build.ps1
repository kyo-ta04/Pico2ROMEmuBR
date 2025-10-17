<#
Build script for Pico2ROMEmuBR
Usage:
  ./scripts/build.ps1          # configure (if needed) and build
  ./scripts/build.ps1 -Clean   # remove build/ then configure and build
  ./scripts/build.ps1 -NoConfigure  # only run ninja build
#>

param(
    [switch]$Clean,
    [switch]$NoConfigure
)

$Root = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $Root\..\

$ConfigureScript = Join-Path $PSScriptRoot "configure.ps1"
$Ninja = "$env:USERPROFILE/.pico-sdk/ninja/$env:PICO_NINJA_VERSION/ninja.exe"

if ($Clean) {
    Write-Output "Cleaning build/ ..."
    if (Test-Path build) { Remove-Item -Recurse -Force build }
}

if (-not $NoConfigure) {
    if (-not (Test-Path $ConfigureScript)) {
        Write-Error "Configure script not found: $ConfigureScript"
        exit 1
    }
    Write-Output "Running configure script..."
    & $ConfigureScript
    if ($LASTEXITCODE -ne 0) { Write-Error "Configure failed"; exit $LASTEXITCODE }
}

if (-not (Test-Path $Ninja)) {
    Write-Error "ninja not found at $Ninja"
    exit 1
}

Write-Output "Building with ninja..."
& $Ninja -C build
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit $LASTEXITCODE }

Write-Output "Build finished successfully."
