# Simulytix

Simulytix is a desktop Arduino-style circuit simulator that combines a visual wiring workspace, an embedded sketch editor, and an AI-powered help panel. It is built with Qt and lets you design simple circuits, write Arduino-style firmware, and simulate the result in a single app.

## What Simulytix does now

- Provides a visual circuit canvas for placing components and drawing wires
- Supports Arduino Nano and Arduino UNO board footprints with real pin layouts
- Includes LED, resistor, push button, potentiometer, and servo components
- Simulates Arduino-style sketches using `setup()`, `loop()`, `pinMode()`, `digitalWrite()`, `analogWrite()`, `delay()`, `digitalRead()`, and `analogRead()` patterns
- Binds output pins to connected LEDs and updates visual state in the circuit
- Lets you write code in the built-in editor and upload it into the simulator
- Includes a stop control to halt the simulation at any time
- Ships with an integrated AI assistant panel that uses the Groq chat API for circuit guidance
- Sends the current sketch and component list as context to the AI assistant for relevant guidance

## Key features

- Desktop Qt UI with split view: canvas on the left, editor and AI assistant on the right
- Drag-and-drop circuit assembling with visible wire connections
- Intelligent circuit simulation for Arduino-style pin control and LED behavior
- AI chat panel for asking questions about your current circuit or requesting wiring/code tips
- Simple workflow: place components, wire them, write code, then upload to simulate

## Supported components

- Arduino Nano
- Arduino UNO
- LED
- Resistor
- Push button
- Potentiometer
- Servo

## How it works

1. Place one or more components on the canvas.
2. Draw wires between component pins to form circuits.
3. Write Arduino-style C++ code in the editor.
4. Click `Upload` to begin simulation.
5. The simulator parses the sketch and maps pin outputs to connected components.
6. LED components light or dim according to the simulated pin state.
7. The AI panel can review your sketch and circuitry when you click `Upload`.

## AI assistant

The AI assistant is built into the right-hand panel and uses Groq's OpenAI-compatible chat API.

### Setup

1. Sign up at `https://console.groq.com`
2. Create a free API key
3. Export it before launching Simulytix:

```bash
export GROQ_API_KEY=gsk_...
```

If the key is missing, the AI panel displays setup instructions inside the app.

## Build dependencies

- CMake 3.16 or newer
- C++17-capable compiler
- Qt5 with Core, Widgets, and Network modules

### Linux / Ubuntu

```bash
sudo apt-get install -y cmake build-essential qtbase5-dev qttools5-dev-tools
```

### macOS

```bash
brew install cmake qt@5
```

### Windows

Install Qt5 and a compatible CMake toolchain; then configure from the Visual Studio or MinGW environment.

## Build and run

```bash
mkdir -p build
cd build
cmake ..
cmake --build . --target Simulytix
./Simulytix
```

On Windows, run `Simulytix.exe` from the build directory.

If Qt is installed in a non-standard location on macOS or Linux, add the prefix path:

```bash
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
```

## Recommended workflow

1. Open the repository in VS Code or another IDE.
2. Configure CMake and select a supported build kit.
3. Build the `Simulytix` target.
4. Run the application from the build output directory.
5. Place components, wire them, write code, and upload to simulate.

## Contributing

1. Fork the repository and create a feature branch.
2. Add or improve board models, component simulations, editor features, or UI polish.
3. Test your changes locally by building and running the app.
4. Open a pull request with a clear description of the behavior change.

## Repository layout

- `src/` — application source code
- `CMakeLists.txt` — project build configuration
- `BUILD.md` — legacy build notes
- `sketch.h` — sample sketch support for the embedded editor

## License

This repository includes an open source license file. See `LICENSE` for details.
