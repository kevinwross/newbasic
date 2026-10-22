# Debugging newbasic from VS Code on Windows

**Status:** the Visual Studio/MSVC build values below are verified by
`buildonwindows.md`; the extension is the same platform-neutral code whose
debug flow was verified on Linux, with Windows paths. Run the F5/breakpoint
checks on the target Windows machine and report them as evidence.
**Audience:** AI coding agents (and humans) asked to make VS Code debugging work
on a Windows 11 machine for this repository.
**Scope:** build the interpreter and DAP adapter with MSVC, install the VS Code
extension, configure the workspace, run a debug session, and verify breakpoints.

The VS Code extension is plain JavaScript and is **platform-neutral**; the same
`newbasic-debug-0.1.0.vsix` installs on Windows and Linux. Only the
`newbasic-dap.exe` adapter is a Windows binary.

---

## 1. What success looks like

A successful run produces all of the following:

- `out\src\newbasic-dap\Release\newbasic-dap.exe` exists.
- `out\tests\Release\newbasic-tests.exe` passes (CTest: 7 of 7, 0 failures).
- `code --list-extensions --show-versions` contains
  `newbasic.newbasic-debug@0.1.0`.
- **Output → newbasic** logs the `.exe` path, working directory and launch.
- F5 on `fixtures\m0\hello.bas` opens the `newbasic` window, prints
  `HELLO FROM NEWBASIC` and `5  10`, and terminates.
- A gutter breakpoint on `fixtures\m0\hello.bas` line 20 stops the session, and
  F10 steps.
- Optionally, `newbasic.exe --gpu-check` reports `PASS` on the D3D11 backend.

Exact test counts can change. Zero failures is the acceptance criterion.

---

## 2. Ground rules for AI agents

1. **Do not commit build output or the VSIX.** `out\`, `build-*\` and `*.vsix`
   are generated and ignored (or must stay untracked). Do not commit `.vs`,
   `.obj`, `.pdb`, `.exe` or logs.
2. **Do not edit the C++ source or CMake files** to make debugging work.
3. **Do not put a Linux-only path in `newbasic.adapterPath`.** The checked-in
   workspace settings leave it empty so the same file works on Windows and
   Linux. If it is set, it must point at a Windows `.exe`.
4. **Reload VS Code after installing or repackaging the extension.** A running
   window keeps the old extension code and the old debug contributions.
5. **Use the Visual Studio generator/MSVC**, not a MinGW build, unless the human
   explicitly asks for a cross-build. The debug adapter and window backend
   should match the normal Windows build.
6. **Do not use a bare extension symlink as proof of installation.**
   `code --list-extensions` does not reliably list symlinked development
   extensions. Install the VSIX.
7. **Report evidence.** Include the adapter path, Output → newbasic lines, the
   window result and the breakpoint stop. Do not claim success from a build
   alone.

---

## 3. Verified environment

| Item | Verified value |
|---|---|
| OS | Windows 10 Pro 19045; Windows 11 uses the same steps |
| Visual Studio | Community 2026, 18.11.12306.390 |
| MSVC | 19.51.36260.0 (toolset 14.51.36231) |
| Windows SDK | 10.0.26100.0 |
| CMake/CTest | 4.3.1-msvc1, bundled with Visual Studio |
| Generator | `Visual Studio 18 2026`, x64 |
| VS Code | 1.142.0 (extension engine requires ≥1.60) |
| SDL2 | not required; Windows uses D3D11 |
| Network | not needed for the build; needed once for `npx @vscode/vsce` |

Visual Studio 2022 uses the generator `Visual Studio 17 2022` and lives under
`C:\Program Files\Microsoft Visual Studio\2022\<Edition>`. Use the generator
that `cmake --help` actually lists.

Node.js LTS and npm are needed **only** to run `npx @vscode/vsce` when packaging
the extension. If a ready-made VSIX is supplied, skip the Node dependency.

### Required Visual Studio workload

**Desktop development with C++** supplies the MSVC x64 compiler, linker and
Windows SDK. Verify it:

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
& $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
```

An empty result means the C++ x64 workload is missing. Install it through the
Visual Studio Installer.

---

## 4. Discover the toolchain

Start from the repository root with the current tree:

```powershell
git pull --ff-only
git status --short
```

The steps below assume the current `editors\vscode` sources (version 0.1.0,
with platform-aware adapter lookup and post-substitution validation). If local
extension changes are present, do not discard them; install the VSIX built from
that tree.

CMake and CTest are installed with Visual Studio but are usually not on `PATH`.
Use the full paths:

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
if (-not $vs) { throw "Visual Studio with the C++ x64 workload was not found." }

$cmakeBin = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
$cmake = Join-Path $cmakeBin "cmake.exe"
$ctest = Join-Path $cmakeBin "ctest.exe"

