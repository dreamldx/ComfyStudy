# Checks that every DLL that comfystudy.exe and the test programs need (directly, or through another
# DLL) is either
#   - a system DLL (it exists in %SystemRoot%\System32, or it is an api-ms-win-* / ext-ms-* name), or
#   - listed in <BinDir>\dlls.manifest, present in <BinDir>, and with the SHA-256 the manifest records.
# Lists every offender with the reason and exits 1; exits 0 when all is well.
# Needs dumpbin (run it from the MSVC environment, e.g. through scripts\dev.ps1).
# Usage: check-dlls.ps1 -BinDir <dir>
param([Parameter(Mandatory)][string]$BinDir)
$ErrorActionPreference = 'Stop'

$manifestFile = Join-Path $BinDir 'dlls.manifest'
if (-not (Test-Path -LiteralPath $manifestFile)) {
  Write-Host "check-dlls: FAILED, $manifestFile does not exist (build the 'deploy' target first)"
  exit 1
}

$manifest = @{}
foreach ($line in [IO.File]::ReadLines($manifestFile)) {
  $f = $line.TrimEnd("`r").Split("`t")
  if ($f.Count -ge 2 -and $f[0]) { $manifest[$f[0].ToLowerInvariant()] = $f[1] }
}

$system32 = Join-Path $env:SystemRoot 'System32'
$problems = New-Object System.Collections.Generic.List[string]
$programs = @(Get-ChildItem -LiteralPath $BinDir -Filter '*.exe' | Where-Object { $_.Name -eq 'comfystudy.exe' -or $_.Name -like 'test_*.exe' })
if ($programs.Count -eq 0) { Write-Host "check-dlls: FAILED, no comfystudy.exe or test_*.exe in $BinDir"; exit 1 }

# Walk the dependencies: the programs first, then every DLL from the manifest that they pull in.
$queue = New-Object System.Collections.Generic.Queue[string]
$seen = @{}
foreach ($program in $programs) { $queue.Enqueue($program.FullName) }
while ($queue.Count -gt 0) {
  $file = $queue.Dequeue()
  $owner = Split-Path -Leaf $file
  $text = (& dumpbin /dependents $file 2>&1 | ForEach-Object { "$_" }) -join "`n"
  if ($LASTEXITCODE -ne 0) { Write-Host "check-dlls: FAILED, dumpbin failed on $owner"; exit 1 }
  $deps = [regex]::Matches($text, '(?m)^\s+(\S+\.dll)\s*$') | ForEach-Object { $_.Groups[1].Value }
  foreach ($dep in $deps) {
    $key = $dep.ToLowerInvariant()
    if ($manifest.ContainsKey($key)) {
      $path = Join-Path $BinDir $dep
      if (-not (Test-Path -LiteralPath $path)) {
        $problems.Add("${owner}: $dep is in the manifest but missing from $BinDir")
      } elseif ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest[$key]) {
        $problems.Add("${owner}: $dep differs from the manifest (run the 'deploy' target again)")
      } elseif (-not $seen.ContainsKey($key)) {
        $seen[$key] = $true
        $queue.Enqueue($path)
      }
    } elseif ($key -like 'api-ms-win-*' -or $key -like 'ext-ms-*' -or (Test-Path -LiteralPath (Join-Path $system32 $dep))) {
      continue
    } else {
      $problems.Add("${owner}: $dep is neither a system DLL nor in the manifest")
    }
  }
}

if ($problems.Count -gt 0) {
  $problems | Sort-Object -Unique | ForEach-Object { Write-Host "check-dlls: $_" }
  Write-Host 'check-dlls: FAILED'
  exit 1
}
Write-Host ("check-dlls: OK ({0} programs, {1} DLLs in the manifest)" -f $programs.Count, $manifest.Count)
exit 0
