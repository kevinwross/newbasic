# Building newbasic on Windows with Visual Studio

**Status:** verified working on 2026-10-21.
**Audience:** AI coding agents (and humans) asked to build this repository on a
Windows machine, targeting Windows, using Microsoft Visual Studio.
**Scope:** configure, build, test and smoke-run the x64 Windows binaries with
MSVC. This is the third build option alongside the Linux-native build and the
Linux-hosted Windows cross-build. It does not replace either of them.

This document is a runbook, not a suggestion box. Follow it in order, keep the
existing build files untouched, and record what you actually observed.

---

## 1. What this build option is

The repository has one platform-aware CMake build. On Windows it selects the
Win32 console backend, the GDI glyph rasteriser and the D3D11 window backend.
No source fork and no second build system is involved: CMake generates a Visual
Studio solution and projects in a build directory.

| Option | Host | Target | Toolchain | Typical use |
|---|---|---|---|---|
| A. Linux native | Linux | Linux | GCC or Clang, CMake | normal Linux development and CI |
| B. Linux to Windows cross | Linux | Windows | LLVM-MinGW, CMake | produce Windows binaries without a Windows host |
| C. **Windows native** (this document) | Windows | Windows | **MSVC + Visual Studio generator** | authoritative Windows check, Windows debugging, D3D11, SDK integration |

Option C is the only one that exercises the Microsoft compiler, the Microsoft
C++ runtime, the Windows SDK and the D3D11 path together. That is why it is
worth having even though A and B already exist.

### What "success" means

A successful run produces all of the following:

- `build-vs2026\src\newbasic-cli\Release\newbasic.exe`
- `build-vs2026\src\newbasic-dap\Release\newbasic-dap.exe`
- `build-vs2026\tests\Release\newbasic-tests.exe`
- `ctest`: 7 of 7 tests pass, 0 failures.
- A headless `--gpu-shot` run of `fixtures\m0\hello.bas` exits 0 and writes a
  valid binary PPM.
- Optionally, on an interactive desktop, `newbasic.exe --gpu-check` reports
  `PASS` for the D3D11 back-buffer readback.

The exact assertion and test-case counts can change as tests are added. Zero
failures is the acceptance criterion; a fixed number is only a snapshot.

---

## 2. Ground rules for AI agents

These rules exist because the Linux and cross-build paths must keep working.
This native-Windows path was added without changing any existing build file,
and it must stay that way.

1. **Do not edit existing build files.**
   This includes the root `CMakeLists.txt`, every
   `src/*/CMakeLists.txt`, `tests/CMakeLists.txt`, `.github/workflows/ci.yml`,
   `tools/verify.ps1` and `tools/verify.sh`.
   If a change to one of them seems necessary, stop and ask the human first.
   State the file, the proposed diff and why the task cannot be done without it.
2. **Do not hand-edit generated files.**
   `build-vs2026\newbasic.sln`, the `.vcxproj` files and `CMakeCache.txt` are
   CMake output. Change the source or the configure command instead.
3. **Do not add a second build system.**
   No hand-written `.sln`, no checked-in `.vcxproj`, no `CMakePresets.json`
   unless the human explicitly asks for one. The documented commands are
   enough.
4. **Keep generated files out of git.**
   Use a `build-*` or `out` directory; both are ignored by `.gitignore`.
   Do not commit `.exe`, `.pdb`, `.obj`, `.vs`, test logs or PPM captures.
5. **Use the build directory in this document.**
   `build-vs2026` matches the ignored `build-*/` pattern and does not collide
   with the Linux `build/` or the short `out/` used by `tools/verify.ps1`.
   A fresh agent may delete and recreate it; never delete the source tree.
6. **Do not weaken the warning policy.**
   First-party targets compile with `/W4` and `/WX`. If MSVC reports a warning,
   fix the code or add a narrowly scoped, commented MSVC branch. Do not remove
   `/WX`, add global `/wd` switches, or define `_CRT_SECURE_NO_WARNINGS`
   globally just to get a build.
7. **Do not skip or delete tests to get green.**
   A failure is information. Capture the log, retry once to distinguish a
   flake, then report the failing test and the first diagnostic.
8. **Do not run interactive or noisy checks unattended.**
   The windowed default and `--audio-check` need a desktop and/or speakers.
   Use `--cli` plus `--gpu-shot` for headless runs. Run `--gpu-check` only when
   an interactive desktop exists; it opens a real window for about three
   seconds.
9. **Preserve line endings and formatting.**
   `.gitattributes` deliberately marks fixtures and samples as binary-like so
   CP437 bytes and CRLF survive checkout. Do not run a whole-tree formatter or
   re-encode files.
10. **Report evidence, not impressions.**
    Include the configure toolchain line, build exit code, CTest summary, the
    headless PPM result and any diff you made. Do not claim the window "looks
    right" from a readback check.

### The one source change that is already applied

