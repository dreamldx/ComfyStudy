# Runs a command inside the MSVC environment, from the repository root.
# Usage (in PowerShell):  .\scripts\dev.ps1 <command> [arguments...]
# Example:                .\scripts\dev.ps1 cmake --preset windows-debug
# The caller's session is left untouched: the environment variables and the working
# directory are restored when the command finishes.
# No declared parameters on purpose: every token, including ones that start with '-',
# belongs to the command.
$ErrorActionPreference = 'Stop'
if ($args.Count -eq 0) {
  Write-Host 'usage: .\scripts\dev.ps1 <command> [arguments...]'
  exit 2
}
$repo = Split-Path -Parent $PSScriptRoot
$savedEnv = [Environment]::GetEnvironmentVariables('Process')
$savedLocation = (Get-Location).Path
$code = 1
try {
  $env:COMFYSTUDY_ENV_QUIET = '1'
  . (Join-Path $PSScriptRoot 'env.ps1')
  Set-Location -LiteralPath $repo
  $command = $args[0]
  $rest = @()
  if ($args.Count -gt 1) { $rest = $args[1..($args.Count - 1)] }
  $global:LASTEXITCODE = 0
  & $command @rest
  $code = $LASTEXITCODE
  if ($null -eq $code) { $code = 0 }
}
finally {
  Set-Location -LiteralPath $savedLocation
  foreach ($name in @([Environment]::GetEnvironmentVariables('Process').Keys)) {
    if (-not $savedEnv.Contains($name)) { [Environment]::SetEnvironmentVariable($name, $null, 'Process') }
  }
  foreach ($name in $savedEnv.Keys) { [Environment]::SetEnvironmentVariable($name, $savedEnv[$name], 'Process') }
}
exit $code
