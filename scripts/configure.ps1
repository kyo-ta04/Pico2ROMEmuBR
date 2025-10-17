# Configure script for Pico2ROMEmuBR
# Usage: ./scripts/configure.ps1 [-Clean]
param(
    [switch]$Clean
)

$Root = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $Root\..\

# PICO_TOOLS_PATHが設定されていなければ、デフォルトのパスを使用
$PicoToolsPath = if ($env:PICO_TOOLS_PATH) { $env:PICO_TOOLS_PATH } else { Join-Path $env:USERPROFILE ".pico-sdk" }
Write-Output "Using PICO tools path: $PicoToolsPath"

# CMakeバージョンの自動検出
function Get-CMakeVersion {
    $cmakePath = Join-Path $PicoToolsPath "cmake"
    if (Test-Path $cmakePath) {
        $versions = Get-ChildItem -Path $cmakePath -Directory | 
                   Where-Object { $_.Name -match "^v?\d+\.\d+\.\d+$" } |
                   Sort-Object { [Version]($_.Name -replace '^v') } -Descending
        if ($versions.Count -gt 0) {
            return $versions[0].Name
        }
    }
    return $null
}

# Ninjaバージョンの自動検出
function Get-NinjaVersion {
    $ninjaPath = Join-Path $PicoToolsPath "ninja"
    if (Test-Path $ninjaPath) {
        $versions = Get-ChildItem -Path $ninjaPath -Directory |
                   Where-Object { $_.Name -match "^v\d+\.\d+\.\d+$" } |
                   Sort-Object Name -Descending
        if ($versions.Count -gt 0) {
            return $versions[0].Name
        }
    }
    return $null
}

# Get versions
$CMAKE_VERSION = Get-CMakeVersion
if (-not $CMAKE_VERSION) {
    $CMAKE_VERSION = if ($env:PICO_CMAKE_VERSION) { $env:PICO_CMAKE_VERSION } else { "3.31.5" }
    Write-Warning "Could not auto-detect CMake version. Using version $CMAKE_VERSION"
}

$NINJA_VERSION = Get-NinjaVersion
if (-not $NINJA_VERSION) {
    $NINJA_VERSION = if ($env:PICO_NINJA_VERSION) { $env:PICO_NINJA_VERSION } else { "v1.12.1" }
    Write-Warning "Could not auto-detect Ninja version. Using version $NINJA_VERSION"
}

Write-Output "Detected versions: CMake=$CMAKE_VERSION, Ninja=$NINJA_VERSION"

$CMake = Join-Path $PicoToolsPath "cmake\$CMAKE_VERSION\bin\cmake.exe"
$Ninja = Join-Path $PicoToolsPath "ninja\$NINJA_VERSION\ninja.exe"

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