`src/newbasic-cli/main.cpp` contains a small portability macro because MSVC
deprecates `std::sscanf` and the CLI target builds with `/WX`. See section 7.
Keep it. If you revert it, the native build will fail again at the CLI link
step and the error will look like a warning, not a build break.

---

## 3. Verified toolchain

This is the environment the document was validated against. Other versions are
expected to work, but the paths and generator name below are the known-good
values.

| Item | Verified value |
|---|---|
| OS | Windows 10 Pro, build 19045 |
| Visual Studio | Community 2026, version 18.11.12306.390 |
| Installation path | `C:\Program Files\Microsoft Visual Studio\18\Community` |
| MSVC compiler | 19.51.36260.0 (toolset 14.51.36231) |
| Windows SDK | 10.0.26100.0 (targeting Windows 10.0.19045) |
| CMake | 4.3.1-msvc1, bundled with Visual Studio |
| CTest | 4.3.1-msvc1, same `bin` directory |
| CMake generator | `Visual Studio 18 2026` |
| Architecture | x64 |
| SDL2 | not required on Windows; the SDL2 backend defaults off |
| Network | not needed; Catch2, RapidCheck, stb and the DejaVu font are vendored |

### Required Visual Studio workload

The **Desktop development with C++** workload is required. It supplies:

- the MSVC x64 compiler and linker;
- a Windows SDK (the 10.0.26100 SDK here);
- the Visual Studio CMake integration used by the generator.

You can verify the C++ toolchain with:

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
& $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
```

An empty result means the C++ x64 tools are missing. Install the workload
through the Visual Studio Installer; do not attempt to repair it by editing
CMake files.

### Why CMake is not on `PATH`

Visual Studio installs CMake under its own directory and does not add it to the
system `PATH`. That is normal. Use the full path discovered below. Do not
install a second CMake, add a system-wide `PATH` entry, or call `setx` just to
make the commands shorter.

### Notes for other Visual Studio versions

- Visual Studio 2022 uses the generator `Visual Studio 17 2022` and lives under
  `C:\Program Files\Microsoft Visual Studio\2022\<Edition>`.
- The rest of this document is version-independent apart from the generator
  name and the installation path.
- If the generator name is wrong, `cmake --help` lists the generators this
  CMake actually supports. Use the newest installed Visual Studio generator;
  do not guess a year that is not installed.

---

## 4. Quick start

Run everything from the repository root in a normal PowerShell window. You do
not need a "Developer PowerShell", because the Visual Studio generator locates
the compiler through the installation.

```powershell
# 1. Discover the Visual Studio installation and its bundled CMake/CTest.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
if (-not $vs) { throw "Visual Studio with the C++ x64 workload was not found." }

$cmakeBin = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
$cmake = Join-Path $cmakeBin "cmake.exe"
$ctest = Join-Path $cmakeBin "ctest.exe"

# 2. Configure, build, test.
& $cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
if ($LASTEXITCODE -ne 0) { throw "configure failed" }

& $cmake --build build-vs2026 --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "build failed" }

