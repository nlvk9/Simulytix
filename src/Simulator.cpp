#include "Simulator.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <regex>

// ── Arduino-style constants ─────────────────────────────────────────────────
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t INPUT = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t LOW = 0;

// ── Thread-local API context ───────────────────────────────────────────────
thread_local ArduinoAPI *g_api = nullptr;

// ── Arduino API implementations ────────────────────────────────────────────
inline void pinMode(uint8_t p, uint8_t m)
{
    if (g_api && g_api->pinMode)
        g_api->pinMode(p, m);
}

inline void digitalWrite(uint8_t p, uint8_t v)
{
    if (g_api && g_api->digitalWrite)
        g_api->digitalWrite(p, v);
}

inline uint8_t digitalRead(uint8_t p)
{
    if (g_api && g_api->digitalRead)
        return g_api->digitalRead(p);
    return LOW;
}

inline void analogWrite(uint8_t p, uint8_t v)
{
    if (g_api && g_api->analogWrite)
        g_api->analogWrite(p, v);
}

inline uint16_t analogRead(uint8_t p)
{
    if (g_api && g_api->analogRead)
        return g_api->analogRead(p);
    return 0;
}

inline void delay(uint32_t ms)
{
    if (g_api && g_api->delay)
        g_api->delay(ms);
}

// ── User sketch ─────────────────────────────────────────────────────────────

// ────────────────────────────────────────────────────────────────────────────
// Simulator Implementation
// ────────────────────────────────────────────────────────────────────────────

Simulator::Simulator()
{
    for (auto &m : m_pinModes)
        m.store(INPUT);
    for (auto &s : m_pinStates)
        s.store(LOW);
    for (auto &a : m_analogStates)
        a.store(0);
}

Simulator::~Simulator()
{
    stop();
}

void Simulator::stop()
{
    m_running.store(false);
    if (m_thread.joinable())
        m_thread.join();
    m_state.store(SimState::Idle);

    for (auto &p : m_ledOn)
        delete p.second;

    m_ledOn.clear();
}

void Simulator::start(const std::string &sketchCode,
                      const std::vector<Component> &components,
                      const std::vector<Wire> &wires)
{
    stop();

    m_sketchCode = sketchCode;
    m_components = components;
    m_error = "";

    for (auto &m : m_pinModes)
        m.store(0);
    for (auto &s : m_pinStates)
        s.store(0);

    buildNets(components, wires);

    parseSketch(sketchCode);

    m_ledBindings.clear();

    for (auto &c : components)
    {
        if (c.kind != ComponentKind::LED)
            continue;

        std::string drivingPin;
        if (!validateLEDCircuit(c.id, components, drivingPin))
            continue;

        int pinNum = -1;
        if (drivingPin.size() > 1 && drivingPin[0] == 'D')
            pinNum = std::stoi(drivingPin.substr(1));

        if (pinNum < 0 || pinNum >= 64)
            continue;

        int arduinoId = -1;
        for (auto &c2 : components)
            if (c2.kind == ComponentKind::ArduinoNano ||
                c2.kind == ComponentKind::ArduinoUno)
                arduinoId = c2.id;

        m_ledBindings.push_back({c.id, pinNum, arduinoId});
        m_ledOn.push_back({c.id, new std::atomic<bool>(false)});
    }

    m_running.store(true);
    m_state.store(SimState::Running);

    m_thread = std::thread(&Simulator::simThread, this);
}

// ── Net building (unchanged) ────────────────────────────────────────────────
void Simulator::buildNets(const std::vector<Component> &components,
                          const std::vector<Wire> &wires)
{
    m_nets.clear();

    int nextId = 0;

    for (auto &w : wires)
    {
        if (!w.complete)
            continue;

        const Component *cA = nullptr;
        const Component *cB = nullptr;

        for (auto &c : components)
        {
            if (c.id == w.compA)
                cA = &c;
            if (c.id == w.compB)
                cB = &c;
        }

        if (!cA || !cB)
            continue;
        if (w.pinA >= (int)cA->pins.size())
            continue;
        if (w.pinB >= (int)cB->pins.size())
            continue;

        NetEndpoint epA{cA->id, cA->pins[w.pinA].id};
        NetEndpoint epB{cB->id, cB->pins[w.pinB].id};

        int netA = -1, netB = -1;

        for (int i = 0; i < (int)m_nets.size(); i++)
        {
            if (m_nets[i].hasPin(epA.compId, epA.pinId))
                netA = i;
            if (m_nets[i].hasPin(epB.compId, epB.pinId))
                netB = i;
        }

        if (netA == -1 && netB == -1)
        {
            Net n;
            n.id = nextId++;
            n.endpoints = {epA, epB};
            m_nets.push_back(n);
        }
        else if (netA == -1)
        {
            m_nets[netB].endpoints.push_back(epA);
        }
        else if (netB == -1)
        {
            m_nets[netA].endpoints.push_back(epB);
        }
        else if (netA != netB)
        {
            m_nets[netA].endpoints.insert(
                m_nets[netA].endpoints.end(),
                m_nets[netB].endpoints.begin(),
                m_nets[netB].endpoints.end());
            m_nets.erase(m_nets.begin() + netB);
        }
    }
}