& $cmake --version
& $ctest --version
```

Choose the generator:

```powershell
$help = & $cmake --help
if ($help -match 'Visual Studio 18 2026') {
    $generator = 'Visual Studio 18 2026'
} elseif ($help -match 'Visual Studio 17 2022') {
    $generator = 'Visual Studio 17 2022'
} else {
    throw "No supported Visual Studio generator found. Inspect: cmake --help"
}
$generator
```

---

## 5. Build the interpreter and adapter

From the repository root. Use **`out`** so the extension's default Windows
lookup finds the adapter:

```powershell
& $cmake -S . -B out -G $generator -A x64
if ($LASTEXITCODE -ne 0) { throw "configure failed" }

& $cmake --build out --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "build failed" }
```

Required artifact:

```powershell
$adapter = Join-Path $PWD "out\src\newbasic-dap\Release\newbasic-dap.exe"
if (-not (Test-Path $adapter)) { throw "adapter not found: $adapter" }
$adapter
```

Run the process-level DAP smoke test:

```powershell
$adapterForCmake = $adapter -replace '\\', '/'
& $cmake `
  "-DDAP_EXE=$adapterForCmake" `
  "-DTMP_DIR=$env:TEMP\newbasic-dap-smoke" `
  -P tests\dap_smoke.cmake
if ($LASTEXITCODE -ne 0) { throw "DAP smoke test failed" }
```

Expected final line:

```text
newbasic-dap smoke test passed
```

Run the test suite:

```powershell
& $ctest --test-dir out --build-config Release --timeout 180 --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }
```

Expected: `100% tests passed, 0 tests failed out of 7`.

### Alternative build directories

The extension also checks `build-sdl\` and `build\` (with `.exe`). If you build
somewhere else, such as `build-vs2026\` from `buildonwindows.md`, either copy the
adapter into `out\src\newbasic-dap\Release\` or set `newbasic.adapterPath` to
the full Windows path in `.vscode\settings.json` (section 7). Do not commit a
Windows-only setting in a repo also used on Linux unless the team accepts that.

---

## 6. Package and install the extension

The extension source is `editors\vscode`; it is plain JavaScript and has no
native dependencies. The VSIX is platform-neutral and can also be installed on
Linux.

```powershell
Set-Location editors\vscode
npx --yes @vscode/vsce package `
  --out "$HOME\newbasic-debug-0.1.0.vsix" `
  --allow-missing-repository