& $ctest --test-dir build-vs2026 --build-config Release --timeout 180 --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }
```

Expected tail of the test step:

```text
100% tests passed, 0 tests failed out of 7
```

If the build exits 1 immediately after `Checking Build System` with no compiler
output, read section 8, "MSBuild stops before compiling". That is an environment
problem, not a source problem.

---

## 5. Step-by-step procedure

### 5.1 Start from a known tree

```powershell
Set-Location C:\Users\kevin\repos\newbasic   # or your checkout path
git status --short
```

Expected before you start: a clean tree, or only the changes you intend to
make. `build-vs2026` and the other ignored build directories must not appear.
If the tree is dirty, understand the existing changes before building; do not
stash or discard another agent's work.

### 5.2 Locate CMake and CTest

Use the discovery block in section 4. Sanity-check it:

```powershell
& $cmake --version     # expect 4.3.1-msvc1 or newer
& $ctest --version
& $cmake --help | Select-String "Visual Studio"
```

The help output must contain `Visual Studio 18 2026` on this machine. If it
only lists older generators, the Visual Studio installation is older than this
document assumes; use the newest listed Visual Studio generator and adjust the
path.

### 5.3 Configure

```powershell
& $cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
```

Why each argument:

| Argument | Purpose |
|---|---|
| `-S .` | source tree is the current directory (the repository root) |
| `-B build-vs2026` | out-of-source build directory; ignored by git |
| `-G "Visual Studio 18 2026"` | generate a Visual Studio 2026 solution and MSVC projects |
| `-A x64` | 64-bit target; do not omit it or use `Win32` unless intentionally testing 32-bit |

Do **not** pass `-DCMAKE_BUILD_TYPE=Release` to a multi-config Visual Studio
generator. It is ignored and only confuses later readers. Select the
configuration with `--config Release` at build and test time instead.

Expected configure output (abridged):

```text
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.19045.
-- The CXX compiler identification is MSVC 19.51.36260.0
-- Check for working CXX compiler: .../cl.exe - skipped
-- Configuring done
-- Generating done
```

If the configure step fails:

- `Generator ... could not find ...` or `could not create named generator`:
  the generator name does not match the installed Visual Studio. Run
  `& $cmake --help` and use the listed generator.
- `No CMAKE_CXX_COMPILER could be found`: the C++ workload or Windows SDK is
  missing. Install "Desktop development with C++" in the Visual Studio
  Installer.
- A cache error after moving the repository or changing Visual Studio
  versions: reconfigure with `--fresh`, or delete `build-vs2026` and configure
  again. The build directory contains no source work.

### 5.4 Build

```powershell
& $cmake --build build-vs2026 --config Release --parallel
```

This builds every target: Catch2, RapidCheck, the core library, screen and
window libraries, the CLI, the DAP server and the test executable. The first
build takes roughly 5 to 10 minutes on a modest desktop; later builds are
incremental and take seconds unless a header changed.

Expected output ends with the project files, for example:

```text
newbasic-core.vcxproj -> ...\build-vs2026\src\newbasic-core\Release\newbasic-core.lib
newbasic-screen.vcxproj -> ...\build-vs2026\src\newbasic-screen\Release\newbasic-screen.lib
newbasic-window.vcxproj -> ...\build-vs2026\src\newbasic-window\Release\newbasic-window.lib
newbasic-cli.vcxproj -> ...\build-vs2026\src\newbasic-cli\Release\newbasic.exe
newbasic-dap.vcxproj -> ...\build-vs2026\src\newbasic-dap\Release\newbasic-dap.exe
newbasic-tests.vcxproj -> ...\build-vs2026\tests\Release\newbasic-tests.exe
```

Useful focused builds while iterating:

```powershell
& $cmake --build build-vs2026 --config Release --target newbasic-cli
& $cmake --build build-vs2026 --config Release --target newbasic-tests
```

Do not invoke `msbuild` or `devenv` directly for automated work. CMake passes
the correct flags and configuration; bypassing it produces a build that this
document cannot vouch for. Humans may open `build-vs2026\newbasic.sln` in
Visual Studio for debugging, but an agent should keep using
`cmake --build`.

### 5.5 Run the test suite

```powershell
& $ctest --test-dir build-vs2026 --build-config Release --timeout 180 --output-on-failure
```

`--build-config Release` is required: with a multi-config generator, CTest
cannot guess which configuration's executables you mean. Without it, CTest may
report zero tests or look in the wrong directory.

Expected output:

```text
Test project C:/Users/kevin/repos/newbasic/build-vs2026
    Start 1: newbasic-tests
1/7 Test #1: newbasic-tests ...................   Passed    1.59 sec
    Start 2: sample-neon-horizon
2/7 Test #2: sample-neon-horizon ..............   Passed    0.06 sec
    Start 3: sample-ringworld
3/7 Test #3: sample-ringworld .................   Passed    0.51 sec
    Start 4: sample-fireworks
4/7 Test #4: sample-fireworks .................   Passed    0.12 sec
    Start 5: sample-spirograph
5/7 Test #5: sample-spirograph ................   Passed    0.16 sec
    Start 6: cli-dos-path
6/7 Test #6: cli-dos-path .....................   Passed    0.17 sec
    Start 7: dap-smoke
7/7 Test #7: dap-smoke ........................   Passed    0.14 sec

100% tests passed, 0 tests failed out of 7
```

The seven tests are:

| Test | What it covers |
|---|---|
| `newbasic-tests` | the Catch2 unit, property, conformance and integration suite |
| `sample-neon-horizon` | the shipped `NeonHorizon.bas` sample, run headlessly with SCREEN 720 pixel checks |
| `sample-ringworld` | the shipped `RingWorld.bas` sample, same presentation checks |
| `sample-fireworks` | the shipped `Fireworks.bas` sample, same presentation checks |
| `sample-spirograph` | the shipped `Spirograph.bas` sample, same presentation checks |
| `cli-dos-path` | the real CLI executable, DOS path translation and `--gpu-shot` capture; may briefly create a D3D11 window on Windows |
| `dap-smoke` | the real `newbasic-dap.exe` process over framed stdio |

To see the Catch2 count directly:

```powershell
& .\build-vs2026\tests\Release\newbasic-tests.exe
```

On this machine it reported:

```text
All tests passed (31209 assertions in 375 test cases)
```

The exact count is a snapshot; a different platform, compiler or a newly added
test will change it. All tests must pass.

Do not run the hidden `[.gpu]` or `[.audio]` tests as part of a headless
verification. They are excluded by default for a reason:

```powershell
# Only with an interactive desktop:
& .\build-vs2026\tests\Release\newbasic-tests.exe "[.gpu]" -s

# Only with speakers and a human nearby:
& .\build-vs2026\tests\Release\newbasic-tests.exe "[.audio]" -s
```

If a test hangs or leaves a process behind:

```powershell
Get-Process newbasic-tests, newbasic-dap -ErrorAction SilentlyContinue |
  Stop-Process -Force