// ── Parse sketch code ───────────────────────────────────────────────────────
void Simulator::parseSketch(const std::string &code)
{
    // Default values
    m_ledPin = 13;
    m_delayHigh = 500;
    m_delayLow = 500;

    // Try to parse LED_PIN constant first
    std::regex pinConstRegex(R"(const\s+\w+\s+LED_PIN\s*=\s*(\d+);)");
    std::smatch pinConstMatch;
    if (std::regex_search(code, pinConstMatch, pinConstRegex))
    {
        m_ledPin = std::stoi(pinConstMatch[1]);
    }
    else
    {
        // Fall back: find pinMode(N, OUTPUT) in setup to determine the pin
        std::regex pinModeRegex(R"(pinMode\s*\(\s*(\d+)\s*,\s*OUTPUT\s*\))");
        std::smatch pinModeMatch;
        if (std::regex_search(code, pinModeMatch, pinModeRegex))
        {
            m_ledPin = std::stoi(pinModeMatch[1]);
        }
    }

    // Parse the digitalWrite sequence in loop to determine HIGH/LOW pattern
    // Find the loop() body
    std::regex loopRegex(R"(void\s+loop\s*\(\s*\)\s*\{([^}]*(?:\{[^}]*\}[^}]*)*)\})");
    std::smatch loopMatch;
    std::string loopBody;
    if (std::regex_search(code, loopMatch, loopRegex))
        loopBody = loopMatch[1];
    else
        loopBody = code; // fall back to whole code

    // Collect all digitalWrite and delay calls in order
    // Pattern: digitalWrite(pin, HIGH/LOW) or delay(ms)
    std::regex stmtRegex(R"(digitalWrite\s*\(\s*\d+\s*,\s*(HIGH|LOW)\s*\)|delay\s*\(\s*(\d+)\s*\))");
    std::sregex_iterator iter(loopBody.begin(), loopBody.end(), stmtRegex);
    std::sregex_iterator end;

    // Build ordered sequence of {isHigh, delayMs} pairs
    m_sequence.clear();
    bool lastWasHigh = true;
    bool lastWasWrite = false;
    uint32_t pendingDelay = 0;

    for (; iter != end; ++iter)
    {
        std::smatch m = *iter;
        if (m[1].matched)
        {
            // digitalWrite
            if (lastWasWrite)
            {
                // Two writes in a row — flush previous with 0 delay
                m_sequence.push_back({lastWasHigh, pendingDelay});
                pendingDelay = 0;
            }
            lastWasHigh = (m[1].str() == "HIGH");
            lastWasWrite = true;
        }
        else if (m[2].matched)
        {
            // delay
            pendingDelay += std::stoi(m[2].str());
            if (lastWasWrite)
            {
                m_sequence.push_back({lastWasHigh, pendingDelay});
                pendingDelay = 0;
                lastWasWrite = false;
            }
        }
    }
    if (lastWasWrite)
        m_sequence.push_back({lastWasHigh, pendingDelay});

    // Fallback: if no sequence parsed, default blink
    if (m_sequence.empty())
    {
        m_sequence.push_back({true,  500});
        m_sequence.push_back({false, 500});
    }

    // For backwards compat keep m_delayHigh/Low
    m_delayHigh = m_delayLow = 500;
    for (auto &s : m_sequence)
    {
        if (s.high) m_delayHigh = s.delayMs;
        else        m_delayLow  = s.delayMs;
    }
}

