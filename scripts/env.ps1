# Puts the current PowerShell session into the MSVC x64 build environment.
# Dot-source it:  . .\scripts\env.ps1
# It locates Visual Studio (vswhere; fallback: $env:COMFYSTUDY_VS_PATH, then the default
# install path), runs vcvars64.bat in a cmd child and imports the resulting variables.
# Safe to source more than once.

if ($env:VSCMD_VER -and (Get-Command cl -ErrorAction SilentlyContinue)) {
  if ($env:COMFYSTUDY_ENV_QUIET -ne '1') { Write-Host 'MSVC x64 environment is already active in this session.' }
  return
}

function Find-VsInstall {
  if ($env:COMFYSTUDY_VS_PATH -and (Test-Path -LiteralPath $env:COMFYSTUDY_VS_PATH)) {
    return $env:COMFYSTUDY_VS_PATH
  }
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path -LiteralPath $vswhere) {
    $found = & $vswhere -latest -products '*' -property installationPath 2>$null | Select-Object -First 1
    if ($found -and (Test-Path -LiteralPath $found)) { return $found }
  }
  $default = 'D:\Microsoft Visual Studio\18\Community'
  if (Test-Path -LiteralPath $default) { return $default }
  return $null
}

$vsInstall = Find-VsInstall
if (-not $vsInstall) {
  throw 'Visual Studio was not found. Install it, or set COMFYSTUDY_VS_PATH to its install directory.'
}
$vcvars = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path -LiteralPath $vcvars)) {
  throw "vcvars64.bat not found under $vsInstall (is the C++ workload installed?)."
}

# vcvars64.bat looks for vswhere on PATH; give the child process the Installer directory.
$installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
$savedPath = $env:PATH
$env:PATH = $env:PATH + ';' + $installer
try {
  $lines = & cmd.exe /d /c ('call "' + $vcvars + '" >nul 2>&1 && set')
}
finally {
  $env:PATH = $savedPath
}
if ($LASTEXITCODE -ne 0) { throw "vcvars64.bat failed (exit $LASTEXITCODE)." }

foreach ($line in $lines) {
  $i = $line.IndexOf('=')
  if ($i -le 0) { continue }
  $name = $line.Substring(0, $i)
  $value = $line.Substring($i + 1)
  [Environment]::SetEnvironmentVariable($name, $value, 'Process')
}

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
  throw 'cl.exe is still not resolvable after running vcvars64.bat.'
}

if ($env:COMFYSTUDY_ENV_QUIET -ne '1') {
  $banner = @(& cl 2>&1 | ForEach-Object { "$_" } | Where-Object { $_ -match 'Optimizing Compiler' }) | Select-Object -First 1
  Write-Host ('MSVC x64 environment ready: ' + "$banner".Trim())
  Write-Host 'It applies to this PowerShell session only: run your cl/link/cmake commands here, in the same session.'
}
