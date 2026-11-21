# Checks that the tools the project needs are resolvable in the current shell.
# Usage (after sourcing scripts\env.ps1): powershell -NoProfile -File scripts\check-env.ps1
# Exit 0 only if every tool is present; otherwise lists what is missing and exits 1.
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

if ($missing.Count -gt 0) {
  Write-Host ('check-env: FAILED, missing: ' + ($missing -join ', '))
  exit 1
}
Write-Host 'check-env: OK'
exit 0