// ── Execute sketch with API context ────────────────────────────────────────
void Simulator::executeSketch(const ArduinoAPI &)
{
    // Simulate setup
    pinMode(m_ledPin, OUTPUT);

    // Simulate loop — replay the parsed digitalWrite/delay sequence
    while (m_running.load())
    {
        for (auto &step : m_sequence)
        {
            if (!m_running.load())
                break;
            digitalWrite(m_ledPin, step.high ? HIGH : LOW);
            if (step.delayMs > 0)
                delay(step.delayMs);
        }
    }
}
bool Simulator::validateLEDCircuit(int ledCompId,
                                   const std::vector<Component> &components,
                                   std::string &outDrivingPin) const
{
    const Component *led = nullptr;

    for (auto &c : components)
        if (c.id == ledCompId)
            led = &c;

    if (!led)
        return false;

    bool anodeOk = false;
    bool cathodeOk = false;

    for (auto &net : m_nets)
    {
        bool hasAnode = net.hasPin(ledCompId, "Anode");
        bool hasCathode = net.hasPin(ledCompId, "Cathode");

        if (hasAnode)
        {
            for (auto &ep : net.endpoints)
            {
                for (auto &c : components)
                {
                    if (c.id != ep.compId)
                        continue;

                    if (c.kind == ComponentKind::ArduinoNano ||
                        c.kind == ComponentKind::ArduinoUno)
                    {
                        for (auto &p : c.pins)
                        {
                            if (p.id == ep.pinId &&
                                (p.type == PinType::Digital || p.type == PinType::PWM))
                            {
                                outDrivingPin = p.id;
                                anodeOk = true;
                            }
                        }
                    }
                }
            }
        }

        if (hasCathode)
        {
            for (auto &ep : net.endpoints)
            {
                for (auto &c : components)
                {
                    if (c.id != ep.compId)
                        continue;

                    if (c.kind == ComponentKind::ArduinoNano ||
                        c.kind == ComponentKind::ArduinoUno)
                    {
                        for (auto &p : c.pins)
                        {
                            if (p.id == ep.pinId && p.type == PinType::Ground)
                                cathodeOk = true;
                        }
                    }
                }
            }
        }
    }

    return anodeOk && cathodeOk;
}

bool Simulator::ledIsOn(int compId) const
{
    for (auto &p : m_ledOn)
        if (p.first == compId)
            return p.second->load();

    return false;
}

void Simulator::tick(float) {}

// ── Thread ──────────────────────────────────────────────────────────────────
void Simulator::simThread()
{
    // Set up API context for this thread
    ArduinoAPI api;
    api.pinMode = [this](uint8_t p, uint8_t m)
    { this->api_pinMode(p, m); };
    api.digitalWrite = [this](uint8_t p, uint8_t v)
    { this->api_digitalWrite(p, v); };
    api.digitalRead = [this](uint8_t p)
    { return this->api_digitalRead(p); };
    api.analogWrite = [this](uint8_t p, uint8_t v)
    { this->api_analogWrite(p, v); };
    api.analogRead = [this](uint8_t p)
    { return this->api_analogRead(p); };
    api.delay = [this](uint32_t ms)
    { this->api_delay(ms); };

    g_api = &api;

    // Execute the sketch with our API
    executeSketch(api);

    g_api = nullptr;
}

// ── API implementation ──────────────────────────────────────────────────────
void Simulator::api_pinMode(uint8_t pin, uint8_t mode)
{
    if (pin < 64)
        m_pinModes[pin].store(mode);
}

void Simulator::api_digitalWrite(uint8_t pin, uint8_t val)
{
    if (pin >= 64)
        return;

    m_pinStates[pin].store(val);

    for (auto &b : m_ledBindings)
    {
        if (b.arduinoPin != pin)
            continue;

        bool on = (val == HIGH);

        for (auto &p : m_ledOn)
            if (p.first == b.ledCompId)
                p.second->store(on);
    }
}

void Simulator::api_delay(uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

uint8_t Simulator::api_digitalRead(uint8_t pin)
{
    if (pin < 64)
        return m_pinStates[pin].load();
    return LOW;
}

void Simulator::api_analogWrite(uint8_t pin, uint8_t val)
{
    if (pin < 64)
        m_analogStates[pin].store(val);
}

uint16_t Simulator::api_analogRead(uint8_t pin)
{
    if (pin < 64)
        return m_analogStates[pin].load();
    return 0;
}