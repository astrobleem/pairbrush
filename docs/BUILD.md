# Build

## DOS application

Use your own Microsoft C 6.0 compiler, headers and libraries, plus DOS LINK. Work in a disposable copy of `src/` with the toolchain available as `C:\BIN`, `C:\INCLUDE` and `C:\LIB`. Run `BUILD.BAT`.

The qualified flags are `/nologo /c /G0 /AS /Os /W3` and linker flags `/NOI /MAP /CP:4096`. The batch builds PAINT.EXE, QA.EXE, MEMQA.EXE and the development capture fixture CAPQA.EXE. Inspect `BUILD.LOG` and `BUILD.OK`; the batch uses markers rather than reliable shell exit codes.

Run QA.EXE and MEMQA.EXE. The historical target smoke sequence was `PAINT /QA /NOMOUSE`, `PAINT /QA`, then `PAINT /QA /NOMOUSE`, checking SMOKE.TXT after each. These scripted tests create temporary files in the current directory. CAPQA is for the configured DOSBox-X integration screenshot interface only, not real hardware.

The original target configuration was DOSBox-X 2025.12.01, Tandy machine, normal core, 8086_prefetch CPU, FPU disabled, DOS 3.30, base memory capped at 640 KB, and EMS/XMS/UMB disabled. Shared-emulator use on Chad's machine needs the established exclusive slot; keep physical cards and other projects' media unmounted.

## Windows host tests

With MinGW GCC available:

```powershell
./scripts/test-host.ps1 -Compiler gcc
```

On Chad's current workspace, use the shared verified GCC launcher required by its AGENTS.md:

```powershell
$taskSavedPath = $env:PATH
try {
    $env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
    ./scripts/test-host.ps1 -GccLauncher "$env:USERPROFILE\Documents\Codex\toolchains\gcc-ucrt64.py"
} finally { $env:PATH = $taskSavedPath }
```

Host executables, temporary art, and build/test logs go under ignored `build/host/`. Host tests exercise shared logic and the production UI through an adapter; they do not replace DOS or hardware testing.
