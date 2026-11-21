# Asserts the build behaviour of the CMake presets (debug/release, incremental builds, clean).
# Usage (in a PowerShell session where  . .\scripts\env.ps1  was run):  .\scripts\check-build.ps1
# It works from any directory, writes only under build\windows-debug and build\windows-release, and puts
# your working directory back when it finishes. Exit 0 only if every assertion holds.
$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent $PSScriptRoot
$startLocation = (Get-Location).Path
Set-Location -LiteralPath $repo

function Fail([string]$message) { Write-Host ('FAIL: ' + $message); Set-Location -LiteralPath $startLocation; exit 1 }
function Ok([string]$message) { Write-Host ('OK   ' + $message) }

# Runs a tool; returns its exit code and all output lines.
function Run-Tool([string]$tool, [string[]]$toolArgs) {
  $lines = @(& $tool @toolArgs 2>&1 | ForEach-Object { "$_" })
  return [pscustomobject]@{ Code = $LASTEXITCODE; Lines = $lines }
}
function Run-Exe([string]$path) {
  $lines = @(& $path 2>&1 | ForEach-Object { "$_" })
  return [pscustomobject]@{ Code = $LASTEXITCODE; Text = ($lines -join "`n").Trim() }
}
function Build([string]$preset, [string[]]$more) {
  return Run-Tool 'cmake' (@('--build', '--preset', $preset) + $more)
}
function Compile-Lines($lines) { @($lines | Where-Object { $_ -match 'cl\.exe' -and $_ -match '\s-c\s' }) }

$versionHeader = Join-Path $repo 'src\version.h'
$greetingHeader = Join-Path $repo 'src\greeting.h'
$ver = [regex]::Match([IO.File]::ReadAllText($versionHeader), 'COMFYSTUDY_VERSION\s+"([^"]+)"')
if (-not $ver.Success) { Fail 'cannot read COMFYSTUDY_VERSION from src\version.h' }
$expected = 'ComfyStudy ' + $ver.Groups[1].Value

$debugExe = Join-Path $repo 'build\windows-debug\bin\comfystudy.exe'
$releaseExe = Join-Path $repo 'build\windows-release\bin\comfystudy.exe'

foreach ($dir in 'build\windows-debug', 'build\windows-release') {
  if (Test-Path $dir) { Remove-Item -LiteralPath $dir -Recurse -Force }
}