```

Then rebuild and rerun once. `tools/verify.ps1` does this hygiene automatically
for the `out` build directory.

### 5.6 Smoke-test the CLI headlessly

The default CLI tries to open a D3D11 window when a window backend is built.
For an unattended check, force the console path and still request a capture, so
the CLI falls back to the headless screen when no terminal is available:

```powershell
& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe `
    --gpu-shot build-vs2026\hello.ppm --cli fixtures\m0\hello.bas
"EXIT=$LASTEXITCODE"
```

Expected:

```text
newbasic: view written to build-vs2026\hello.ppm
EXIT=0
```

Verify the capture is a real PPM rather than an empty file:

```powershell
$ppm = "build-vs2026\hello.ppm"
$bytes = [System.IO.File]::ReadAllBytes($ppm)
"magic=$([System.Text.Encoding]::ASCII.GetString($bytes[0..1])) bytes=$($bytes.Length)"
```

Expected: `magic=P6 bytes=768015` for a 640x400 text screen (640 * 400 * 3
bytes plus a 15-byte header).

**Argument-order detail:** `--gpu-shot` sets the window backend on, and `--cli`
sets it off. To get a headless run, put `--cli` *after* `--gpu-shot` as shown.
If `--cli` comes first, `--gpu-shot` turns the window back on and the run may
open a window. Do not reorder these flags casually.

Other useful CLI checks:

```powershell
& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe --help
& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe --bench
```

`--bench` is headless by design and runs four fixed workloads. It does not
touch the window backend and is a good interpreter smoke test. Its numbers are
machine-specific; do not treat them as a performance baseline for another host.

### 5.7 Optional: verify the D3D11 window path

Only run this when an interactive desktop exists. It creates a real window,
draws one frame, reads pixels back from the back buffer, prints the result and
sleeps for about three seconds:

```powershell
& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe --gpu-check
"GPU_CHECK_EXIT=$LASTEXITCODE"
```

Expected:

```text
gpu-check: centre=170,0,0 corner=0,0,0
gpu-check: PASS
```

This proves that the D3D11 device, swap chain, shaders and readback work. It
does **not** prove that the picture looks correct to a human. Say so if you
report it. The D3D11 backend tries hardware first and WARP second; a `PASS`
under WARP is still a valid software-renderer check.

For a specific pixel after a program run, use `--gpu-pixel X,Y` (canvas
coordinates) and read the reported `window pixel` line. That reads the window's
own back buffer, not the separate `--gpu-shot` model.

### 5.8 Optional: run a sample

With a desktop, the default windowed path is:

```powershell
& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe samples\Beethoven.bas
```

Headless equivalent:

```powershell
& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe `
    --gpu-shot build-vs2026\beethoven.ppm --cli samples\Beethoven.bas
```

`samples\accounting.bas` opens its data file relative to the working
directory, so run it from the repository root. `--audio-check` plays three
tones and exits; do not run it on a machine where unexpected sound is a
problem.

---

## 6. Build variants

These are optional. The default above is the configuration this document
verifies.

### Debug build

```powershell
& $cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
& $cmake --build build-vs2026 --config Debug --parallel
& $ctest --test-dir build-vs2026 --build-config Debug --timeout 300 --output-on-failure
```

Binaries land under `Debug\` instead of `Release\`. Debug is slower and enables
the C++ runtime's iterator and heap checks. Use it when diagnosing a crash, not
for the acceptance run.

### Console-only build (no D3D11)

```powershell
& $cmake -S . -B build-vs2026-console -G "Visual Studio 18 2026" -A x64 `
    -DNEWBASIC_WINDOW_D3D11=OFF
& $cmake --build build-vs2026-console --config Release --parallel
```

This removes the D3D11 source and libraries. `--gpu-check` then reports that no
windowed backend exists and exits 1; that is the expected degradation, not a
test failure. This variant is useful for isolating a display-driver problem.
It was not part of the primary verification run recorded below.

### Build without tests

```powershell
& $cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64 `
    -DNEWBASIC_BUILD_TESTS=OFF
