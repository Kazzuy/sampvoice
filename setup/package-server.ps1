<#
.SYNOPSIS
  Builds the SampVoice server side and assembles install packages in dist/.
.DESCRIPTION
  Packages: dist/sampvoice-server-omp (open.mp), dist/sampvoice-server-samp (SA:MP 0.3.7 R2).
  Layout contract: specs/002-server-plugin-inc-build/contracts/package-layout.md
#>
[CmdletBinding()]
param(
    [string]$OmpDepsDir = $env:OMP_DEPS_DIR,
    [string]$PawncPath,
    [switch]$SkipPawn,
    [ValidateSet('omp', 'samp', 'all')][string]$Platform = 'all'
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Dist = Join-Path $Root 'dist'
$Forbidden = '.pdb', '.ipdb', '.iobj', '.exp', '.lib', '.obj', '.tlog', '.user'
$TextExt = '.md', '.cfg', '.txt', '.inc'

function Find-VsInstall {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found (Visual Studio Installer missing)" }
    $p = & $vswhere -all -products '*' -latest -property installationPath
    if (-not $p) { throw "Visual Studio / Build Tools 2022 not found" }
    return $p
}

function Test-Prerequisites {
    $vs = Find-VsInstall
    $script:MSBuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path $script:MSBuild)) { throw "MSBuild.exe not found under $vs" }
    if (-not (Test-Path (Join-Path $vs 'MSBuild\Microsoft\VC\v150\Platforms\Win32\PlatformToolsets\v141_xp'))) {
        throw "toolset v141_xp not installed (components VC.v141.x86.x64 and WinXP)"
    }
    if ($Platform -in 'omp', 'all') {
        $cm = Get-Command cmake.exe -ErrorAction SilentlyContinue
        if ($cm) { $script:CMake = $cm.Source }
        else {
            $script:CMake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (-not (Test-Path $script:CMake)) { throw "cmake.exe not found (PATH or Build Tools)" }
        }
        foreach ($d in 'open.mp-sdk', 'open.mp-network') {
            if (-not $OmpDepsDir -or -not (Test-Path (Join-Path $OmpDepsDir $d))) {
                throw "missing '$d' under -OmpDepsDir/OMP_DEPS_DIR ('$OmpDepsDir'); pinned commits: open.mp-sdk 08cc69a, open.mp-network f223fbd"
            }
        }
    }
    if (-not $SkipPawn -and (-not $PawncPath -or -not (Test-Path $PawncPath))) {
        throw "pawncc.exe not found: pass -PawncPath <pawncc.exe> or -SkipPawn"
    }
}

