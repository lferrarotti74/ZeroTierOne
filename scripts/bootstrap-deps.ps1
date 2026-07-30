<#
.SYNOPSIS
    Build & install the pinned, header-only OpenTelemetry C++ API into per-arch
    prefixes (.deps\<arch>) that the windows-* CMake presets consume. The Windows
    analog of scripts/bootstrap-deps.sh (its api-only mode).

.DESCRIPTION
    The Windows build is daemon-only -- the Central Controller is NOT supported on
    Windows -- so it needs only the *header-only* OpenTelemetry API, not the SDK,
    the OTLP exporters, or google-cloud-cpp. The other dependencies come from vcpkg
    (openssl, nlohmann-json; per-triplet) and CMake FetchContent (inja, cpp-httplib,
    miniupnpc/natpmp).

    opentelemetry-cpp's installed CMake package is *arch-stamped* (its config-version
    file checks CMAKE_SIZEOF_VOID_P), so a prefix built for one architecture is
    rejected by a build for another. Each Windows target therefore gets its own
    prefix under .deps\<arch>, and the windows-* presets point at the matching one.

    With no -Arch, all three (x64, Win32, ARM64) are built; pass -Arch to build one.

    Prerequisites: git, CMake, and the Visual Studio 2022 C++ tools for each target
    arch on PATH; vcpkg installed with VCPKG_ROOT set (for the main build).

.PARAMETER Arch
    Target architecture: x64, Win32 (32-bit x86), or ARM64. If omitted, all three are
    built. ARM64 requires the VS ARM64 C++ build tools (its -A ARM64 probe needs them).

.PARAMETER Prefix
    Override the install prefix (default: <repo>\.deps\<Arch>). Only honored when a
    single -Arch is given.

.EXAMPLE
    .\scripts\bootstrap-deps.ps1                 # x64 + Win32 + ARM64
    .\scripts\bootstrap-deps.ps1 -Arch Win32     # just x86  -> cmake --preset windows-x86
#>
[CmdletBinding()]
param(
    [ValidateSet('x64', 'Win32', 'ARM64')]
    [string]$Arch,
    [string]$Prefix
)
$ErrorActionPreference = 'Stop'

# Keep in sync with OTEL_VERSION in scripts/bootstrap-deps.sh.
$OtelVersion = 'v1.27.0'
$RepoRoot  = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildRoot = Join-Path $RepoRoot '.deps-build'
$OtelSrc   = Join-Path $BuildRoot 'opentelemetry-cpp-src'   # arch-independent source clone

function Initialize-OtelSource {
    # Clone (or re-fetch on a tag change) the OTel source. Idempotent; safe to call per arch.
    if (Test-Path (Join-Path $OtelSrc '.git')) {
        $tag = (git -C $OtelSrc describe --tags --exact-match 2>$null)
        if ($tag -ne $OtelVersion) {
            Write-Host "==> opentelemetry-cpp clone is not $OtelVersion; re-fetching"
            Remove-Item -Recurse -Force $OtelSrc
        }
    }
    if (-not (Test-Path (Join-Path $OtelSrc '.git'))) {
        Write-Host "==> Fetching opentelemetry-cpp $OtelVersion"
        git clone --depth 1 --branch $OtelVersion `
            https://github.com/open-telemetry/opentelemetry-cpp.git $OtelSrc
        if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
    }
}

function Build-OtelApi {
    param([Parameter(Mandatory)][string]$TargetArch, [string]$InstallPrefix)
    if (-not $InstallPrefix) { $InstallPrefix = Join-Path $RepoRoot ".deps\$TargetArch" }
    $otelBuild = Join-Path $BuildRoot "otel-$TargetArch"
    New-Item -ItemType Directory -Force -Path $InstallPrefix | Out-Null

    if ((Test-Path (Join-Path $InstallPrefix 'lib\cmake\opentelemetry-cpp')) -or
        (Test-Path (Join-Path $InstallPrefix 'lib64\cmake\opentelemetry-cpp'))) {
        Write-Host "==> [$TargetArch] opentelemetry-cpp already installed, skipping (delete $InstallPrefix to rebuild)"
        return
    }
    Initialize-OtelSource

    # Header-only API: quick, no SDK/exporters. -A stamps the package config for $TargetArch.
    Write-Host "==> [$TargetArch] Building opentelemetry-cpp (API only) -> $InstallPrefix"
    cmake -S $OtelSrc -B $otelBuild -G "Visual Studio 17 2022" -A $TargetArch `
        -DCMAKE_INSTALL_PREFIX="$InstallPrefix" `
        -DBUILD_TESTING=OFF -DOPENTELEMETRY_INSTALL=ON -DWITH_API_ONLY=ON
    if ($LASTEXITCODE -ne 0) { throw "cmake configure (otel) failed" }
    cmake --build $otelBuild --config Release
    if ($LASTEXITCODE -ne 0) { throw "cmake build (otel) failed" }
    cmake --install $otelBuild --config Release
    if ($LASTEXITCODE -ne 0) { throw "cmake install (otel) failed" }
}

# ---- main --------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $BuildRoot | Out-Null
if (-not $env:VCPKG_ROOT) {
    Write-Warning "VCPKG_ROOT is not set. openssl/nlohmann-json come from vcpkg at configure time; set VCPKG_ROOT before running CMake."
}

$targets = if ($Arch) { @($Arch) } else { @('x64', 'Win32', 'ARM64') }
if ($Prefix -and $targets.Count -gt 1) {
    Write-Warning "-Prefix is ignored when building multiple architectures; using .deps\<arch> for each."
    $Prefix = $null
}
Write-Host "==> Bootstrapping pinned deps for: $($targets -join ', ')"

$failed = @()
foreach ($a in $targets) {
    try { Build-OtelApi -TargetArch $a -InstallPrefix $Prefix }
    catch { Write-Warning "[$a] $($_.Exception.Message)"; $failed += $a }
}

$presetOf = @{ 'x64' = 'windows-x64'; 'Win32' = 'windows-x86'; 'ARM64' = 'windows-arm64' }
$built = @($targets | Where-Object { $_ -notin $failed })
Write-Host ""
Write-Host "==> Done. Built: $($built -join ', ')"
foreach ($a in $built) {
    Write-Host ("    {0,-6} -> cmake --preset {1}  ;  cmake --build --preset {1}-release" -f $a, $presetOf[$a])
}
if ($failed.Count -gt 0) {
    Write-Warning ("Failed: " + ($failed -join ', ') + ". (ARM64 needs the Visual Studio ARM64 C++ build tools installed.)")
    if ($failed.Count -eq $targets.Count) { exit 1 }
}
