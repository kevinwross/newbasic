# Regenerates RefManual/README.md's index and inserts stable entry anchors.
# Run from the repository root with:
#   pwsh -File RefManual/_build-index.ps1
$root = Split-Path -Parent $PSScriptRoot
$ref = Join-Path $root 'RefManual'
$letters = Get-ChildItem $ref -Filter '*.md' |
    Where-Object { $_.Name -match '^[A-Z]\.md$' } |
    Sort-Object Name

function Get-Slug([string] $text) {
    $slug = $text.ToLowerInvariant()
    $slug = $slug -replace '`', ''
    $slug = $slug -replace '[^a-z0-9\s-]', ''
    $slug = $slug -replace '\s+', '-'
    $slug = $slug -replace '-+', '-'
    return $slug.Trim('-')
}

$entries = @()
foreach ($file in $letters) {
    $lines = [System.Collections.Generic.List[string]](Get-Content $file.FullName |
        Where-Object { $_ -notmatch '^<a id="[a-z0-9-]+"></a>$' })
    $used = @{}
    $out = [System.Collections.Generic.List[string]]::new()
    for ($i = 0; $i -lt $lines.Count; ++$i) {
        $line = $lines[$i]
        if ($line -match '^## (.+)$') {
            $heading = $Matches[1].Trim()
            $slug = Get-Slug $heading
            if ($used.ContainsKey($slug)) {
                $used[$slug]++
                $slug = "$slug-$($used[$slug])"
            } else {
                $used[$slug] = 1
            }
            $out.Add("<a id=`"$slug`"></a>")
            $out.Add($line)

            $type = ''
            $status = ''
            for ($j = $i + 1; $j -lt $lines.Count -and $lines[$j] -notmatch '^## '; ++$j) {
                if ($lines[$j] -match '\*\*Type:\*\*\s*(.+?)\s*$') { $type = $Matches[1] }
                if ($lines[$j] -match '\*\*Status:\*\*\s*(.+?)\s*$') { $status = $Matches[1] }
            }
            $entries += [pscustomobject]@{
                Heading = $heading
                Type = $type
                Status = $status
                File = $file.Name
                Slug = $slug
            }
        } else {
            $out.Add($line)
        }
    }
    [System.IO.File]::WriteAllLines($file.FullName, $out)
}

$sorted = $entries | Sort-Object { $_.Heading.ToLowerInvariant() }
$index = [System.Text.StringBuilder]::new()
[void]$index.AppendLine('| Entry | Type | Status |')
[void]$index.AppendLine('|---|---|---|')
foreach ($entry in $sorted) {
    $link = "[$($entry.Heading)]($($entry.File)#$($entry.Slug))"
    [void]$index.AppendLine("| $link | $($entry.Type) | $($entry.Status) |")
}

$unsupported = $entries |
    Where-Object { $_.Status -match '^Not supported' } |
    Sort-Object { $_.Heading.ToLowerInvariant() }
$unsupportedList = [System.Text.StringBuilder]::new()
[void]$unsupportedList.AppendLine('| Entry |')
[void]$unsupportedList.AppendLine('|---|')
foreach ($entry in $unsupported) {
    [void]$unsupportedList.AppendLine("| [$($entry.Heading)]($($entry.File)#$($entry.Slug)) |")
}

$readmePath = Join-Path $ref 'README.md'
$readme = Get-Content $readmePath -Raw
$readme = $readme -replace '(?s)<!-- INDEX:START -->.*?<!-- INDEX:END -->',
    "<!-- INDEX:START -->`n$($index.ToString().TrimEnd())`n<!-- INDEX:END -->"
$readme = $readme -replace '(?s)<!-- UNSUPPORTED:START -->.*?<!-- UNSUPPORTED:END -->',
    "<!-- UNSUPPORTED:START -->`n$($unsupportedList.ToString().TrimEnd())`n<!-- UNSUPPORTED:END -->"
[System.IO.File]::WriteAllText($readmePath, $readme, [System.Text.UTF8Encoding]::new($false))

"Indexed $($entries.Count) entries; $($unsupported.Count) not supported."
