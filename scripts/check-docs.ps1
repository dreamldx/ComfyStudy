# Validates the tutorial chapters under docs/tutorial/phase-*/ (mainline) and docs/contrib/quest-*/ (quest).
# Usage: powershell -NoProfile -File scripts/check-docs.ps1 [-CheckTags]
# ASCII-only on purpose: Chinese labels are built from code points.
param([switch]$CheckTags)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function U([string]$hex) { -join ($hex.Split(' ') | ForEach-Object { [char][Convert]::ToInt32($_, 16) }) }
$LBL_STATUS  = U '72B6 6001 FF1A'          # status label
$SEP         = U 'FF5C'                    # header field separator
$TODO        = U '5F85 5199'               # placeholder text
$ST_NEW      = U '672A 5F00 59CB'
$ST_WRITING  = U '7F16 5199 4E2D'
$ST_VERIFIED = U '5DF2 9A8C 8BC1'
$ST_TAGGED   = U '5DF2 6253 0020 0074 0061 0067'
$LBL_TAG     = 'tag' + (U 'FF1A')
$LBL_BRANCH  = U '5206 652F FF1A'
$validStatus = @($ST_NEW, $ST_WRITING, $ST_VERIFIED, $ST_TAGGED)

$errors = New-Object System.Collections.Generic.List[string]
function Fail([string]$file, [string]$msg) { $script:errors.Add(('FAIL {0}: {1}' -f $file, $msg)) }
function ReadLf([string]$path) { ([IO.File]::ReadAllText($path, [Text.Encoding]::UTF8) -replace "`r`n", "`n") }

$taggedPhases = New-Object System.Collections.Generic.HashSet[string]
$tutorial = Join-Path $repo 'docs\tutorial'
$contrib = Join-Path $repo 'docs\contrib'
$chapters = @(Get-ChildItem -Path @($tutorial, $contrib) -Recurse -File -Filter '*.md' |
  Where-Object { ($_.Directory.Name -like 'phase-*' -and $_.Name -match '^\d+\.\d+-.+\.md$') -or
                 ($_.Directory.Name -like 'quest-*' -and $_.Name -match '^(diffusion|llm|platform|paper)-\d+\.\d+-.+\.md$') })

$existingTags = @(& git -C $repo tag -l)

