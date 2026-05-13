# # Build Notes

This repository builds a Qt-based desktop circuit simulator called `Simulytix`.

## Current build requirements

- CMake 3.16 or newer
- C++17-capable compiler
- Qt5 modules: Core, Widgets, Network
- Qt development files and headers

The app is built with Qt and links against `Qt5::Core`, `Qt5::Widgets`, and `Qt5::Network`.

## Linux / Ubuntu

```bash
sudo apt-get install -y cmake build-essential qtbase5-dev qttools5-dev-tools
```

## macOS

```bash
brew install cmake qt@5
```

If Qt is installed in a non-standard location, pass `CMAKE_PREFIX_PATH`:

```bash
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
```

## Windows

Install Qt5 and a compatible CMake toolchain, then configure from the Visual Studio or MinGW environment.

## Build and run

```bash
mkdir -p build
cd build
cmake ..
cmake --build . --target Simulytix
./Simulytix
```

On Windows, run `Simulytix.exe` from the build directory.

## Notes

- The UI is implemented in Qt Widgets.
- The simulator ships with a built-in code editor and AI chat panel.
- The AI panel uses Qt Network to send requests to the Groq OpenAI-compatible endpoint.
- The main application entry point is `src/main.cpp`.
