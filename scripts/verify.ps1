# Single entry point for verification at any tag (see plan Task 1).
# Stages run in order and only when their files exist, so later tasks add stages
# simply by creating the scripts probed below.
# Usage (in PowerShell):  .\scripts\verify.ps1 [-CheckTags]
# Every stage runs in its own PowerShell process, so your session is left untouched.
param([switch]$CheckTags)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$ran = New-Object System.Collections.Generic.List[string]

function Run-Stage([string]$name, [string]$command) {
  Write-Host "== stage: $name"
  & powershell -NoProfile -ExecutionPolicy Bypass -Command ("`$env:COMFYSTUDY_ENV_QUIET = '1'; " + $command)
  if ($LASTEXITCODE -ne 0) {
    Write-Host "verify: FAILED at stage '$name' (exit $LASTEXITCODE)"
    exit 1
  }
  $script:ran.Add($name)
}

function Have([string]$relative) { Test-Path (Join-Path $repo $relative) }
$envScript = Join-Path $repo 'scripts\env.ps1'

# 1. tutorial documents
$tags = ''
if ($CheckTags) { $tags = ' -CheckTags' }
Run-Stage 'check-docs' ("& '$repo\scripts\check-docs.ps1'$tags; exit `$LASTEXITCODE")

# 2. toolchain and build checks that need the MSVC environment
$withEnv = @(
  @('check-env',    'scripts\check-env.ps1'),
  @('build-basics', 'scripts\check-build.ps1'),
  @('check-vs',     'scripts\check-vs.ps1')
)
foreach ($s in $withEnv) {
  if (Have $s[1]) {
    if (-not (Test-Path $envScript)) { Write-Host "verify: $($s[1]) exists but scripts\env.ps1 is missing"; exit 1 }
    Run-Stage $s[0] (". '$envScript'; & '$repo\$($s[1])'; exit `$LASTEXITCODE")
  }
}

# 3. the C++ tests (from chapter 0.2 on): configure, build and run the debug preset
if (Have 'tests\CMakeLists.txt') {
  $dev = "& '$repo\scripts\dev.ps1'"
  $step = "; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE }; "
  Run-Stage 'ctest' ("$dev cmake --preset windows-debug" + $step + "$dev cmake --build --preset windows-debug" + $step + "$dev ctest --preset windows-debug; exit `$LASTEXITCODE")
}

Write-Host ('verify: OK, stages: ' + ($ran -join ', '))
exit 0
