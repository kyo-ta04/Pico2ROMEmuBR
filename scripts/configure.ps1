# Configure script for Pico2ROMEmuBR
# Usage: ./scripts/configure.ps1 [-Clean]
param(
    [switch]$Clean
)

$Root = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $Root\..\

$CMake = "C:\Users\abu_s\.pico-sdk\cmake\v3.31.5\bin\cmake.exe"
$Ninja = "C:/Users/abu_s/.pico-sdk/ninja/v1.12.1/ninja.exe"

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
