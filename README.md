# Novo Bus Analyzer

<p align="center">
  <img src="resources/logo.png">
</p>

1. For setting up the development environment, please refer to the document `development_environment.pdf` occuring under the [BUSMASTER documentation](https://reymor.github.io/documentation/2023/12/06/busmaster-docs.html).

2. This BUSMASTER application is entirely compilable and it is possible to run it in simulation mode by following the instruction set provided in the aforementioned document.

3. In order to build an installation application, please follow the instruction set in the developer's environment document. The install script and the related artifacts occur under `Installation Setup` folder.

# Dependencies

`NBA` has migrated the build system and now it uses `cmake`. However, right now for compilation you shall use `Visual Studio 2019 or 2022`. Qt code has been migrated to `Qt6`.

- cmake >= 3.27
- Visual Studio 2019 or 2022
- Qt6 tested on v6.8.1
- The legacy `Win32` configuration remains available for vendor adapters that
  only ship x86 libraries. Use the x64 instructions below for a modern local
  build.

## Local Windows x64 build and run

The modern Windows x64 build uses vcpkg for `gettext`, `libxml2`, and `zlib`.
Configure a fresh build directory with the Visual Studio generator, the Qt 6
MSVC package, and your vcpkg toolchain:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

cmake -S . -B build_modern `
  -G "Visual Studio 18 2026" -A x64 `
  -DQt6_DIR="C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6" `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

Build either configuration:

```powershell
cmake --build build_modern --config Debug
cmake --build build_modern --config Release
```

The build runs the `deploy_runtime` target automatically. It creates a
self-contained local run folder beside the selected x64 executable, copies the
BUSMASTER modules, Qt runtime and plugins, and the matching vcpkg DLLs, then
registers the deployed x64 `BusEmulation.exe` COM server for the current user
and fails if any deployed DLL is not x64. Do not manually copy DLLs from
`Sources/BUSMASTER/BIN`, because that legacy folder includes x86 vendor DLLs.

The repository does not include the source or an x64 binary for
`DBManager.dll`; its bundled copy is x86-only and is intentionally excluded.
In the x64 build, CAN database import uses the in-repo DBC converter path,
which converts `.dbc` files into the same in-memory `sMESSAGE` / `sSIGNALS`
database structures that the legacy loader populated. J1939 database import
and the LDF Editor still depend on `DBManager.dll` and are disabled with a
clear message. Install an x64-compatible `DBManager.dll` beside the
executable to re-enable those legacy workflows.
The Qt-based LDF Editor and LDF Viewer helpers are excluded from the default
x64 build so the main application can still be built and tested even if Qt
autogen for those tools is unavailable.

Supported x64 CAN database flow:
- `.dbc` files are accepted.
- Messages, signals, start bits, lengths, byte order, scale, offset, min/max,
  units, and enumerated values are preserved through the converter.
- Unsupported database formats fail with a user-visible message instead of a
  silent DLL load error.
- The x64 CAN DBC bridge supports multiple associated CAN DBCs globally on
  CAN channel 0, mirroring the original BUSMASTER CAN DBF behavior.
- It does not use channel-based associations such as `CAN1/FileA.dbc` and
  `CAN2/FileB.dbc`; that remains a future PR if ever needed.
- File dialog multi-select is unrelated to the global association model. Each
  `.dbc` is still associated globally on CAN channel 0.

The deploy step also copies `BUSMASTER.chm` from `Sources/BUSMASTER/BIN/Release`
into the runtime folder so the Help menu and the Test Automation Editor help
link can resolve locally.

## CI Windows Build

The GitHub Actions workflow at `.github/workflows/windows-build.yml` validates
that the modern x64 BUSMASTER build configures and compiles on a clean Windows
runner using the closest available Microsoft toolchain on GitHub-hosted
machines. It installs Qt 6.8.3 through `jurplel/install-qt-action`.

It currently checks:
- Release x64 build only
- runtime deployment for `NovoBusAnalyzer.exe`
- deployment of `DBC2DBFConverter.dll` and `DBC2DBFConverterLibrary.dll`
- x64 PE architecture for `DBC2DBFConverter.dll`
- presence of the CAN smoke sample DBC files
- creation of a standalone portable ZIP artifact
- creation of an NSIS installer artifact

Debug x64 is intentionally left as a future TODO for the workflow so the CI
job stays fast and focused on the shipping runtime path.

It does not run the GUI interactively and it does not validate the
DBManager-backed J1939, LDF, or Test Automation database workflows.

The workflow publishes two artifacts:
- the NSIS installer for normal installation
- a portable ZIP that can be unzipped and run directly without installing

The portable ZIP includes the Release runtime folder and the CAN smoke sample
DBCs under `Samples\CAN`. Like the normal runtime, it may still require a
compatible Windows runtime and/or VC++ redistributable on a clean machine if
those dependencies are not already present or bundled.

Run the selected configuration directly:

```powershell
.\build_modern\Sources\BUSMASTER\Application\Debug\NovoBusAnalyzer.exe
.\build_modern\Sources\BUSMASTER\Application\Release\NovoBusAnalyzer.exe
```

Quick smoke checklist:

1. Start the matching `Debug` or `Release` exe from `build_modern`.
2. Open `Format Converter` and confirm at least one converter tab is loaded.
3. Open `Test Automation Editor` and verify the editor window appears.
4. Open `Help` and confirm `BUSMASTER.chm` launches.
5. Click `J1939 -> Activate` and confirm the trace window reports progress
   instead of freezing.
6. Open the diagnostics and LIN schedule dialogs and confirm they appear
   without an MFC assertion.