if ($LASTEXITCODE -ne 0) { throw "vsce package failed" }
```

If PowerShell blocks the npm wrapper with an execution-policy error, use
`npx.cmd` or bypass the policy for this process:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

`npx` downloads `@vscode/vsce` on first use (network). If network is unavailable,
copy a `newbasic-debug-0.1.0.vsix` produced elsewhere.

Install and verify. If `code` is not on `PATH`, use:

```powershell
$code = "$env:LOCALAPPDATA\Programs\Microsoft VS Code\bin\code.cmd"
```

Then:

```powershell
code --install-extension "$HOME\newbasic-debug-0.1.0.vsix" --force
code --list-extensions --show-versions | Select-String newbasic
```

Expected:

```text
newbasic.newbasic-debug@0.1.0
```

The extension is copied under:

```text
%USERPROFILE%\.vscode\extensions\newbasic.newbasic-debug-0.1.0
```

Do not commit the VSIX. Remove stale duplicate copies if the list shows more
than one.

---

## 7. Workspace configuration

The repository should contain these files. Create them if missing; otherwise
verify their contents without overwriting unrelated settings.

`.vscode\launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "newbasic",
      "request": "launch",
      "name": "Run this program",
      "program": "${file}"
    }
  ]
}
```

`.vscode\settings.json`:

```json
{
  "debug.allowBreakpointsEverywhere": true
}
```

Leave `newbasic.adapterPath` **empty/absent** so the same settings work on both
platforms. The extension searches:

1. `newbasic.adapterPath`, if set (supports `${workspaceFolder}`);
2. `build-sdl\src\newbasic-dap\newbasic-dap.exe`;
3. `build\src\newbasic-dap\newbasic-dap.exe`;
4. `out\src\newbasic-dap\Release\newbasic-dap.exe`;
5. `out\src\newbasic-dap\Debug\newbasic-dap.exe`.

If you must set it for an unusual build directory, use forward slashes and a
Windows path, for example:

```json
{
  "debug.allowBreakpointsEverywhere": true,
  "newbasic.adapterPath": "${workspaceFolder}/build-vs2026/src/newbasic-dap/Release/newbasic-dap.exe"
}
```

---

## 8. Reload and run

1. Command Palette → **Developer: Reload Window**. Required after installing or
   repackaging the extension.
2. Open `fixtures\m0\hello.bas`.
3. Press **F5** and choose **Run this program** if prompted.
4. Expect a separate `newbasic` window showing:

   ```text
   HELLO FROM NEWBASIC
   5  10
   ```

5. Open **Output → newbasic**. A healthy session contains lines like:

   ```text
   newbasic debugger on win32/x64, VS Code 1.142.0
   registered debug configuration provider and adapter factory
   launch: type=newbasic program=C:\...\fixtures\m0\hello.bas cwd=C:\...\newbasic
   adapter: C:\...\out\src\newbasic-dap\Release\newbasic-dap.exe args=[] cwd=C:\...\newbasic
   ```

6. Click the **gutter** left of line 20 (or right-click the gutter), press F5,
   and confirm the session stops on line 20. F10 steps; the Variables view shows
   program state.

PRINT output appears in the `newbasic` window, **not** in the VS Code Debug
Console. An empty Debug Console is expected.

For graphics, open `samples\RingWorld.bas`, press F5, and confirm the D3D11
window renders the planet. Optionally, on an interactive desktop:

```powershell
out\src\newbasic-cli\Release\newbasic.exe --gpu-check
```

Expected: `gpu-check: PASS`.

---

## 9. Breakpoints

`.bas` files are claimed by VS Code's built-in **Visual Basic** language
(`vb`). The debugger contributes breakpoint support for both `basic` and `vb`,
and the workspace sets `debug.allowBreakpointsEverywhere` as a fallback.

- **Left-click the gutter** to toggle a breakpoint.
- **Right-click the gutter** for Add Breakpoint / Conditional Breakpoint /
  Logpoint.
- **Right-click the source text** offers *Add Inline Breakpoint* while a debug
  session is active. That is standard VS Code; the full menu is on the gutter.

If the gutter menu is missing: reload the window, confirm the extension version,
check the status-bar language (`Visual Basic`), and keep
`debug.allowBreakpointsEverywhere` true.

---

## 10. Headless debugging

On a build agent or over RDP without a desktop, set:

```json
{
  "debug.allowBreakpointsEverywhere": true,
  "newbasic.showWindow": false
}
```

The adapter receives `--nowindow`, uses a headless screen, and falls back
automatically if D3D11 cannot initialize. `PRINT` is still not forwarded to the
Debug Console, and `INPUT`/`INKEY$` cannot be used headless. Use this only for
stepping and variable inspection.

For `Step Back`/`reverseContinue`, set `"newbasic.history": true`.

---

## 11. Troubleshooting

| Symptom | Check | Fix |
|---|---|---|
| `code` is not recognized | `Get-Command code` | Use `%LOCALAPPDATA%\Programs\Microsoft VS Code\bin\code.cmd` or reinstall VS Code with "Add to PATH". |
| Visual Studio/C++ tools not found | `vswhere` output | Install **Desktop development with C++**; use the Visual Studio Installer, not CMake edits. |
| Generator not found | `cmake --help` | Use the installed Visual Studio generator (`17 2022` or `18 2026`). |
| `newbasic-dap was not found` | Output lists searched paths | Build `out\` or `build\`, or set `newbasic.adapterPath` to the `.exe`. |
| `Invalid debug adapter` | **Output → newbasic**, adapter process | Usually a bad adapter path, non-existent program, or launch-time error. The Output channel now names the path. |
| `program not found: .../${file}` | Output shows literal `${file}` | Old extension. Repackage/reinstall current `editors\vscode`, then reload. |
| No breakpoint menu | Window not reloaded | Reload; confirm `newbasic.newbasic-debug@0.1.0`; keep `debug.allowBreakpointsEverywhere`. |
| Session starts, no output window | Interactive desktop? D3D11? | Run on a desktop session; check Output for fallback; or use `showWindow: false`. PRINT is not in the Debug Console. |
| `npx` policy error | PowerShell execution policy | `Set-ExecutionPolicy -Scope Process Bypass`, then retry; or use `npx.cmd`. |
| `vsce` network failure | Network/proxy | Use a VSIX produced elsewhere; do not vendor `node_modules`. |
| Wrong platform path in settings | `newbasic.adapterPath` | Remove it or point to the Windows `.exe`; never copy the Linux path here. |

---

## 12. Evidence checklist for the agent

Report all of these:

- Windows version/arch, Visual Studio version, MSVC/SDK, CMake/CTest versions.
- Configure/build commands, generator, exit codes.
- Adapter path and `Test-Path` result.
- DAP smoke output (`newbasic-dap smoke test passed`).
- CTest summary (7/7, 0 failures).
- `code --list-extensions --show-versions` line for `newbasic`.
- Output → newbasic lines showing the resolved `.exe` and cwd.
- Fixture result in the `newbasic` window.
- Breakpoint line and the fact that the session stopped there.
