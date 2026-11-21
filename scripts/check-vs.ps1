# Checks that the Visual Studio preset generates a solution, builds it and that the program runs.
# Usage (in PowerShell, after  . .\scripts\env.ps1 ):  .\scripts\check-vs.ps1
# (the Visual Studio generator itself does not need the environment, but cmake must be on PATH).
# Writes only under build\windows-vs and puts your working directory back when it finishes.
# Exit 0 only if every step works.
$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent $PSScriptRoot
$startLocation = (Get-Location).Path
Set-Location -LiteralPath $repo

function Fail([string]$message) { Write-Host ('FAIL: ' + $message); Set-Location -LiteralPath $startLocation; exit 1 }
function Ok([string]$message) { Write-Host ('OK   ' + $message) }
function Run-Tool([string]$tool, [string[]]$toolArgs) {
  $lines = @(& $tool @toolArgs 2>&1 | ForEach-Object { "$_" })
  return [pscustomobject]@{ Code = $LASTEXITCODE; Lines = $lines }
}

$ver = [regex]::Match([IO.File]::ReadAllText((Join-Path $repo 'src\version.h')), 'COMFYSTUDY_VERSION\s+"([^"]+)"')
if (-not $ver.Success) { Fail 'cannot read COMFYSTUDY_VERSION from src\version.h' }
$expected = 'ComfyStudy ' + $ver.Groups[1].Value

if (Test-Path 'build\windows-vs') { Remove-Item -LiteralPath 'build\windows-vs' -Recurse -Force }

$r = Run-Tool 'cmake' @('--preset', 'windows-vs')
if ($r.Code -ne 0) { Fail ("cmake --preset windows-vs failed (exit {0}): {1}" -f $r.Code, ($r.Lines -join ' | ')) }
# Visual Studio 2026 writes the new .slnx format; older versions write .sln.
$solution = @(Get-ChildItem 'build\windows-vs' -Filter 'ComfyStudy.sln*' | Where-Object { $_.Extension -in '.sln', '.slnx' })
if ($solution.Count -ne 1) { Fail 'expected exactly one solution file (ComfyStudy.sln or ComfyStudy.slnx) in build\windows-vs' }
foreach ($project in 'comfystudy.vcxproj', 'comfystudy_core.vcxproj') {
  if (-not (Test-Path (Join-Path 'build\windows-vs' $project))) { Fail "the project file $project was not generated" }
}
if ($solution[0].Extension -eq '.slnx') {
  $text = [IO.File]::ReadAllText($solution[0].FullName)
  if ($text -notmatch 'Path="comfystudy\.vcxproj"[^>]*DefaultStartup="true"') { Fail 'comfystudy must be the startup project of the solution' }
}
Ok ('generated build\windows-vs\' + $solution[0].Name + ' with a project per target and comfystudy as the startup project')

$r = Run-Tool 'cmake' @('--build', '--preset', 'windows-vs-debug')
if ($r.Code -ne 0) { Fail ("cmake --build --preset windows-vs-debug failed (exit {0}): {1}" -f $r.Code, ($r.Lines -join ' | ')) }
$exe = 'build\windows-vs\bin\Debug\comfystudy.exe'
if (-not (Test-Path $exe)) { Fail "$exe was not produced" }
$out = @(& $exe 2>&1 | ForEach-Object { "$_" }) -join "`n"
if ($LASTEXITCODE -ne 0 -or $out.Trim() -ne $expected) { Fail ("the program printed '{0}', expected '{1}'" -f $out.Trim(), $expected) }
Ok 'built with the solution and the program prints the greeting'

Write-Host 'check-vs: OK'
Set-Location -LiteralPath $startLocation
exit 0
