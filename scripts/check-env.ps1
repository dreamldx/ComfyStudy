# Checks that the tools the project needs are resolvable in the current shell.
# Usage (after sourcing scripts\env.ps1): powershell -NoProfile -File scripts\check-env.ps1
# Exit 0 only if every tool is present; otherwise lists what is missing and exits 1.
# With -Gpu it also asks the NVIDIA driver whether the GPU can be used (tools\gpu-probe).
param([switch]$Gpu)
$ErrorActionPreference = 'Continue'

$missing = New-Object System.Collections.Generic.List[string]

function First-Line([string]$command, [string[]]$arguments) {
  $out = & $command @arguments 2>&1 | ForEach-Object { "$_" } | Where-Object { $_.Trim() -ne '' } | Select-Object -First 1
  if ($null -eq $out) { return '' }
  return $out.Trim()
}

function Check-Tool([string]$name, [scriptblock]$describe) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if ($null -eq $cmd) {
    Write-Host ("MISSING {0}" -f $name)
    $script:missing.Add($name)
    return
  }
  $text = & $describe $cmd
  if ($text -is [string] -and $text.StartsWith('FAIL:')) {
    Write-Host ("MISSING {0}: {1}" -f $name, $text.Substring(5).Trim())
    $script:missing.Add($name)
    return
  }
  Write-Host ("OK      {0}: {1}" -f $name, $text)
}

Check-Tool 'cl' {
  param($cmd)
  $line = First-Line 'cl' @()
  if ($line -notmatch 'Version (\d+)\.') { return 'FAIL: cannot read the compiler version' }
  if ($line -notmatch 'for x64') { return ('FAIL: target is not x64 (' + $line + ')') }
  return $line
}
Check-Tool 'link' { param($cmd) $cmd.Source }
Check-Tool 'dumpbin' { param($cmd) $cmd.Source }
Check-Tool 'cmake' {
  param($cmd)
  $line = First-Line 'cmake' @('--version')
  if ($line -notmatch 'cmake version (\d+)\.(\d+)') { return ('FAIL: cannot read the CMake version (' + $line + ')') }
  $major = [int]$Matches[1]; $minor = [int]$Matches[2]
  if ($major -lt 3 -or ($major -eq 3 -and $minor -lt 25)) { return ('FAIL: CMake 3.25 or newer is required (' + $line + ')') }
  return $line
}
Check-Tool 'ninja' { param($cmd) 'version ' + (First-Line 'ninja' @('--version')) }
Check-Tool 'git' { param($cmd) First-Line 'git' @('--version') }
Check-Tool 'vcpkg' { param($cmd) $cmd.Source }
Check-Tool 'powershell' { param($cmd) $cmd.Source }

# Not a check: the vcpkg that builds the libraries (env.ps1 sets it to the one installed with scoop).
Write-Host ('INFO    VCPKG_ROOT: ' + $(if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { '(not set: run env.ps1)' }))

# -Gpu: the GPU as seen by the NVIDIA driver. nvidia-smi names the card and the driver; tools\gpu-probe
# (configured on its own, so it does not need the project's dependencies) calls the driver the way a
# program would. It needs the driver and cuda.lib from the CUDA toolkit, not nvcc.
if ($Gpu) {
  $smi = Get-Command 'nvidia-smi' -ErrorAction SilentlyContinue
  if ($null -eq $smi) {
    Write-Host 'MISSING nvidia-smi: no NVIDIA driver found'
    $missing.Add('nvidia-smi')
  } else {
    $card = First-Line 'nvidia-smi' @('--query-gpu=name,driver_version', '--format=csv,noheader')
    $cuda = ''
    if ((& nvidia-smi 2>&1 | Out-String) -match 'CUDA (?:UMD )?Version:\s*([0-9]+\.[0-9]+)') { $cuda = $Matches[1] }
    if ($card -eq '' -or $cuda -eq '') {
      Write-Host 'MISSING nvidia-smi: cannot read the GPU name, the driver or the CUDA version'
      $missing.Add('nvidia-smi')
    } else {
      Write-Host ("OK      nvidia-smi: {0}, supports CUDA {1}" -f $card, $cuda)
      $probeSource = Join-Path (Split-Path -Parent $PSScriptRoot) 'tools\gpu-probe'
      $probeBuild = Join-Path ([IO.Path]::GetTempPath()) ('gpu-probe-' + [Guid]::NewGuid().ToString('N'))
      $step = 'configure'
      $out = & cmake -S $probeSource -B $probeBuild -G Ninja 2>&1 | Out-String
      if ($LASTEXITCODE -eq 0 -and $out -notmatch 'no CUDA driver library found') {
        $step = 'build'
        $out = & cmake --build $probeBuild 2>&1 | Out-String
        if ($LASTEXITCODE -eq 0) {
          $step = 'run'
          $out = & (Join-Path $probeBuild 'gpu_probe.exe') 2>&1 | Out-String
        }
      }
      if ($step -eq 'run' -and $LASTEXITCODE -eq 0) {
        $device = ($out -split "`r?`n" | Where-Object { $_ -like 'device 0:*' } | Select-Object -First 1) -replace '^device 0:\s*', ''
        Write-Host ("OK      gpu: {0}, context created" -f $device)
      } else {
        $tail = ($out -split "`r?`n" | Where-Object { $_.Trim() -ne '' } | Select-Object -Last 3) -join ' | '
        Write-Host ("MISSING gpu: {0} failed: {1}" -f $step, $tail)
        $missing.Add('gpu')
      }
      Remove-Item -LiteralPath $probeBuild -Recurse -Force -ErrorAction SilentlyContinue
    }
  }
}

if ($missing.Count -gt 0) {
  Write-Host ('check-env: FAILED, missing: ' + ($missing -join ', '))
  exit 1
}
Write-Host 'check-env: OK'
exit 0