```

This shortens the build and is fine for a quick executable, but it cannot
satisfy the acceptance checklist. Do not use it for the final verification.

### Warnings-as-errors

No extra switch is needed under MSVC: first-party targets already compile with
`/W4 /WX`. `-DNEWBASIC_WERROR=ON` is for the GCC/Clang path and has no effect on
the MSVC branch. Do not try to turn `/WX` off to make a build pass.

### The `tools/verify.ps1` wrapper

The CI hygiene wrapper uses the hard-coded build directory `out`:

```powershell
& $cmake -S . -B out -G "Visual Studio 18 2026" -A x64
.\tools\verify.ps1 -NoRetry
```

It builds, kills stray `newbasic-tests`/`newbasic-dap` processes, runs CTest,
captures the log under `out\test-logs\`, and retries once unless `-NoRetry` is
given. It is convenient, but it assumes `out` has already been configured and
it looks for CMake on `PATH` or at the VS 2026 path. The direct commands in
section 5 are the canonical procedure; the wrapper is the same shape with
extra hygiene.

### Clean rebuild

```powershell
Remove-Item -Recurse -Force build-vs2026
& $cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
& $cmake --build build-vs2026 --config Release --parallel
```

Deleting the build directory is safe. Never delete or move source files to
"clean" a build.

---

## 7. The MSVC-specific source fix that is already applied

### Symptom

Before the fix, the native build compiled every library, the DAP server and the
test executable, then failed at the CLI:

```text
src\newbasic-cli\main.cpp(56,12): error C2220: the following warning is treated as an error
src\newbasic-cli\main.cpp(56,12): warning C4996: 'sscanf': This function or variable may be unsafe.
Consider using sscanf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
src\newbasic-cli\main.cpp(192,14): warning C4996: 'sscanf': ...
```

The line numbers are from the pre-fix file; the two functions still exist, but
the calls now go through `NEWBASIC_SSCANF`.

### Cause

The Linux port replaced the original MSVC-only `sscanf_s` calls with portable
`std::sscanf`. That is correct for GCC and Clang, but MSVC still marks the
portable CRT `scanf` family as deprecated (C4996). The CLI target is a
first-party target, so `newbasic_target_warnings` adds `/WX`, turning the
warning into error C2220. The rest of the tree compiled without diagnostics
under MSVC, so the CLI was the only blocker.

### Fix

`src/newbasic-cli/main.cpp` now selects the secure variant only on MSVC:

```cpp
// The two scripted-input options below parse fixed "int,int[,int]" strings.
// MSVC marks the portable scanf family deprecated (C4996), and first-party
// targets build with /W4 /WX, so use the secure variant there. For numeric
// conversions the two functions behave identically.
#if defined(_MSC_VER)
#define NEWBASIC_SSCANF sscanf_s
#else
#define NEWBASIC_SSCANF std::sscanf
#endif
```

Both call sites (`ProbeWindowPixel` for `--gpu-pixel` and
`ApplyScriptedClicks` for `--clicks`) use `NEWBASIC_SSCANF`:

```cpp
if (NEWBASIC_SSCANF(g_pixel_probe.c_str(), "%d,%d", &x, &y) < 2) { ... }
if (NEWBASIC_SSCANF(entry.c_str(), "%d,%d,%d", &x, &y, &button) >= 2) { ... }
```

For `%d` conversions the two functions have identical behaviour, so the Linux
and MinGW builds are unchanged.

### Why not a build-file change

The alternative fixes were rejected deliberately:

- Defining `_CRT_SECURE_NO_WARNINGS` globally would silence a whole class of
  deprecation warnings and weaken the `/WX` contract for every translation
  unit.
- Adding `/wd4996` to the CLI target would hide future unsafe CRT use in that
  target.
- Removing `/WX` would let warnings accumulate and make the MSVC result
  unreliable.

The source-level branch is narrow, self-documenting and preserves all three
build options. If a future MSVC warning appears, fix the code the same way;
ask the human before touching a build file.

---

## 8. Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| `cmake` is not recognized | Visual Studio does not put its CMake on `PATH` | Use the full path from section 5.2. Do not install another CMake or change system `PATH`. |
| `could not create named generator "Visual Studio 18 2026"` | The installed Visual Studio is older, or the generator name is wrong | Run `& $cmake --help` and use the newest listed Visual Studio generator (for VS 2022: `Visual Studio 17 2022`). |
| Configure says no C++ compiler was found | The "Desktop development with C++" workload or Windows SDK is missing | Install the workload with the Visual Studio Installer. Re-run configure. |
| `C1083: Cannot open include file: 'windows.h'` | Windows SDK not installed or not selected | Install the Windows SDK component for the C++ workload. Check the configure line that selects the SDK. |
| Build exits 1 immediately after `Checking Build System`, with no compiler output | MSBuild's out-of-process worker nodes cannot communicate (sandbox, restricted token, antivirus) | Set `$env:MSBUILDDISABLENODEREUSE = "1"` and build with `--parallel 1 -- /nodeReuse:false /m:1`. See the command below. |
| `error C2220` followed by `warning C4996` | A portable CRT function is deprecated under MSVC and `/WX` is on | Use the MSVC-safe variant, as in section 7. Do not remove `/WX` or add a global deprecation define. |
| `LNK1104: cannot open file ...newbasic-tests.exe` | A previous test process still holds the executable | Kill stray `newbasic-tests` and `newbasic-dap` processes, then rebuild. |
| CTest reports 0 tests | Wrong build directory or missing `--build-config Release` | Pass `--test-dir build-vs2026 --build-config Release`; confirm `ctest -N` lists seven tests. |
| `newbasic requires a terminal` | The console backend could not initialise in a headless shell | Add `--gpu-shot FILE` and put `--cli` after it, or run in Windows Terminal. |
| `--gpu-check` prints `gpu-check: no windowed backend` | Built with `-DNEWBASIC_WINDOW_D3D11=OFF`, or the process has no interactive desktop | Reconfigure with D3D11 on for a desktop check; a headless agent should not treat this as a regression. |
| A window opens during a scripted test | The default path uses the D3D11 backend | Force `--cli` after the capture flag, as in section 5.6. Close the window if it is waiting for input. |
| Tests hang or time out | A test left a process behind, or an interactive test is waiting | Run with `--timeout 180`, kill stray processes, rerun once. Do not increase the timeout without evidence. |
| `dap-smoke` fails | The DAP executable is missing or a stale process holds it | Rebuild `newbasic-dap`, kill stray processes, rerun `ctest -R dap-smoke -V`. |
| `cli-dos-path` fails with "Can't open file" | The test passes a DOS-style path; the CLI translates it at the host boundary | Run from the repository root and do not rewrite the test's path by hand. Rebuild if the CLI is stale. |
| Configure or build fails only under a long repository path | Windows path-length limits in tools or the SDK | Use a shorter build directory (for example `C:\b\nb`) via `-B`, or enable long paths. Do not move the source tree casually. |
| Access denied writing the build directory | Antivirus, OneDrive sync or a permission issue | Use a local, non-synced build directory such as `C:\b\nb`; do not run the whole shell elevated. |
| Build uses a stale Visual Studio or SDK | Reused `CMakeCache.txt` after a toolchain change | Reconfigure with `--fresh` or delete `build-vs2026` and configure again. |

### MSBuild stops before compiling

If the plain parallel build stops immediately with only:

```text
MSBuild version ...
  Checking Build System
