<#
 Assembles a fully portable folder + zip of the demo (no installs needed on the user's PC).
 Bundles: demo.exe and its assets folder (never the test executables), plus app-local VC++ runtime DLLs.
 Run after: cmake -S . -B build -A x64 ; cmake --build build --config Release
#>
param(
    [string]$OutDir   = "dist/sdlgf-demo",
    [string]$BuildDir = "build/bin/Release"
)
$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $root

if (-not (Test-Path (Join-Path $BuildDir "demo.exe"))) { throw "demo.exe not found in $BuildDir - build Release first." }

Remove-Item $OutDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $OutDir | Out-Null

# 1. game exe and assets, exactly as the CMake build lays them out
#    (the game loads "assets/..." relative to the exe folder).
#    Copied by whitelist, not "*": the test executables share this output directory
#    (sdlgf_*_test.exe) and must never ship in the release zip.
Copy-Item (Join-Path $BuildDir "demo.exe") $OutDir
$assets = Join-Path $BuildDir "assets"
if (-not (Test-Path $assets)) { throw "assets not found in $BuildDir - configure with SDLGF_COPY_ASSETS=ON (the default)." }
Copy-Item $assets (Join-Path $OutDir "assets") -Recurse
# Aseprite sources are artist files, not runtime assets. CMake's copy already skips them; this
# also covers packaging a build tree that was populated before it did.
Get-ChildItem (Join-Path $OutDir "assets") -Recurse -File |
    Where-Object { $_.Extension -in '.ase', '.aseprite' } |
    Remove-Item -Force
# runtime DLLs, should a future build stop linking SDL statically
Get-ChildItem $BuildDir -Filter *.dll -File -ErrorAction SilentlyContinue | Copy-Item -Destination $OutDir

# 2. app-local VC++ runtime (msvcp140, vcruntime140, vcruntime140_1, ...)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -property installationPath
$crt = Get-ChildItem "$vs\VC\Redist\MSVC\*\x64\Microsoft.VC14*.CRT" -Directory |
       Sort-Object FullName | Select-Object -Last 1
if (-not $crt) { throw "VC++ redist CRT folder not found." }
Copy-Item "$($crt.FullName)\*.dll" $OutDir

# 3. zip
$zip = "$OutDir.zip"
Remove-Item $zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$OutDir\*" -DestinationPath $zip
Write-Host "Portable build: $zip"
