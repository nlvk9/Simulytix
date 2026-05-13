#pragma once
// component.h — defines the hardware component abstraction used throughout the simulator.
// each component models a real-world electronic part: its pins, dimensions, and sim state.
// components are the fundamental building blocks placed on the canvas and wired together.

#include <string>
#include <vector>
#include <cstdint>

// ── pin type ─────────────────────────────────────────────────────────────────
// pin types allow the net-solver and simulation engine to understand the
// electrical role of each pin — important for validation and signal propagation.

enum class PinType
{
    Digital,  // general-purpose gpio (can be input or output)
    Analog,   // analog input capable pin (adc)
    PWM,      // digital pin that also supports pwm output
    Power,    // vcc / power rail pin (3.3v or 5v)
    Ground,   // gnd reference pin
    Anode,    // led / diode positive terminal
    Cathode,  // led / diode negative terminal
    I2C_SDA,  // i2c data line
    I2C_SCL,  // i2c clock line
    SPI_MOSI, // spi master-out slave-in
    SPI_MISO, // spi master-in slave-out
    SPI_SCK,  // spi clock
    SPI_CS,   // spi chip-select
    UART_TX,  // uart transmit
    UART_RX,  // uart receive
    Any       // accepts any connection (generic passive terminals)
};

// returns a human-readable label for a pin type — used in tooltips and debug output
const char *pinTypeName(PinType t);

// ── pin ───────────────────────────────────────────────────────────────────────
// a pin is a named electrical terminal on a component with a position relative
// to the component's origin. the world position is computed from the component's
// (x,y) plus the pin's local offset, accounting for the current zoom level.

struct Pin
{
    std::string id;    // unique id within the component, e.g. "D13", "GND0", "Anode"
    std::string label; // short display label shown next to the pin dot

    PinType type; // electrical role — used for validation and rendering color

    float localX; // horizontal offset from component origin (world units)
    float localY; // vertical offset from component origin (world units)

    bool connected = false; // true if this pin has at least one wire attached

    // convenience: returns whether this pin can carry digital signals
    bool isDigital() const
    {
        return type == PinType::Digital || type == PinType::PWM;
    }

    // convenience: returns whether this pin is a power/ground rail
    bool isPowerRail() const
    {
        return type == PinType::Power || type == PinType::Ground;
    }
};

// ── component kind ────────────────────────────────────────────────────────────
// the kind enum identifies what logical device a component models.
// it drives factory dispatch in addComponent() and the renderer's draw dispatch.

enum class ComponentKind
{
    ArduinoNano,   // arduino nano development board (atmega328p)
    ArduinoUno,    // arduino uno development board (atmega328p, different footprint)
    LED,           // single-color led (anode + cathode)
    Resistor,      // passive resistor with configurable value
    PushButton,    // momentary push-button (normally open)
    Potentiometer, // 3-terminal variable resistor / voltage divider
    Servo,         // pwm-driven servo motor
    UARTTerminal,  // virtual uart terminal device for serial i/o
    I2CDevice      // generic i2c slave device placeholder
};

// returns the display name of a component kind
const char *componentKindName(ComponentKind k);

// ── component ─────────────────────────────────────────────────────────────────
// a component is a placed instance of an electronic part on the canvas.
// it holds its world-space position, dimensions, pin list, and simulation state.
// components are owned by the canvas and referenced by the simulator via id.

struct Component
{
    int id;             // globally unique component id assigned at creation time
    ComponentKind kind; // what type of device this component models
    std::string label;  // human-readable label (e.g. "Arduino Nano", "LED", "220Ω")

    float x, y; // world-space position of the component's top-left corner
    float w, h; // world-space dimensions at zoom=1 (pixels when zoom=1.0)

    std::vector<Pin> pins; // ordered list of electrical terminals on this component

    // ── simulation state ─────────────────────────────────────────────────────
    // these fields are written by the simulator thread and read by the renderer.
    // they use simple booleans/floats rather than atomics because the renderer
    // only reads them — the simulator snapshots are updated atomically elsewhere.

    bool ledOn = false;         // true when the led is being driven high
    bool buttonPressed = false; // true while the push button is held down
    float glowPhase = 0.f;      // phase accumulator for led glow animation
    float analogValue = 0.5f;   // 0..1 normalized analog output (potentiometer)
    float servoAngle = 90.f;    // current servo position in degrees (0..180)
    bool selected = false;      // true when this component is the selected item

    // ── pin world-position helpers ────────────────────────────────────────────
    // returns the absolute world-space x/y of the nth pin
    float pinWorldX(int pinIdx) const { return x + pins[pinIdx].localX; }
    float pinWorldY(int pinIdx) const { return y + pins[pinIdx].localY; }

    // ── factory methods ───────────────────────────────────────────────────────
    // each factory method constructs a fully initialised component of its kind,
    // laying out pins according to the real-world part's physical pin numbering.

    static Component makeArduinoNano(int id, float x, float y);
    static Component makeArduinoUno(int id, float x, float y);
    static Component makeLED(int id, float x, float y);
    static Component makeResistor(int id, float x, float y);
    static Component makePushButton(int id, float x, float y);
    static Component makePotentiometer(int id, float x, float y);
    static Component makeServo(int id, float x, float y);
    static Component makeUARTTerminal(int id, float x, float y);
    static Component makeI2CDevice(int id, float x, float y);
};
