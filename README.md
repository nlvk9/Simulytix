# Simulytix

Simulytix is an accessible Arduino-style circuit code simulator focused on making it easy to design, visualize, and extend digital hardware simulations. The goal is to let users add more sensors, actuators, and hardware modules over time while editing code live in the built-in editor.

## Purpose

This project exists because it is hard to find a single open platform that supports arbitrary hardware devices while also letting developers design the simulator behavior themselves. Simulytix is intended to be:

- easy to open and run in VS Code or another IDE
- approachable for people who want to learn embedded code simulation
- extensible so new devices, sensors, and boards can be added
- open source so contributors can improve support and add hardware modules

## What it currently supports

- Arduino-style code editor with live simulation support
- Drag-and-drop circuit workspace with wiring
- Arduino Nano-style board component
- LED component with anode/cathode terminals
- Resistor and push button components
- Push button interaction with visible terminals and press state
- Undo (Ctrl/Cmd + Z) in the code editor
- Focus management so clicking the simulation canvas unfocuses the editor
- Resizable code editor sidebar

## What is being developed now

Currently this repository is being improved with support for:

- Arduino UNO compatibility
- better overall visuals and UI polish
- support for sensors such as DHT11, BME680, and VL53L0X
- richer library support inside the text editor

## Future plans

Simulytix is planned to grow into a wider platform that may eventually support:

- ESP32
- Teensy / Arduino-compatible Duino boards
- community-contributed device models and libraries
- reusable simulation modules for buttons, sensors, displays, and more

## Getting started in VS Code / IDE

### Prerequisites

- Git
- A C++17-capable compiler
- CMake
- SDL2 and SDL2_ttf
- VS Code (or another C++ IDE)
- Recommended VS Code extensions:
  - C/C++ by Microsoft
  - CMake Tools
  - SDL2 support if desired

### macOS install commands

```bash
brew install cmake sdl2 sdl2_ttf
```

### Clone and open in VS Code

```bash
git clone https://github.com/<your-username>/Simulytix.git
cd Simulytix
code .
```

### Build and run

```bash
mkdir -p build
cd build
cmake ..
cmake --build . --target Simulytix
./Simulytix
```

If you use the VS Code CMake Tools extension, open the workspace and configure the kit, then build and run from the status bar.

## Recommended workflow

1. Open the repository root in VS Code.
2. Configure CMake using the CMake Tools extension.
3. Build the `Simulytix` target.
4. Run the binary from the `build` folder.
5. Edit code in the built-in editor and click `Upload` to simulate.

## How to contribute

1. Fork the repository.
2. Create a feature branch.
3. Add or improve device models, board support, or editor features.
4. Test locally by building and running the app.
5. Submit a pull request with a clear description of the change.

## Repository layout

- `src/` — application source code
- `build/` — generated build files
- `CMakeLists.txt` — project build configuration
- `BUILD.md` — legacy build notes
- `sketch.h` — sample sketch support for the editor

## License

This repository is intended to be open source. A proper open source license file is included.