function Invoke-Checked([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$([IO.Path]::GetFileName($Exe)) failed with exit code $LASTEXITCODE" }
}

function Build-Project([string]$Proj, [string]$Output) {
    Invoke-Checked $script:MSBuild @((Join-Path $Root $Proj), '/p:Configuration=Release', '/p:Platform=Win32', '/m', '/nologo', '/v:minimal')
    $out = Join-Path $Root $Output
    if (-not (Test-Path $out)) { throw "$Output missing after build" }
}

function Build-ControlOmp {
    $build = Join-Path $env:LOCALAPPDATA 'sampvoice_build_omp'
    # Fresh tree every time: CXXFLAGS is read only at first configure. /pathmap keeps local paths out of __FILE__ strings.
    if (Test-Path $build) { Remove-Item $build -Recurse -Force }
    $prevFlags = $env:CXXFLAGS
    $maps = foreach ($pair in @(@($OmpDepsDir.TrimEnd('\'), '/omp-deps'), @($Root, '/sampvoice'))) {
        "/pathmap:$($pair[0])=$($pair[1])"
        "/pathmap:$($pair[0].Replace('\', '/'))=$($pair[1])"
    }
    $env:CXXFLAGS = '/experimental:deterministic ' + ($maps -join ' ')
    Invoke-Checked $script:CMake @('-S', (Join-Path $Root 'control\omp'), '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'Win32', "-DOMP_DEPS_DIR=$OmpDepsDir")
    Invoke-Checked $script:CMake @('--build', $build, '--config', 'Release')
    $env:CXXFLAGS = $prevFlags
    $dll = Get-ChildItem $build -Recurse -Filter SampVoiceControl.dll | Select-Object -First 1
    if (-not $dll) { throw "SampVoiceControl.dll not produced" }
    return $dll.FullName
}

function Get-Sha256([string]$Path) { (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant() }

function New-Package([string]$Name, [hashtable]$Layout) {
    $dir = Join-Path $Dist $Name
    if (Test-Path $dir) { Remove-Item $dir -Recurse -Force }
    foreach ($target in $Layout.Keys) {
        $src = $Layout[$target]
        if (-not (Test-Path $src)) { throw "package '$Name': source missing for ${target}: $src" }
        $dest = Join-Path $dir $target
        New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
        Copy-Item $src $dest
    }
    return $dir
}

function Write-Manifest([string]$Dir) {
    $files = Get-ChildItem $Dir -Recurse -File | Where-Object { $_.Name -ne 'MANIFEST.txt' }
    $lines = $files | ForEach-Object {
        $rel = $_.FullName.Substring($Dir.Length + 1).Replace('\', '/')
        "$rel  $(Get-Sha256 $_.FullName)"
    } | Sort-Object { $_ } -CaseSensitive
    [IO.File]::WriteAllText((Join-Path $Dir 'MANIFEST.txt'), (($lines -join "`n") + "`n"))
}

function Test-BinaryPaths([string]$File, [string]$Rel) {
    $bytes = [IO.File]::ReadAllBytes($File)
    $ascii = [Text.Encoding]::ASCII.GetString($bytes)
    $utf16 = [Text.Encoding]::Unicode.GetString($bytes)
    $leaks = @($Root, $env:USERPROFILE) + $(if ($OmpDepsDir -and (Test-Path $OmpDepsDir)) { (Resolve-Path $OmpDepsDir).Path })
    foreach ($leak in $leaks) {
        foreach ($v in $leak, $leak.Replace('\', '/')) {
            foreach ($hay in $ascii, $utf16) {
                if ($hay.IndexOf($v, [StringComparison]::OrdinalIgnoreCase) -ge 0) { throw "$Rel contains local path '$v'" }
            }
        }
    }
}

function Test-IncludeIdentity([string]$Dir) {
    foreach ($f in 'sampvoice.inc', 'sampvoice.hnd') {
        if ((Get-Sha256 (Join-Path $Dir "include\$f")) -ne (Get-Sha256 (Join-Path $Root "documents\$f"))) {
            throw "include\$f differs from documents\$f"
        }
    }
}

function Test-IncludeCoverage([string]$Dir) {
    $inc = Get-Content (Join-Path $Root 'documents\sampvoice.inc') -Raw
    # Comments do not count as calls.
    $test = (Get-Content (Join-Path $Root 'tests\pawn\svcoverage.pwn') -Raw) -replace '(?s)/\*.*?\*/', '' -replace '(?m)//.*$', ''
    $names = [regex]::Matches($inc, '(?m)^native\s+[^\s:]+:\s*(\w+)\(') | ForEach-Object { $_.Groups[1].Value }
    $enum = [regex]::Match($inc, 'enum\s+SV_FILTER\s*\{([^}]*)\}').Groups[1].Value
    $names += [regex]::Matches($enum, '\bSV_\w+') | ForEach-Object { $_.Value }
    $missing = $names | Where-Object { $test -notmatch "\b$_\b" }
    if ($missing) { throw "tests\pawn\svcoverage.pwn does not cover: $($missing -join ', ')" }
    $amx = Join-Path $env:TEMP 'svcoverage.amx'
    $prev = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $out = & $PawncPath (Join-Path $Root 'tests\pawn\svcoverage.pwn') "-i$Dir\include" "-i$(Join-Path (Split-Path $PawncPath) 'include')" "-o$amx" 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE; $ErrorActionPreference = $prev
    if ($code -ne 0) { throw "pawncc failed:`n$($out -join "`n")" }
    if ($out -match 'sampvoice\.inc.*(warning|error)') { throw "pawncc reported problems in sampvoice.inc:`n$($out -join "`n")" }
}

function Test-Package([string]$Dir, [string[]]$Expected) {
    $files = Get-ChildItem $Dir -Recurse -File
    foreach ($f in $files) {
        if ($Forbidden -contains $f.Extension.ToLowerInvariant()) { throw "forbidden file in package: $($f.Name)" }
        $rel = $f.FullName.Substring($Dir.Length + 1).Replace('\', '/')
        if ($rel -ne 'MANIFEST.txt' -and $Expected -notcontains $rel) { throw "unexpected file in package: $rel" }
        if ('.dll', '.exe' -contains $f.Extension.ToLowerInvariant()) { Test-BinaryPaths $f.FullName $rel }
        if ($TextExt -contains $f.Extension.ToLowerInvariant()) {
            $t = Get-Content $f.FullName -Raw
            foreach ($leak in $Root, $env:USERPROFILE) {
                if ($t -and $t.IndexOf($leak, [StringComparison]::OrdinalIgnoreCase) -ge 0) { throw "$rel contains local path '$leak'" }
            }
        }
    }
    Test-IncludeIdentity $Dir
    if ($SkipPawn) { $script:NotVerified = $true } else { Test-IncludeCoverage $Dir }
}

function Get-Layout([string]$Plat, [string]$ControlDll) {
    $l = [ordered]@{
        'control.cfg'          = Join-Path $Root 'control\control.cfg'
        'voice.cfg'            = Join-Path $Root 'voice\voice.cfg'
        'include/sampvoice.inc' = Join-Path $Root 'documents\sampvoice.inc'
        'include/sampvoice.hnd' = Join-Path $Root 'documents\sampvoice.hnd'
        'INSTALL-SERVER.md'    = Join-Path $Root 'documents\INSTALL-SERVER.md'
    }
    if ($Plat -eq 'omp') { $l['components/SampVoiceControl.dll'] = $ControlDll }
    else
    {
        $l['plugins/sampvoice.dll'] = $ControlDll
        $l['sampvoice_voice.exe'] = Join-Path $Root 'voice\binaries\sampvoice.exe'
    }
    return $l
}

# ---- main ----
$script:NotVerified = $false
Test-Prerequisites

$plats = if ($Platform -eq 'all') { 'omp', 'samp' } else { @($Platform) }
# The open.mp component embeds the relay; only the SA:MP package ships the separate voice executable.
if ('samp' -in $plats) { Build-Project 'voice\voice.vcxproj' 'voice\binaries\sampvoice.exe' }
$failed = $false
$ompDll = $null; $sampDll = $null
if ('samp' -in $plats) { Build-Project 'control\control.vcxproj' 'control\binaries\sampvoice.dll'; $sampDll = Join-Path $Root 'control\binaries\sampvoice.dll' }
if ('omp' -in $plats) { $ompDll = Build-ControlOmp }

foreach ($p in $plats) {
    $name = "sampvoice-server-$p"
    try {
        $layout = Get-Layout $p $(if ($p -eq 'omp') { $ompDll } else { $sampDll })
        $dir = New-Package $name $layout
        Test-Package $dir @($layout.Keys)
        Write-Manifest $dir
        Write-Host "PASS  $name"
    }
    catch { $failed = $true; Write-Host "FAIL  ${name}: $($_.Exception.Message)" -ForegroundColor Red }
}
if ($script:NotVerified) { Write-Host 'NOT VERIFIED: FR-004, SC-002 (-SkipPawn)' -ForegroundColor Yellow }
if ($failed) { exit 1 }