EXIT=1
```

then the compiler never ran. This was observed in a restricted sandbox where
MSBuild's worker nodes could not communicate. The reliable workaround is to
disable node reuse and build in one process:

```powershell
$env:MSBUILDDISABLENODEREUSE = "1"
& $cmake --build build-vs2026 --config Release --parallel 1 -- /nodeReuse:false /m:1
```

The first successful build in this document used exactly that command. On a
normal desktop, plain `--parallel` is correct and much faster; use the
workaround only when the symptom matches. It is not a source or CMake change.

---

## 9. Acceptance checklist

Copy this into the handoff note and fill in what you observed.

- [ ] Started from the repository root; `git status --short` understood.
- [ ] Located the VS-bundled `cmake.exe` and `ctest.exe`; versions recorded.
- [ ] Configure exited 0 with `MSVC 19.5x` and a Windows SDK line.
- [ ] Release build exited 0.
- [ ] `newbasic.exe`, `newbasic-dap.exe` and `newbasic-tests.exe` exist under
      `build-vs2026\...\Release\`.
- [ ] CTest: 7 of 7 passed, 0 failed.
- [ ] Direct `newbasic-tests.exe` run: all tests passed, 0 failures.
- [ ] Headless `hello.bas` run exited 0 and wrote a `P6` PPM of 768015 bytes.
- [ ] Optional desktop check: `--gpu-check` printed `PASS`.
- [ ] `git diff --` shows no changes to existing build files:
      `CMakeLists.txt`, `src/*/CMakeLists.txt`, `tests/CMakeLists.txt`,
      `.github/workflows/ci.yml`, `tools/verify.ps1`, `tools/verify.sh`.
- [ ] Only intended files are modified/untracked; build output stays ignored.
- [ ] The report states what was verified and what was not (especially visual
      correctness).

A quick guard for the build-file condition:

```powershell
git status --short
git diff -- CMakeLists.txt ":(glob)src/*/CMakeLists.txt" `
    tests/CMakeLists.txt .github/workflows/ci.yml `
    tools/verify.ps1 tools/verify.sh
```

The diff must print nothing. `git status` must not show a new or modified build
file. `src/newbasic-cli/main.cpp` is expected to differ only by the C4996 fix
described in section 7; `buildonwindows.md` is expected to be new.

---

## 10. Plan for the next agent (triage and handoff)

Follow the phases in order. Each phase has a stop condition so a small
environment problem does not turn into a speculative source rewrite.

### Phase 0 - read and inspect

1. Read this document fully.
2. Check `git status --short` and `git log -1 --oneline`.
3. Confirm the repository path, Visual Studio installation and build directory.
4. Do not start by editing CMake files. The known-good path needs no build-file
   changes.

Stop if the tree contains unexplained modifications. Report them instead of
building over them.

### Phase 1 - configure

Run the configure command from section 5.3. Capture the first 40 lines and the
last 10. The key facts to record are the compiler identification and the
selected Windows SDK.

- If the generator is missing, list generators and pick the installed one.
- If the compiler is missing, the environment is incomplete; report the exact
  missing component. Do not install software without permission.
- If the cache is stale, use `--fresh` or a new build directory.

Stop when configure exits non-zero for an environment reason that you cannot
fix without installing or changing the machine.

