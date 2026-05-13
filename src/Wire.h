#pragma once
// wire.h — represents a single electrical connection between two component pins.
// wires are the fundamental connectivity primitive in the net-list system.
// each wire stores both endpoint references and a visual color for rendering.

#include <cstdint>

// wire routing style controls how the visual path is drawn on the canvas.
// orthogonal uses right-angle elbows (more realistic pcb style).
// direct draws a straight diagonal line (simpler, less cluttered).
enum class WireRouting
{
    Orthogonal, // standard pcb-style right-angle routing
    Direct      // straight-line routing for simple connections
};

// wire — a completed or in-progress electrical connection.
// compa/pina and compb/pinb identify the two endpoints by component id and pin index.
// wires become "complete" only when both endpoints are resolved.
struct Wire
{
    int id;
    int compA; // source component id
    int pinA;  // source pin index within compa's pin list
    int compB; // destination component id
    int pinB;  // destination pin index within compb's pin list

    bool complete = false;

    // visual color assigned at creation time — cycles through a predefined palette
    uint8_t r = 200;
    uint8_t g = 100;
    uint8_t b = 100;

    // routing style for this wire — defaults to orthogonal (pcb-like)
    WireRouting routing = WireRouting::Orthogonal;

    // electrical state — driven high/low or floating (for future simulation depth)
    enum class State
    {
        Floating, // no driver is asserting this net
        Low,      // driven to logic 0 (gnd)
        High      // driven to logic 1 (vcc)
    } electricalState = State::Floating;
};