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
# PICO_TOOLS_PATHが設定されていなければ、デフォルトのパスを使用
$PicoToolsPath = if ($env:PICO_TOOLS_PATH) { $env:PICO_TOOLS_PATH } else { Join-Path $env:USERPROFILE ".pico-sdk" }

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

$NINJA_VERSION = Get-NinjaVersion
if (-not $NINJA_VERSION) {
    $NINJA_VERSION = if ($env:PICO_NINJA_VERSION) { $env:PICO_NINJA_VERSION } else { "v1.12.1" }
    Write-Warning "Could not auto-detect Ninja version. Using version $NINJA_VERSION"
}

Write-Output "Detected Ninja version: $NINJA_VERSION"
$Ninja = Join-Path $PicoToolsPath "ninja\$NINJA_VERSION\ninja.exe"

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