# --- 1. verbose debug build ---------------------------------------------------------------
$r = Run-Tool 'cmake' @('--preset', 'windows-debug')
if ($r.Code -ne 0) { Fail ("cmake --preset windows-debug failed (exit {0}): {1}" -f $r.Code, ($r.Lines -join ' | ')) }
$r = Build 'windows-debug' @('--verbose')
if ($r.Code -ne 0) { Fail ("debug build failed (exit {0}): {1}" -f $r.Code, ($r.Lines -join ' | ')) }
$compile = Compile-Lines $r.Lines
if ($compile.Count -ne 3) { Fail ("expected exactly 3 compile commands (main, greeting, test_greeting), found {0}" -f $compile.Count) }
foreach ($name in 'main.cpp', 'greeting.cpp', 'test_greeting.cpp') {
  $line = @($compile | Where-Object { $_.Contains('\' + $name) })
  if ($line.Count -ne 1) { Fail "expected one compile command for $name" }
  foreach ($flag in '/Fo', '-std:c++20') {
    if (-not $line[0].Contains($flag)) { Fail "compile command for $name lacks '$flag'" }
  }
}
$link = @($r.Lines | Where-Object { $_ -match 'link\.exe' })
if ($link.Count -ne 2) { Fail ("expected exactly 2 link commands (the program and the test), found {0}" -f $link.Count) }
$programLink = @($link | Where-Object { $_.Contains('comfystudy.exe') })
if ($programLink.Count -ne 1) { Fail 'expected one link command for comfystudy.exe' }
foreach ($needle in 'comfystudy_core.lib', 'main.cpp.obj') {
  if (-not $programLink[0].Contains($needle)) { Fail "link command for comfystudy.exe lacks '$needle'" }
}
$testLink = @($link | Where-Object { $_.Contains('test_greeting.exe') })
if ($testLink.Count -ne 1 -or -not $testLink[0].Contains('comfystudy_core.lib')) { Fail 'the test program must link comfystudy_core.lib' }
if (-not (Test-Path $debugExe)) { Fail 'debug executable was not produced' }
$run = Run-Exe $debugExe
if ($run.Code -ne 0 -or $run.Text -ne $expected) { Fail ("debug exe printed '{0}' (exit {1}), expected '{2}'" -f $run.Text, $run.Code, $expected) }
$ct = Run-Tool 'ctest' @('--preset', 'windows-debug')
if ($ct.Code -ne 0 -or -not (($ct.Lines -join "`n") -match '100% tests passed')) { Fail ("ctest failed (exit {0}): {1}" -f $ct.Code, ($ct.Lines -join ' | ')) }
Ok '1 verbose debug build: 3 compile commands, 2 link commands, the program prints the greeting, ctest passes'

# --- 2. nothing to do the second time -------------------------------------------------------
$r = Build 'windows-debug' @()
if ($r.Code -ne 0 -or -not (($r.Lines -join "`n") -match 'no work to do')) { Fail 'a second build must report that there is no work to do' }
Ok '2 second build does nothing'

# --- 3. precise header tracking -------------------------------------------------------------
function Rebuild-After-Edit([string]$header, [int]$expectedObjects, [string]$what) {
  $original = [IO.File]::ReadAllBytes($header)
  try {
    [IO.File]::WriteAllBytes($header, $original + [byte[]](32))
    $r = Build 'windows-debug' @()
    $cxx = @($r.Lines | Where-Object { $_ -match 'Building CXX object' })
    if ($r.Code -ne 0 -or $cxx.Count -ne $expectedObjects) {
      Fail ("{0}: expected {1} object(s) to be rebuilt, saw {2}" -f $what, $expectedObjects, $cxx.Count)
    }
    return $cxx
  }
  finally {
    [IO.File]::WriteAllBytes($header, $original)
    $null = Build 'windows-debug' @()
  }
}
$cxx = @(Rebuild-After-Edit $versionHeader 2 'editing version.h')
$cxxText = $cxx -join "`n"
if ($cxxText -notmatch 'src\\greeting\.cpp' -or $cxxText -notmatch 'test_greeting\.cpp' -or $cxxText -match 'main\.cpp') { Fail 'editing version.h must rebuild greeting.cpp and test_greeting.cpp, but not main.cpp' }
$null = Rebuild-After-Edit $greetingHeader 3 'editing greeting.h'
Ok '3 editing version.h rebuilds greeting.cpp and test_greeting.cpp (not main.cpp), editing greeting.h rebuilds all three objects'

# --- 4. release build is separate ---------------------------------------------------------------
$stamps = @{}
foreach ($o in Get-ChildItem 'build\windows-debug' -Recurse -Filter *.obj) { $stamps[$o.FullName] = $o.LastWriteTimeUtc.Ticks }
$r = Run-Tool 'cmake' @('--preset', 'windows-release')
if ($r.Code -ne 0) { Fail 'cmake --preset windows-release failed' }
$r = Build 'windows-release' @('--verbose')
if ($r.Code -ne 0) { Fail ("release build failed (exit {0})" -f $r.Code) }
$compile = Compile-Lines $r.Lines
if ($compile.Count -ne 3) { Fail 'release build should compile three files' }
foreach ($line in $compile) {
  if (-not $line.Contains('/O2') -or $line -notmatch '-MD(\s|$)') { Fail 'release compile command must have /O2 and -MD' }
}
if (-not (Test-Path $releaseExe)) { Fail 'release executable was not produced' }
$run = Run-Exe $releaseExe
if ($run.Code -ne 0 -or $run.Text -ne $expected) { Fail ("release exe printed '{0}', expected '{1}'" -f $run.Text, $expected) }
foreach ($o in Get-ChildItem 'build\windows-debug' -Recurse -Filter *.obj) {
  if ($stamps[$o.FullName] -ne $o.LastWriteTimeUtc.Ticks) { Fail "release build touched debug object $($o.Name)" }
}
Ok '4 release build in build\windows-release, debug objects untouched'

# --- 5. runtime libraries and symbols ----------------------------------------------------------------
$debugDeps = (& dumpbin /dependents $debugExe 2>&1 | ForEach-Object { "$_" }) -join "`n"
$releaseDeps = (& dumpbin /dependents $releaseExe 2>&1 | ForEach-Object { "$_" }) -join "`n"
if ($debugDeps -notmatch '(?i)VCRUNTIME140D\.dll') { Fail 'debug exe must depend on VCRUNTIME140D.dll' }
if ($releaseDeps -notmatch '(?i)VCRUNTIME140\.dll') { Fail 'release exe must depend on VCRUNTIME140.dll' }
if ($releaseDeps -match '(?i)VCRUNTIME140D\.dll') { Fail 'release exe must not depend on the debug runtime' }
if (@(Get-ChildItem 'build\windows-debug\bin' -Filter *.pdb).Count -eq 0) { Fail 'debug build must produce a .pdb next to the exe' }
if (@(Get-ChildItem 'build\windows-release' -Recurse -Filter *.pdb).Count -ne 0) { Fail 'release build must not produce a .pdb' }
Ok '5 debug uses VCRUNTIME140D and has a .pdb, release uses VCRUNTIME140 and has none'

# --- 6. clean -----------------------------------------------------------------------------------
$r = Build 'windows-debug' @('--target', 'clean')
if ($r.Code -ne 0 -or (Test-Path $debugExe)) { Fail 'the clean target must remove the executable' }
if (@(Get-ChildItem 'build\windows-debug' -Recurse -Filter *.cpp.obj).Count -ne 0) { Fail 'the clean target must remove the project objects' }
Ok '6 clean target removes the executable and the objects'

Write-Host 'check-build: OK'
Set-Location -LiteralPath $startLocation
exit 0