### Phase 2 - build

Run the build from section 5.4. If it fails, inspect the **first** error, not
the last line of output.

Classify it:

| Class | Example | Action |
|---|---|---|
| Environment/MSBuild | exits immediately after "Checking Build System" | Apply the node-reuse workaround in section 8. |
| Missing SDK/toolchain | `windows.h` not found, `cl.exe` not found | Report the missing component; do not patch source. |
| Source warning/error | `C2220`, `C4996`, a real compile error | Fix the source narrowly, preserving `/WX`; add a test if behaviour changed. |
| Link error | missing library, stale executable lock | Check the target's CMake link line; kill stray processes; rebuild. |
| Build-file need | a new source file must be added to a target | **Stop and ask the human.** Propose the CMake diff; do not apply it unilaterally. |

Rebuild only the failing target while iterating, then run the full build before
declaring success.

### Phase 3 - test

Run CTest from section 5.5. If a test fails:

1. Capture the full failing test output and the CTest log.
2. Rerun the single test with `-R <name> -V` to get detail.
3. Kill stray processes and rerun once to rule out a lock or timing flake.
4. If it fails again, treat it as real. Do not delete, disable or retag the
   test to get green.

A Windows-only failure is still a failure. Check whether the test is
platform-conditional (`tests\test_files.cpp` has a `_WIN32` branch) before
assuming the Linux result transfers.

### Phase 4 - smoke and optional desktop checks

Run the headless `--gpu-shot` check from section 5.6. This exercises the real
CLI, the host path translation, the headless screen and the PPM writer without
needing a desktop.

Run `--gpu-check` only if an interactive desktop is available and a brief
window is acceptable. Record `PASS` or the exact failure. Do not claim visual
correctness from it.

### Phase 5 - report

The handoff note should contain:

- date, machine and Windows version;
- Visual Studio version, MSVC version, Windows SDK version, CMake version;
- exact configure/build/test commands;
- configure result, build exit code, CTest summary, direct test count;
- headless PPM result (magic and byte count);
- optional GPU check result, clearly labelled readback-only;
- every file changed, with a one-line reason;
- any remaining risk or unverified area.

Do not pad the report with claims that were not measured.

### Things a next agent must not do

- Do not edit `CMakeLists.txt`, `tests/CMakeLists.txt`, the CI workflow or the
  verify scripts to make the build pass.
- Do not add a global `_CRT_SECURE_NO_WARNINGS`, remove `/WX`, or add `/wd`
  switches.
- Do not replace the Visual Studio generator with a hand-written project.
- Do not commit `build-vs2026`, `out`, `.vs`, binaries or logs.
- Do not run the GUI or audio tests unattended.
- Do not "fix" a test by changing its expectations without understanding the
  dialect behaviour.
- Do not run `tools/verify.ps1` against a dirty `out` directory that another
  agent may be using; prefer the direct `build-vs2026` commands.

---

## 11. Command cheat sheet

Assumes the discovery block from section 4 has been run in the current shell.

| Task | Command |
|---|---|
| Configure Release | `& $cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64` |
| Configure fresh | `& $cmake --fresh -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64` |
| Build Release | `& $cmake --build build-vs2026 --config Release --parallel` |
| Build one target | `& $cmake --build build-vs2026 --config Release --target newbasic-cli` |
| Build Debug | `& $cmake --build build-vs2026 --config Debug --parallel` |
| Test Release | `& $ctest --test-dir build-vs2026 --build-config Release --timeout 180 --output-on-failure` |
| List tests | `& $ctest --test-dir build-vs2026 --build-config Release -N` |
| Run one test verbosely | `& $ctest --test-dir build-vs2026 --build-config Release -R dap-smoke -V` |
| Run the whole Catch2 suite | `& .\build-vs2026\tests\Release\newbasic-tests.exe` |
| Headless hello capture | `& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe --gpu-shot build-vs2026\hello.ppm --cli fixtures\m0\hello.bas` |
| Headless benchmark | `& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe --bench` |
| D3D11 readback check | `& .\build-vs2026\src\newbasic-cli\Release\newbasic.exe --gpu-check` |
| Kill stray test processes | `Get-Process newbasic-tests, newbasic-dap -ErrorAction SilentlyContinue \| Stop-Process -Force` |
| Show build-file diff | `git diff -- CMakeLists.txt ":(glob)src/*/CMakeLists.txt" tests/CMakeLists.txt .github/workflows/ci.yml tools/verify.ps1 tools/verify.sh` |
| Remove the build directory | `Remove-Item -Recurse -Force build-vs2026` |

The generated solution, for humans who want the IDE:

```text
build-vs2026\newbasic.sln
```

---

## 12. Verification record

Measured on 2026-10-21 on the machine described in section 3.

