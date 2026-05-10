#pragma once
#include <cstdint>

struct Wire {
    int  id;
    int  compA, pinA;   // component id + pin index
    int  compB, pinB;
    bool complete = false;

    // Color for this wire (assigned on creation)
    uint8_t r, g, b;
};
