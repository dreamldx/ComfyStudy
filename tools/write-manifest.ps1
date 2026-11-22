# Writes <BinDir>\dlls.manifest: one line per *.dll next to the executable, sorted by name:
#   name <TAB> sha256 <TAB> package <TAB> package_version
# `package` is the vcpkg package that installed the DLL (found in vcpkg's own file lists), or "-".
# Usage: write-manifest.ps1 -BinDir <dir> -VcpkgInfoDir <vcpkg_installed\vcpkg\info>
param(
  [Parameter(Mandatory)][string]$BinDir,
  [Parameter(Mandatory)][string]$VcpkgInfoDir
)
$ErrorActionPreference = 'Stop'

# DLL file name (lower case) -> package and version, from "<package>_<version>_<triplet>.list".
$owner = @{}
if (Test-Path -LiteralPath $VcpkgInfoDir) {
  foreach ($list in Get-ChildItem -LiteralPath $VcpkgInfoDir -Filter '*.list') {
    $m = [regex]::Match($list.BaseName, '^(?<package>.+?)_(?<version>[^_]+)_(?<triplet>[^_]+)$')
    if (-not $m.Success) { continue }
    foreach ($line in [IO.File]::ReadLines($list.FullName)) {
      if ($line -match '(?i)\.dll$') {
        $owner[[IO.Path]::GetFileName($line).ToLowerInvariant()] = $m.Groups['package'].Value, $m.Groups['version'].Value
      }
    }
  }
}

$lines = foreach ($dll in Get-ChildItem -LiteralPath $BinDir -Filter '*.dll' | Sort-Object { $_.Name.ToLowerInvariant() }) {
  $hash = (Get-FileHash -LiteralPath $dll.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  $package, $version = '-', '-'
  $found = $owner[$dll.Name.ToLowerInvariant()]
  if ($found) { $package, $version = $found }
  "{0}`t{1}`t{2}`t{3}" -f $dll.Name, $hash, $package, $version
}

$manifest = Join-Path $BinDir 'dlls.manifest'
[IO.File]::WriteAllText($manifest, (($lines -join "`n") + "`n"), (New-Object Text.UTF8Encoding $false))
Write-Host ("write-manifest: {0} DLL(s) -> {1}" -f @($lines).Count, $manifest)
