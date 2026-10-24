param(
    [string]$Newbasic = ""
)

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$repo = (Resolve-Path (Join-Path $here "..\..")).Path
if ([string]::IsNullOrWhiteSpace($Newbasic)) {
    $Newbasic = Join-Path $repo "out\src\newbasic-cli\Release\newbasic.exe"
}
$Newbasic = (Resolve-Path $Newbasic).Path
$library = Join-Path $repo "baslib\baslib.bas"
if (-not (Test-Path $library)) { throw "missing $library" }

function Normalize-Text([string]$text) {
    $lines = $text -split "\r?\n" | ForEach-Object { ($_ -replace '\s+', ' ').Trim() } |
        Where-Object { $_ -ne "" }
    return ($lines -join "`n")
}

$failures = 0
$fixtures = Get-ChildItem (Join-Path $here "fixtures") -Filter "*.bas" | Sort-Object Name
foreach ($fixture in $fixtures) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($fixture.Name)
    $expectedPath = Join-Path $here "expected\$name.txt"
    if (-not (Test-Path $expectedPath)) {
        Write-Host "FAIL $name (missing expected\$name.txt)"
        $failures++
        continue
    }

    $temp = Join-Path $env:TEMP ("baslib-cli-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $temp | Out-Null
    try {
        $merged = Join-Path $temp "merged.bas"
        $hostText = Get-Content -Raw -LiteralPath $fixture.FullName
        $libraryText = Get-Content -Raw -LiteralPath $library
        Set-Content -LiteralPath $merged -Value ($hostText + "`n" + $libraryText) -Encoding ascii

        $resultPath = Join-Path $repo "result.txt"
        Remove-Item -LiteralPath $resultPath -ErrorAction SilentlyContinue
        Push-Location $repo
        try {
            $ppm = Join-Path $temp "screen.ppm"
            & $Newbasic --gpu-shot $ppm $merged 2>$null | Out-Null
            $runExit = $LASTEXITCODE
        } finally {
            Pop-Location
        }

        if ($runExit -ne 0 -or -not (Test-Path $resultPath)) {
            Write-Host "FAIL $name (run exit $runExit, result file missing)"
            Remove-Item -LiteralPath $resultPath -ErrorAction SilentlyContinue
            $failures++
            continue
        }

        $actual = Normalize-Text (Get-Content -Raw -LiteralPath $resultPath)
        Remove-Item -LiteralPath $resultPath -ErrorAction SilentlyContinue
        $expected = Normalize-Text (Get-Content -Raw -LiteralPath $expectedPath)
        if ($actual -ne $expected) {
            Write-Host "FAIL $name"
            Write-Host "--- expected"
            Write-Host $expected
            Write-Host "--- actual"
            Write-Host $actual
            $failures++
        } else {
            Write-Host "PASS $name"
        }
    } finally {
        Remove-Item -Recurse -Force $temp -ErrorAction SilentlyContinue
    }
}

if ($failures -ne 0) {
    Write-Host "$failures fixture(s) failed"
    exit 1
}
Write-Host "All baslib CLI fixtures passed."
