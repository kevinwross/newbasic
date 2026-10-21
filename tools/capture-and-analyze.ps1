# Captures the newbasic window and checks the drawing pixel by pixel against what
# docs/verify-graphics.bas asks for. Run from the repo root, in-process:
#   . .\tools\capture-and-analyze.ps1
# Dot-source rather than launching a nested pwsh: the nested process inherits the
# caller's stdio and a GUI child that runs forever keeps that pipe open.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public struct RECT { public int Left, Top, Right, Bottom; }
public struct POINT { public int X, Y; }
public class LwWin {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
'@

$exe = "out\src\newbasic-cli\Release\newbasic.exe"
$program = "docs\verify-graphics.bas"
$png = (Join-Path (Get-Location) "docs\newbasic-verify.png")

Get-Process newbasic -ErrorAction SilentlyContinue | ForEach-Object { Stop-Process -Id $_.Id -Force }
$proc = Start-Process -FilePath $exe -ArgumentList "--gpu", $program `
  -RedirectStandardOutput (Join-Path $env:TEMP "gpu_out.txt") `
  -RedirectStandardError (Join-Path $env:TEMP "gpu_err.txt") -PassThru
Start-Sleep -Seconds 3
if ($null -eq (Get-Process -Id $proc.Id -ErrorAction SilentlyContinue)) {
  throw "newbasic exited early: $(Get-Content (Join-Path $env:TEMP 'gpu_err.txt') -Raw)"
}

$handle = (Get-Process -Id $proc.Id).MainWindowHandle
[LwWin]::SetForegroundWindow($handle) | Out-Null
Start-Sleep -Milliseconds 900
$client = New-Object RECT
[LwWin]::GetClientRect($handle, [ref]$client) | Out-Null
$origin = New-Object POINT
[LwWin]::ClientToScreen($handle, [ref]$origin) | Out-Null
$width = $client.Right - $client.Left
$height = $client.Bottom - $client.Top

$bmp = New-Object System.Drawing.Bitmap($width, $height)
$graphics = [System.Drawing.Graphics]::FromImage($bmp)
$graphics.CopyFromScreen($origin.X, $origin.Y, 0, 0, $bmp.Size)
$graphics.Dispose()
$bmp.Save($png, [System.Drawing.Imaging.ImageFormat]::Png)

# Locate the canvas from the image itself: the program draws a grey (colour 7) frame
# on the canvas border, so scanning one row and one column finds the exact rectangle.
# Trusting the window rect instead is fragile - a few pixels of desktop can creep into
# the capture at the window edge.
$grey = @{}
function FindFrameX([int]$y) {
  $lo = -1; $hi = -1
  for ($x = 0; $x -lt $width; $x++) {
    $c = $bmp.GetPixel($x, $y)
    if ($c.R -gt 140 -and $c.G -gt 140 -and $c.B -gt 140 -and
        [Math]::Abs($c.R - $c.G) -lt 40 -and [Math]::Abs($c.G - $c.B) -lt 40) {
      if ($lo -lt 0) { $lo = $x }
      $hi = $x
    }
  }
  return @($lo, $hi)
}
function FindFrameY([int]$x) {
  $lo = -1; $hi = -1
  for ($y = 0; $y -lt $height; $y++) {
    $c = $bmp.GetPixel($x, $y)
    if ($c.R -gt 140 -and $c.G -gt 140 -and $c.B -gt 140 -and
        [Math]::Abs($c.R - $c.G) -lt 40 -and [Math]::Abs($c.G - $c.B) -lt 40) {
      if ($lo -lt 0) { $lo = $y }
      $hi = $y
    }
  }
  return @($lo, $hi)
}
$rowY = [int]($height / 2)
$cols = FindFrameX $rowY
if ($cols[0] -lt 0 -or $cols[1] -le $cols[0]) { throw "no canvas frame found on row $rowY" }
$minX = $cols[0]
$frameW = $cols[1] - $cols[0]
$rowsY = FindFrameY ([int]($minX + $frameW / 2))
if ($rowsY[0] -lt 0 -or $rowsY[1] -le $rowsY[0]) { throw "no canvas frame found on the centre column" }
$minY = $rowsY[0]
$frameH = $rowsY[1] - $rowsY[0]
"canvas measured from the frame: ($minX,$minY) ${frameW}x${frameH} in a ${width}x${height} client area"

function Pixel([double]$cx, [double]$cy) {
  $px = [int]($minX + ($cx + 0.5) * $frameW / 320.0)
  $py = [int]($minY + ($cy + 0.5) * $frameH / 200.0)
  if ($px -lt 0 -or $py -lt 0 -or $px -ge $width -or $py -ge $height) { return $null }
  return $bmp.GetPixel($px, $py)
}

$results = New-Object System.Collections.ArrayList
function Check([string]$name, [double]$cx, [double]$cy, [string]$expect) {
  $c = Pixel $cx $cy
  if ($null -eq $c) {
    [void]$results.Add([pscustomobject]@{ Check = $name; At = "$cx,$cy"; Expect = $expect; Read = 'off-image'; Pass = $false })
    return
  }
  # The CGA palette is not sRGB white: colour 7 is (170,170,170), 15 is (255,255,255).
  $ok = switch ($expect) {
    'green'   { $c.G -gt 110 -and $c.G -gt ($c.R + 50) }
    'red'     { $c.R -gt 110 -and $c.R -gt ($c.G + 50) }
    'cyan'    { $c.G -gt 110 -and $c.B -gt 110 -and $c.R -lt 100 }
    'brown'   { $c.R -gt 90 -and $c.G -gt 35 -and $c.B -lt 95 -and $c.R -gt $c.G }
    'blue'    { $c.B -gt 110 -and $c.B -gt ($c.R + 50) }
    'magenta' { $c.R -gt 110 -and $c.B -gt 110 -and $c.G -lt 100 }
    'grey7'   { $c.R -gt 140 -and $c.G -gt 140 -and $c.B -gt 140 -and
                [Math]::Abs($c.R - $c.G) -lt 40 -and [Math]::Abs($c.G - $c.B) -lt 40 }
    'black'   { $c.R -lt 70 -and $c.G -lt 70 -and $c.B -lt 70 }
  }
  [void]$results.Add([pscustomobject]@{
      Check = $name; At = "$cx,$cy"; Expect = $expect
      Read = "rgb($($c.R),$($c.G),$($c.B))"; Pass = [bool]$ok
    })
}

# CIRCLE radius 40 in SCREEN 1 gives a y radius of 33 (aspect 5/6).
Check "frame top"        160 0    grey7
Check "frame bottom"     160 199  grey7
# CIRCLE colour 1 is the mode's green, which also proves the SCREEN 1 palette.
Check "circle left"       40 100  green
Check "circle right"     120 100  green
Check "circle top"        80 67   green
Check "circle bottom"     80 133  green
Check "circle centre"     80 100  black
Check "diagonal magenta"  80 149  magenta
Check "BF box interior"  230 40   red
Check "BF box corner"    200 20   red
Check "magenta dot"       10 10   magenta
Check "brown dot"        310 190  brown
Check "B box top edge"   170 20   red
Check "B box interior"   170 30   black
Check "DRAW square edge" 305 40   brown
# The frame is one canvas pixel thick, so check its edges at the canvas border rather
# than outside it.
Check "frame left"         0 100  grey7
Check "frame right"      319 100  grey7

$bmp.Dispose()
Stop-Process -Id $proc.Id -Force

$results | Format-Table -AutoSize | Out-String -Width 100
$failed = @($results | Where-Object { -not $_.Pass })
"PASS $($results.Count - $failed.Count)/$($results.Count)"
if ($failed.Count -gt 0) {
  "FAILED:"
  $failed | Format-Table -AutoSize | Out-String -Width 100
  exit 1
}
