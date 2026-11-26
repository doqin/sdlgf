<#
 Assembles a fully portable folder + zip of the demo (no installs needed on the user's PC).
 Bundles: everything the CMake build puts next to demo.exe (demo.exe, D3DX9_43.dll, assets),
 plus app-local VC++ runtime DLLs.
 Run after: cmake -S . -B build -A x64 ; cmake --build build --config Release
#>
param(
    [string]$OutDir   = "dist/DX9GF-Demo",
    [string]$BuildDir = "build/bin/Release"
)
$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $root

if (-not (Test-Path (Join-Path $BuildDir "demo.exe"))) { throw "demo.exe not found in $BuildDir - build Release first." }

Remove-Item $OutDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $OutDir | Out-Null

# 1. game exe, D3DX9_43.dll and assets, exactly as the CMake build lays them out
#    (the game loads "assets/..." relative to the exe folder)
Copy-Item (Join-Path $BuildDir "*") $OutDir -Recurse

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
