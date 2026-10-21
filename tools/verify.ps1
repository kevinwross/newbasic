# Builds the project and runs the test suite with the hygiene that the flaky
# failures needed:
#   * stray test processes are killed first (a leftover newbasic-tests used to
#     hold the binary and confuse later runs),
#   * ctest output is always captured to a log, not just shown on failure,
#   * a failing run is retried once, so an environment glitch is reported as
#     flaky (with the log kept) instead of being mistaken for a regression.
#
# Usage:
#   pwsh tools/verify.ps1                 # build Release, test, retry once
#   pwsh tools/verify.ps1 -Config Debug
#   pwsh tools/verify.ps1 -NoRetry        # CI-style: treat any failure as real
#
# Exit code: 0 when the suite passed (first or second attempt), 1 otherwise.

param(
  [string]$Config = "Release",
  [switch]$NoRetry
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "out"
$logDir = Join-Path $root "out\test-logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

function Find-Cmake {
  $command = Get-Command cmake -ErrorAction SilentlyContinue
  if ($command) { return $command.Source }
  $candidate = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if (Test-Path $candidate) { return $candidate }
  throw "cmake not found on PATH and the VS 2026 bundled copy is missing."
}

function Stop-StrayTests {
  foreach ($name in @("newbasic-tests", "newbasic-dap")) {
    Get-Process $name -ErrorAction SilentlyContinue | ForEach-Object {
      Write-Host "killing stray $name (pid $($_.Id))"
      Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
  }
}

$cmake = Find-Cmake
$ctest = Join-Path (Split-Path $cmake) "ctest.exe"

Stop-StrayTests
Write-Host "== build ($Config) =="
& $cmake --build $buildDir --config $Config --parallel | Out-Host
if ($LASTEXITCODE -ne 0) {
  Write-Host "BUILD FAILED (exit $LASTEXITCODE)"
  exit 1
}
Stop-StrayTests

function Invoke-Suite([string]$tag) {
  $log = Join-Path $logDir "ctest-$tag.log"
  & $ctest --test-dir $buildDir --build-config $Config --timeout 120 `
    --output-on-failure 2>&1 | Out-File -Encoding utf8 $log
  $code = $LASTEXITCODE
  Write-Host "== ctest ($tag) exit $code, log: $log =="
  if ($code -ne 0) {
    Select-String -Path $log -Pattern "Test #|FAILED|Timeout|FATAL|error" |
      Select-Object -First 20 | ForEach-Object { Write-Host "   $($_.Line)" }
  }
  return $code
}

$first = Invoke-Suite "run1"
if ($first -eq 0) {
  Write-Host "PASS (first attempt)"
  exit 0
}
if ($NoRetry) {
  Write-Host "FAIL (no retry requested)"
  exit 1
}

Stop-StrayTests
$second = Invoke-Suite "run2"
if ($second -eq 0) {
  Write-Host "FLAKY: the first run failed but a retry passed."
  Write-Host "Both logs are kept under out\test-logs for comparison."
  exit 0
}
Write-Host "FAIL (both attempts failed) - this is a real failure, not flakiness."
exit 1
