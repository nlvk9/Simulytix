#include "Component.h"

// ── Arduino Nano ──────────────────────────────────────────────────────────────
// Pins along left and right edges, like a real Nano DIP footprint

const char *componentKindName(ComponentKind k)
{
    switch (k)
    {
    case ComponentKind::ArduinoNano:
        return "Arduino Nano";
    case ComponentKind::ArduinoUno:
        return "Arduino Uno";
    case ComponentKind::LED:
        return "LED";
    case ComponentKind::Resistor:
        return "Resistor";
    case ComponentKind::PushButton:
        return "Push Button";
    case ComponentKind::Potentiometer:
        return "Potentiometer";
    case ComponentKind::Servo:
        return "Servo";
    case ComponentKind::UARTTerminal:
        return "UART Terminal";
    case ComponentKind::I2CDevice:
        return "I2C Device";
    default:
        return "Unknown";
    }
}

Component Component::makeArduinoNano(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::ArduinoNano;
    c.label = "Arduino Nano";
    c.x = x;
    c.y = y;
    c.w = 140; // Wider to accommodate more pins
    c.h = 300; // Taller for all pins

    // Arduino Nano pin layout (30 pins total)
    // Left side (15 pins, top to bottom), outside the board body
    const float lx = -10.f;
    float ly = 15.f;
    float step = 18.f;

    auto lpin = [&](std::string pid, std::string lbl, PinType t)
    {
        c.pins.push_back({pid, lbl, t, lx, ly});
        ly += step;
    };

    // Left side pins
    lpin("D13", "D13", PinType::Digital);
    lpin("3V3", "3V3", PinType::Power);
    lpin("AREF", "AREF", PinType::Any);
    lpin("A0", "A0", PinType::Analog);
    lpin("A1", "A1", PinType::Analog);
    lpin("A2", "A2", PinType::Analog);
    lpin("A3", "A3", PinType::Analog);
    lpin("A4", "A4", PinType::I2C_SDA);
    lpin("A5", "A5", PinType::I2C_SCL);
    lpin("A6", "A6", PinType::Analog);
    lpin("A7", "A7", PinType::Analog);
    lpin("5V", "5V", PinType::Power);
    lpin("RST", "RST", PinType::Any);
    lpin("GND", "GND", PinType::Ground);
    lpin("VIN", "VIN", PinType::Power);

    // Right side (15 pins, top to bottom), outside the board body
    const float rx = c.w + 10.f;
    float ry = 15.f;

    auto rpin = [&](std::string pid, std::string lbl, PinType t)
    {
        c.pins.push_back({pid, lbl, t, rx, ry});
        ry += step;
    };

    // Right side pins
    rpin("D12", "D12", PinType::Digital);
    rpin("D11", "D11", PinType::PWM);
    rpin("D10", "D10", PinType::PWM);
    rpin("D9", "D9", PinType::PWM);
    rpin("D8", "D8", PinType::Digital);
    rpin("D7", "D7", PinType::Digital);
    rpin("D6", "D6", PinType::PWM);
    rpin("D5", "D5", PinType::PWM);
    rpin("D4", "D4", PinType::Digital);
    rpin("D3", "D3", PinType::PWM);
    rpin("D2", "D2", PinType::Digital);
    rpin("GND2", "GND", PinType::Ground);
    rpin("RST2", "RST", PinType::Any);
    rpin("D1", "D1", PinType::UART_TX);
    rpin("D0", "D0", PinType::UART_RX);

    return c;
}

// ── LED ───────────────────────────────────────────────────────────────────────

Component Component::makeLED(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::LED;
    c.label = "LED";
    c.x = x;
    c.y = y;
    c.w = 50;
    c.h = 70;

    // Anode (+) at top, Cathode (-) at bottom
    // Pins are positioned relative to component center (25, 35)
    c.pins.push_back({"Anode", "+", PinType::Anode, 25.f, -10.f});
    c.pins.push_back({"Cathode", "-", PinType::Cathode, 25.f, 80.f});

    return c;
}

// ── Resistor ──────────────────────────────────────────────────────────────────

Component Component::makeResistor(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::Resistor;
    c.label = "220Ω";
    c.x = x;
    c.y = y;
    c.w = 20;
    c.h = 60;

    c.pins.push_back({"R_A", "A", PinType::Any, 10.f, -10.f});
    c.pins.push_back({"R_B", "B", PinType::Any, 10.f, c.h + 10.f});

    return c;
}

// ── PushButton ────────────────────────────────────────────────────────────────

Component Component::makePushButton(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::PushButton;
    c.label = "Button";
    c.x = x;
    c.y = y;
    c.w = 40;
    c.h = 40;

    // Four legs: VCC (left), GND (right), Data (bottom), Unlabeled (top)
    c.pins.push_back({"BTN_VCC", "VCC", PinType::Power, -10.f, 10.f});
    c.pins.push_back({"BTN_GND", "GND", PinType::Ground, c.w + 10.f, 10.f});
    c.pins.push_back({"BTN_DATA", "DATA", PinType::Digital, 20.f, c.h + 10.f});
    c.pins.push_back({"BTN_NC", "", PinType::Any, 20.f, -10.f});

    return c;
}

