# Development setup

## Prerequisites

- Qt 6.8 with `Quick` and `QuickControls2` modules.
- CMake 3.25 or later.
- A C++20 compiler: MSVC 2022, Apple Clang, GCC 11+, or Clang 14+.

## Configure and build

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8/<compiler>
cmake --build build --config Release
```

Run `build/marknote` on Linux, `build/Release/marknote.exe` on Windows, or the
generated application bundle on macOS.

The build fetches pinned `cmark-gfm` through CMake FetchContent and links it
into the `marknote_markdown` library (HTML render, syntax highlighter, and
project Document AST).

### Windows with MSVC

The repository includes a helper that loads the Visual Studio developer
environment before configuring Ninja. Its default Qt path is the current local
development installation, `D:\Qt\6.10.0\msvc2022_64`:

```bat
scripts\configure-msvc.bat
scripts\build-msvc.bat
```

Pass the Qt root and Visual Studio root as the first and second arguments when
the local installation differs. Both scripts use the same argument order.

To generate a Visual Studio solution instead of using Ninja:

```bat
scripts\configure-visual-studio.bat
scripts\build-visual-studio.bat
```

Open `build-vs2022\Marknote.sln`. Do not reuse a build directory that was
configured with a different Qt compiler ABI; in particular, an MSVC generator
cannot use a Qt `mingw_64` installation.

## Windows runtime deployment

Windows builds run `windeployqt` after linking `marknote`. Required Qt DLLs,
platform plugins, QML modules, and compiler runtime files are copied beside the
executable automatically. This behavior is controlled by the CMake option
`MARKNOTE_DEPLOY_QT`, which defaults to `ON` on Windows:

```sh
cmake -S . -B <build-dir> -DMARKNOTE_DEPLOY_QT=OFF
```

Disabling it is useful for fast inner-loop builds when Qt is already available
through `PATH`; release and distributable builds must leave it enabled.
