# Simulytix — Arduino Nano Blink Demo

## What it does
Opens a desktop window showing a single LED (circle) that visibly blinks ON/OFF
every 500ms, driven by a simulated Arduino Nano running blink firmware.

## Build instructions

### Dependencies
- CMake 3.16+
- SDL2 (`libsdl2-dev` on Ubuntu/Debian)
- C++17 compiler (GCC or Clang)

### Ubuntu/Debian
```bash
sudo apt-get install -y libsdl2-dev cmake build-essential
```

### Build & Run
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
./Simulytix
```

Or on Windows the binary is `Simulytix.exe`.

## Architecture

```
Blink Firmware (setup/loop)
        │
        ▼
VirtualArduinoNano (wraps SimGPIO + SimClock)
        │  pinMode(13, OUTPUT)
        │  digitalWrite(13, HIGH/LOW)
        │  delay(500)
        ▼
    SimGPIO ──── Wire ───▶ LED Anode (D13)
                               │
                           Cathode → GND (pin 0, always LOW)
                               │
                           LEDComponent.isOn (atomic bool)
                               │
                           SDL2 Renderer (60 FPS)
                               │
                        ┌──────────────┐
                        │  ● LED ON    │  bright amber circle + glow
                        │  ○ LED OFF   │  dark circle
                        └──────────────┘
```

### Components
- **VirtualArduinoNano** — wraps SimGPIO/SimClock, exposes D0–D13 with pinMode/digitalWrite/digitalRead
- **SimGPIO** — tracks pin modes and states, fires change callbacks
- **SimClock** — deterministic sim time; `delay()` advances it, no real-time sleep
- **Wire** — D13 → LED Anode (same GPIO model, direct observation)
- **LEDComponent** — logic: ON if Anode=HIGH AND Cathode=LOW
- **FirmwareRunner** — calls `firmware_setup()` once, then `firmware_loop()` in a tight loop
- **SimulationContext** — runs sim in a background thread, updates LED state atomically
- **SDL2 UI** — reads LED state at 60 FPS, renders glowing circle

### Blink firmware (embedded in simulation)
```cpp
void setup() {
    pinMode(13, OUTPUT);
}
void loop() {
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
}
```

All `pinMode`, `digitalWrite`, `delay`, and `millis` calls are macro-shimmed
to forward to the active `VirtualArduinoNano` instance.
