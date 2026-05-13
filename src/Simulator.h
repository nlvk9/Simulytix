#pragma once
#include "Component.h"
#include "Wire.h"
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>

// ── Arduino API Interface ───────────────────────────────────────────────────
struct ArduinoAPI
{
    std::function<void(uint8_t, uint8_t)> pinMode;
    std::function<void(uint8_t, uint8_t)> digitalWrite;
    std::function<void(uint32_t)> delay;
    std::function<uint8_t(uint8_t)> digitalRead;
    std::function<void(uint8_t, uint8_t)> analogWrite;
    std::function<uint16_t(uint8_t)> analogRead;
};

// ── Net ───────────────────────────────────────────────────────────────────────

struct NetEndpoint
{
    int compId;
    std::string pinId;
};

struct Net
{
    int id;
    std::vector<NetEndpoint> endpoints;

    bool hasPin(int compId, const std::string &pinId) const
    {
        for (auto &e : endpoints)
            if (e.compId == compId && e.pinId == pinId)
                return true;
        return false;
    }
};

// ── SimulatorState ────────────────────────────────────────────────────────────

enum class SimState
{
    Idle,
    Running,
    Error
};

// ── Simulator ─────────────────────────────────────────────────────────────────

class Simulator
{
public:
    Simulator();
    ~Simulator();

    void start(const std::string &sketchCode,
               const std::vector<Component> &components,
               const std::vector<Wire> &wires);

    void stop();

    SimState state() const { return m_state.load(); }
    std::string errorMsg() const { return m_error; }

    bool ledIsOn(int compId) const;

    void tick(float dt);

    // ── Arduino-style API implementations ──
    void api_pinMode(uint8_t pin, uint8_t mode);
    void api_digitalWrite(uint8_t pin, uint8_t val);
    uint8_t api_digitalRead(uint8_t pin);
    void api_analogWrite(uint8_t pin, uint8_t val);
    uint16_t api_analogRead(uint8_t pin);
    void api_delay(uint32_t ms);

private:
    void parseSketch(const std::string &code);
    void buildNets(const std::vector<Component> &components,
                   const std::vector<Wire> &wires);

    bool validateLEDCircuit(int ledCompId,
                            const std::vector<Component> &components,
                            std::string &outDrivingPin) const;

    void simThread();

    // Sketch execution with API context
    void executeSketch(const ArduinoAPI &api);

    // Parsed values from sketch
    int m_ledPin = 13;
    int m_delayHigh = 500;
    int m_delayLow = 500;

    struct SketchStep { bool high; uint32_t delayMs; };
    std::vector<SketchStep> m_sequence;

    // ── Nets ───────────────────────────────────────────
    std::vector<Net> m_nets;

    struct LEDBinding
    {
        int ledCompId;
        int arduinoPin;
        int nanoCompId;
    };

    std::vector<LEDBinding> m_ledBindings;

    std::atomic<uint8_t> m_pinModes[64];
    std::atomic<uint8_t> m_pinStates[64];
    std::atomic<uint8_t> m_analogStates[64]; // Added for PWM

    std::vector<std::pair<int, std::atomic<bool> *>> m_ledOn;

    std::atomic<SimState> m_state{SimState::Idle};
    std::atomic<bool> m_running{false};

    std::string m_error;
    std::string m_sketchCode;

    std::thread m_thread;

    std::vector<Component> m_components;
};