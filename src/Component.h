#pragma once
#include <string>
#include <vector>
#include <cstdint>

// ── Pin ──────────────────────────────────────────────────────────────────────

enum class PinType
{
    Digital,
    Power,
    Ground,
    Anode,
    Cathode,
    Any
};

struct Pin
{
    std::string id; // e.g. "D13", "GND", "Anode"
    std::string label;
    PinType type;
    float localX; // offset from component origin
    float localY;
    bool connected = false;

    // World position (computed each frame)
    float worldX() const;
    float worldY() const;
};

// ── ComponentKind ─────────────────────────────────────────────────────────────

enum class ComponentKind
{
    ArduinoNano,
    LED,
    Resistor,
    PushButton
};

// ── Component ─────────────────────────────────────────────────────────────────

struct Component
{
    int id;
    ComponentKind kind;
    std::string label;
    float x, y; // canvas position (world coords)
    float w, h; // size in pixels at zoom=1
    std::vector<Pin> pins;

    // Simulation state
    bool ledOn = false;         // for LED
    bool buttonPressed = false; // for PushButton
    float glowPhase = 0.f;

    // Returns world-space pin position
    float pinWorldX(int pinIdx) const { return x + pins[pinIdx].localX; }
    float pinWorldY(int pinIdx) const { return y + pins[pinIdx].localY; }

    static Component makeArduinoNano(int id, float x, float y);
    static Component makeLED(int id, float x, float y);
    static Component makeResistor(int id, float x, float y);
    static Component makePushButton(int id, float x, float y);
};