| Check | Command | Result |
|---|---|---|
| Configure | `cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64` | exit 0; MSVC 19.51.36260.0; Windows SDK 10.0.26100.0; 3.9 s |
| Clean build | `cmake --build build-vs2026 --config Release --parallel` | exit 0 from an empty build directory; all targets linked |
| CTest | `ctest --test-dir build-vs2026 --build-config Release --timeout 180 --output-on-failure` | 7/7 passed, 0 failed, 3.04 s |
| Catch2 suite | `build-vs2026\tests\Release\newbasic-tests.exe` | 31,209 assertions in 375 test cases, all passed |
| Headless CLI | `newbasic.exe --gpu-shot build-vs2026\hello.ppm --cli fixtures\m0\hello.bas` | exit 0; `P6`, 768,015 bytes |
| Benchmark | `newbasic.exe --bench` | all four cases completed |
| D3D11 readback | `newbasic.exe --gpu-check` | `PASS`; centre `170,0,0`, corner `0,0,0` |
| Build-file changes | `git diff -- ...` | none |

The final clean build used plain `--parallel` and succeeded. In a restricted
sandbox the first attempt can fail before compiling (section 8); the single-node
command there is the fallback, not the default.

### Known limits of this verification

- The D3D11 check verifies device creation and back-buffer readback, not what a
  human sees on the monitor.
- The hidden `[.gpu]` and `[.audio]` tests were not run in the automated pass.
- The console-only variant was not exercised in this run; it is documented as a
  diagnostic option, not as a verified configuration here.
- Linux and MinGW builds were not rebuilt here. The source change is guarded by
  `_MSC_VER`, so it should not affect them, but this run did not prove it.
- The test-case count is a snapshot and will change as tests are added.

---

## 13. Maintenance notes

- **Visual Studio updates** can change the generator name. Check
  `cmake --help` after a major update. The generator is derived from the
  installed Visual Studio year, not from the repository.
- **CMake updates** should be harmless; the project requires CMake 3.20 or
  newer. Keep using the VS-bundled copy so CMake and the generator stay
  compatible.
- **New source files** must be added to the relevant `CMakeLists.txt`. That is
  a build-file change: stop and ask the human, then make the smallest possible
  edit and re-run the acceptance checklist.
- **New `sscanf` uses** in first-party code should use `NEWBASIC_SSCANF` or a
  portable alternative, not a global warning suppression.
- **CI parity:** the Windows CI job uses the same CMake project with the `out`
  directory and `tools/verify.ps1 -NoRetry`. If the local procedure changes,
  check whether the workflow needs the same change; do not edit the workflow
  without approval.
- **Stale status pages:** `docs/status.md` is a historical upstream snapshot
  and `plan.md` records that the MSVC job had not yet run on Windows. This
  document records the verified native procedure; update those pages only when
  the human asks, and do not rewrite history in a snapshot.
- **Keep this document executable.** If a command or path stops working, update
  the command here and record the new verification result. A runbook that has
  drifted from reality is worse than no runbook.

---

## Appendix A - Files that matter to this build

| Path | Role |
|---|---|
| `CMakeLists.txt` | top-level project, compiler flags, backend options, targets |
| `src/newbasic-core/CMakeLists.txt` | core library; `winmm` on Windows |
| `src/newbasic-screen/CMakeLists.txt` | Win32 console and GDI atlas on Windows |
| `src/newbasic-window/CMakeLists.txt` | D3D11 backend on Windows; `d3d11`, `dxgi`, `d3dcompiler` |
| `src/newbasic-cli/CMakeLists.txt` | `newbasic.exe`; first-party `/WX` |
| `src/newbasic-dap/CMakeLists.txt` | `newbasic-dap.exe`; `ws2_32` on Windows |
| `tests/CMakeLists.txt` | test executable and the CTest entries: unit suite, four sample regressions, CLI path and DAP smoke |
| `src/newbasic-cli/main.cpp` | CLI; contains the `NEWBASIC_SSCANF` MSVC fix |
| `tools/verify.ps1` | CI-style build/test wrapper for the `out` directory |
| `.github/workflows/ci.yml` | Windows and Linux CI jobs |
| `.gitignore` | ignores `out/` and `build-*/` |
| `docs/portability.md` | host-service and platform differences |
| `plan.md` | port plan and outstanding Windows/MSVC notes |

## Appendix B - Build options that affect this path

| Option | Default on Windows | Effect |
|---|---|---|
| `NEWBASIC_WINDOW_D3D11` | `ON` | builds the Win32/D3D11 window backend and links `d3d11`, `dxgi`, `d3dcompiler` |
| `NEWBASIC_WINDOW_SDL` | `OFF` | SDL2 is optional and unnecessary on Windows; leave it off |
| `NEWBASIC_BUILD_TESTS` | `ON` | builds `newbasic-tests` and registers the CTest entries |
| `NEWBASIC_WERROR` | `OFF` | only affects the GCC/Clang branch; MSVC first-party targets always use `/WX` |

`NEWBASIC_PLATFORM_WINDOWS` is set by the top-level CMake file when `WIN32` is
true and is informational for the platform-specific sources.

---

*End of runbook.*
