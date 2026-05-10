#include "Component.h"

// ── Arduino Nano ──────────────────────────────────────────────────────────────
// Pins along left and right edges, like a real Nano DIP footprint

Component Component::makeArduinoNano(int id, float x, float y)
{
    Component c;
    c.id = id;
    c.kind = ComponentKind::ArduinoNano;
    c.label = "Arduino Nano";
    c.x = x;
    c.y = y;
    c.w = 120;
    c.h = 260;

    // Left side pins (top to bottom)
    const float lx = 0.f;
    float ly = 20.f;
    float step = 20.f;

    auto lpin = [&](std::string pid, std::string lbl, PinType t)
    {
        c.pins.push_back({pid, lbl, t, lx, ly});
        ly += step;
    };

    lpin("D1", "D1/TX", PinType::Digital);
    lpin("D0", "D0/RX", PinType::Digital);
    lpin("NRST", "RST", PinType::Any);
    lpin("GND0", "GND", PinType::Ground);
    lpin("D2", "D2", PinType::Digital);
    lpin("D3", "D3~", PinType::Digital);
    lpin("D4", "D4", PinType::Digital);
    lpin("D5", "D5~", PinType::Digital);
    lpin("D6", "D6~", PinType::Digital);
    lpin("D7", "D7", PinType::Digital);
    lpin("D8", "D8", PinType::Digital);
    lpin("D9", "D9~", PinType::Digital);

    // Right side pins (top to bottom)
    const float rx = c.w;
    float ry = 20.f;

    auto rpin = [&](std::string pid, std::string lbl, PinType t)
    {
        c.pins.push_back({pid, lbl, t, rx, ry});
        ry += step;
    };

    rpin("D13", "D13", PinType::Digital);
    rpin("3V3", "3.3V", PinType::Power);
    rpin("AREF", "AREF", PinType::Any);
    rpin("A0", "A0", PinType::Digital);
    rpin("A1", "A1", PinType::Digital);
    rpin("A2", "A2", PinType::Digital);
    rpin("A3", "A3", PinType::Digital);
    rpin("A4", "A4/SDA", PinType::Digital);
    rpin("A5", "A5/SCL", PinType::Digital);
    rpin("A6", "A6", PinType::Digital);
    rpin("A7", "A7", PinType::Digital);
    rpin("5V", "5V", PinType::Power);
    rpin("RST2", "RST", PinType::Any);
    rpin("GND1", "GND", PinType::Ground);
    rpin("VIN", "VIN", PinType::Power);

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

    // Anode (+) top, Cathode (-) bottom
    c.pins.push_back({"Anode", "+", PinType::Anode, 25.f, 0.f});
    c.pins.push_back({"Cathode", "-", PinType::Cathode, 25.f, 70.f});

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

    c.pins.push_back({"R_A", "A", PinType::Any, 10.f, 0.f});
    c.pins.push_back({"R_B", "B", PinType::Any, 10.f, 60.f});

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

    c.pins.push_back({"BTN_A", "A", PinType::Any, 0.f, 20.f});
    c.pins.push_back({"BTN_B", "B", PinType::Any, 40.f, 20.f});

    return c;
}
