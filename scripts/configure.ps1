# Configure script for Pico2ROMEmuBR
# Usage: ./scripts/configure.ps1 [-Clean]
param(
    [switch]$Clean
)

$Root = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $Root\..\

$CMake = "$env:USERPROFILE\.pico-sdk\cmake\$env:PICO_CMAKE_VERSION\bin\cmake.exe"
$Ninja = "$env:USERPROFILE/.pico-sdk/ninja/$env:PICO_NINJA_VERSION/ninja.exe"

if ($Clean) {
    Write-Output "Removing build directory..."
    if (Test-Path build) { Remove-Item -Recurse -Force build }
}

Write-Output "Running CMake configure (Generator: Ninja)..."
& $CMake -S . -B build -G Ninja -D CMAKE_MAKE_PROGRAM="$Ninja"

if ($LASTEXITCODE -eq 0) {
    Write-Output "CMake configure succeeded. You can build with:
    $Ninja -C build"
} else {
    Write-Error "CMake configure failed with exit code $LASTEXITCODE"
}