foreach ($f in $chapters) {
  $rel = $f.FullName.Substring($repo.Length + 1).Replace('\', '/')
  $text = ReadLf $f.FullName
  $lines = $text.Split("`n")
  $head = @($lines | Select-Object -First 8)

  # (a) status line
  $status = $null
  $rxStatus = '^-\s*' + [regex]::Escape($LBL_STATUS) + '\s*(.+?)\s*' + [regex]::Escape($SEP)
  foreach ($l in $head) {
    $m = [regex]::Match($l, $rxStatus)
    if ($m.Success) { $status = $m.Groups[1].Value; break }
  }
  if ($null -eq $status) { Fail $rel 'missing status line in header'; continue }
  if ($validStatus -notcontains $status) { Fail $rel "unknown status '$status'"; continue }
  # Mainline chapters live in phase-NN-<slug>; quest chapters in quest-(diffusion|llm|platform|paper)-NN-<slug>.
  $isQuest = $f.Directory.Name -like 'quest-*'

  # A finished chapter is checked against the sources as they were at its tag (they keep
  # changing in later chapters); a chapter still in progress is checked against the working tree.
  $headerTag = $null
  $rxHeaderTag = [regex]::Escape($LBL_TAG) + '`([^`]+)`'
  foreach ($l in $head) { $m = [regex]::Match($l, $rxHeaderTag); if ($m.Success) { $headerTag = $m.Groups[1].Value; break } }
  $useTag = (-not $isQuest -and $status -eq $ST_TAGGED -and $headerTag -and ($existingTags -contains $headerTag))
  # A quest chapter is checked against the sources at the last commit of the chapter on its quest
  # branch (found by the commit prefix "chapter-llm-1.1-"), local or on origin. Without such a
  # commit (the branch is not there, or the chapter has no commits yet) the working tree is used.
  $questRev = $null
  if ($isQuest -and $status -ne $ST_NEW) {
    $questName = 'quest/' + ($f.Directory.Name -replace '^quest-', '')
    $chapterId = ($f.Name -replace '^((diffusion|llm|platform|paper)-\d+\.\d+)-.*$', '$1')
    foreach ($ref in @($questName, "origin/$questName")) {
      & git -C $repo rev-parse --verify --quiet "$ref^{commit}" 2>$null | Out-Null
      if ($LASTEXITCODE -ne 0) { continue }
      $hash = (& git -C $repo log $ref -E --grep ('^chapter-' + [regex]::Escape($chapterId) + '-') -n 1 --format=%H 2>$null)
      if ($hash) { $questRev = "$hash".Trim(); break }
    }
  }

  # (b) finished chapters: sections 1..11 in order, no placeholder left
  if ($status -ne $ST_NEW) {
    $found = @()
    foreach ($l in $lines) {
      $m = [regex]::Match($l, '^## (\d+)\.')
      if ($m.Success) { $found += [int]$m.Groups[1].Value }
    }
    $want9 = (1..9) -join ','
    $got = $found -join ','
    if ($got -ne "$want9,10,11") { Fail $rel 'sections must be 1..11 in order' }
    if ($text.Contains($TODO)) { Fail $rel 'status is not "not started" but the placeholder text is still present' }
  }

  # (c) annotated snippets must equal the source lines
  for ($i = 0; $i -lt $lines.Count; $i++) {
    $m = [regex]::Match($lines[$i], '^<!-- src: (.+?)#L(\d+)-L(\d+) -->\s*$')
    if (-not $m.Success) { continue }
    $srcRel = $m.Groups[1].Value; $a = [int]$m.Groups[2].Value; $b = [int]$m.Groups[3].Value
    if ($i + 1 -ge $lines.Count -or -not $lines[$i + 1].StartsWith('```')) { Fail $rel "src annotation for $srcRel is not followed by a code fence"; continue }
    $j = $i + 2
    $block = New-Object System.Collections.Generic.List[string]
    while ($j -lt $lines.Count -and -not $lines[$j].StartsWith('```')) { $block.Add($lines[$j]); $j++ }
    if ($j -ge $lines.Count) { Fail $rel "unterminated code fence after annotation for $srcRel"; continue }
    if ($useTag -or $questRev) {
      $srcRev = if ($useTag) { $headerTag } else { $questRev }
      $shown = @(& git -C $repo show ("{0}:{1}" -f $srcRev, $srcRel) 2>$null)
      if ($LASTEXITCODE -ne 0) { Fail $rel "annotated source file not found at ${srcRev}: $srcRel"; $i = $j; continue }
      $srcLines = ($shown -join "`n").Split("`n")
    }
    else {
      $srcPath = Join-Path $repo $srcRel
      if (-not (Test-Path $srcPath)) { Fail $rel "annotated source file not found: $srcRel"; $i = $j; continue }
      $srcLines = (ReadLf $srcPath).Split("`n")
    }
    if ($a -lt 1 -or $b -lt $a -or $b -gt $srcLines.Count) { Fail $rel "bad line range L$a-L$b for $srcRel"; $i = $j; continue }
    $expected = $srcLines[($a - 1)..($b - 1)]
    if (($expected -join "`n") -ne ($block -join "`n")) { Fail $rel "snippet differs from $srcRel lines $a-$b" }
    $i = $j
  }

  # (d) mainline tags / (e) quest branches
  if ($isQuest) {
    $branch = 'quest/' + ($f.Directory.Name -replace '^quest-', '')
    $want = $LBL_BRANCH + '`' + $branch + '`'
    $hit = $false
    foreach ($l in $head) { if ($l.Contains($want)) { $hit = $true } }
    if (-not $hit) { Fail $rel "header must contain branch label with `"$branch`"" }
  }
  elseif ($CheckTags -and $status -eq $ST_TAGGED) {
    $tag = $null
    $rxTag = [regex]::Escape($LBL_TAG) + '`([^`]+)`'
    foreach ($l in $head) { $m = [regex]::Match($l, $rxTag); if ($m.Success) { $tag = $m.Groups[1].Value; break } }
    if ($null -eq $tag) { Fail $rel 'tagged chapter has no tag in its header'; continue }
    $found = @(& git -C $repo tag -l $tag)
    if ($found -notcontains $tag) { Fail $rel "tag '$tag' does not exist" }
    [void]$taggedPhases.Add($f.Directory.Name)
  }
}

# (f) a phase with a tagged chapter must have its phase tag, named like the phase folder
if ($CheckTags) {
  foreach ($phase in $taggedPhases) {
    $found = @(& git -C $repo tag -l $phase)
    if ($found -notcontains $phase) { Fail "docs/tutorial/$phase" "phase tag '$phase' does not exist" }
  }
}

if ($errors.Count -gt 0) {
  $errors | ForEach-Object { Write-Host $_ }
  Write-Host ('check-docs: FAILED ({0} problem(s))' -f $errors.Count)
  exit 1
}
Write-Host ('check-docs: OK ({0} chapters)' -f $chapters.Count)
exit 0
