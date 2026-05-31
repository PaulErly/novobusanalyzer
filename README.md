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
The x64 application runs without database import support. Install an
x64-compatible `DBManager.dll` beside the executable to enable database import.

Run the selected configuration directly:

```powershell
.\build_modern\Sources\BUSMASTER\Application\Debug\NovoBusAnalyzer.exe
.\build_modern\Sources\BUSMASTER\Application\Release\NovoBusAnalyzer.exe
```