// ── Arduino UNO ───────────────────────────────────────────────────────────────
// Similar to Nano but different footprint and pin layout

Component Component::makeArduinoUno(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::ArduinoUno;
    c.label = "Arduino UNO";
    c.x = x;
    c.y = y;
    c.w = 180; // Wider board
    c.h = 280; // Tall enough for all side pins (14 pins × 18px + margin)

    // Left side pins: D0–D13 (digital), top to bottom
    const float lx = -10.f;
    float ly = 20.f;
    const float step = 18.f;

    auto lpin = [&](std::string pid, std::string lbl, PinType t)
    {
        c.pins.push_back({pid, lbl, t, lx, ly});
        ly += step;
    };

    lpin("D0", "D0", PinType::UART_RX);
    lpin("D1", "D1", PinType::UART_TX);
    lpin("D2", "D2", PinType::Digital);
    lpin("D3", "D3", PinType::PWM);
    lpin("D4", "D4", PinType::Digital);
    lpin("D5", "D5", PinType::PWM);
    lpin("D6", "D6", PinType::PWM);
    lpin("D7", "D7", PinType::Digital);
    lpin("D8", "D8", PinType::Digital);
    lpin("D9", "D9", PinType::PWM);
    lpin("D10", "D10", PinType::SPI_CS);
    lpin("D11", "D11", PinType::SPI_MOSI);
    lpin("D12", "D12", PinType::SPI_MISO);
    lpin("D13", "D13", PinType::Digital);

    // Right side pins: power + analog, top to bottom
    const float rx = c.w + 10.f;
    float ry = 20.f;

    auto rpin = [&](std::string pid, std::string lbl, PinType t)
    {
        c.pins.push_back({pid, lbl, t, rx, ry});
        ry += step;
    };

    rpin("VIN", "VIN", PinType::Power);
    rpin("GND0", "GND", PinType::Ground);
    rpin("GND1", "GND", PinType::Ground);
    rpin("5V", "5V", PinType::Power);
    rpin("3V3", "3V3", PinType::Power);
    rpin("RST", "RST", PinType::Any);
    rpin("AREF", "AREF", PinType::Any);
    rpin("A0", "A0", PinType::Analog);
    rpin("A1", "A1", PinType::Analog);
    rpin("A2", "A2", PinType::Analog);
    rpin("A3", "A3", PinType::Analog);
    rpin("A4", "A4", PinType::I2C_SDA);
    rpin("A5", "A5", PinType::I2C_SCL);

    return c;
}

// ── Potentiometer ─────────────────────────────────────────────────────────────

Component Component::makePotentiometer(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::Potentiometer;
    c.label = "Potentiometer";
    c.x = x;
    c.y = y;
    c.w = 60;
    c.h = 40;

    // Three terminals: left, wiper, right
    c.pins.push_back({"POT_L", "L", PinType::Any, -10.f, 20.f});
    c.pins.push_back({"POT_W", "W", PinType::Any, 30.f, -10.f}); // Top
    c.pins.push_back({"POT_R", "R", PinType::Any, c.w + 10.f, 20.f});

    return c;
}

// ── Servo ─────────────────────────────────────────────────────────────────────

Component Component::makeServo(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::Servo;
    c.label = "Servo";
    c.x = x;
    c.y = y;
    c.w = 50;
    c.h = 60;

    // Three wires: VCC (left), Data (center), GND (right); legs extend below the body
    c.pins.push_back({"SERVO_VCC", "VCC", PinType::Power, 10.f, c.h + 10.f});
    c.pins.push_back({"SERVO_DATA", "DATA", PinType::PWM, 25.f, c.h + 10.f});
    c.pins.push_back({"SERVO_GND", "GND", PinType::Ground, 40.f, c.h + 10.f});

    return c;
}

// ── UART Terminal ─────────────────────────────────────────────────────────────

Component Component::makeUARTTerminal(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::UARTTerminal;
    c.label = "UART Terminal";
    c.x = x;
    c.y = y;
    c.w = 120;
    c.h = 80;

    // RX and TX pins
    c.pins.push_back({"UART_RX", "RX", PinType::UART_RX, 0.f, 40.f});
    c.pins.push_back({"UART_TX", "TX", PinType::UART_TX, c.w, 40.f});

    return c;
}

// ── I2C Device ────────────────────────────────────────────────────────────────

Component Component::makeI2CDevice(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::I2CDevice;
    c.label = "I2C Device";
    c.x = x;
    c.y = y;
    c.w = 80;
    c.h = 60;

    // SDA and SCL pins
    c.pins.push_back({"I2C_SDA", "SDA", PinType::I2C_SDA, 0.f, 30.f});
    c.pins.push_back({"I2C_SCL", "SCL", PinType::I2C_SCL, c.w, 30.f});
    c.pins.push_back({"I2C_VCC", "VCC", PinType::Power, 20.f, 0.f});
    c.pins.push_back({"I2C_GND", "GND", PinType::Ground, 60.f, 0.f});

    return c;
}
