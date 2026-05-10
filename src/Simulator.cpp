#include "Simulator.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <regex>

// ── Global simulator pointer ────────────────────────────────────────────────
Simulator *g_sim = nullptr;

// ── Arduino-style constants ─────────────────────────────────────────────────
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t INPUT = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t LOW = 0;

// ── Arduino API (GLOBAL INLINE SHIMS — FIXED) ───────────────────────────────
inline void pinMode(uint8_t p, uint8_t m)
{
    if (g_sim)
        g_sim->api_pinMode(p, m);
}

inline void digitalWrite(uint8_t p, uint8_t v)
{
    if (g_sim)
        g_sim->api_digitalWrite(p, v);
}

inline void delay(uint32_t ms)
{
    if (g_sim)
        g_sim->api_delay(ms);
}

// ── User sketch ─────────────────────────────────────────────────────────────

// ────────────────────────────────────────────────────────────────────────────
// Simulator Implementation
// ────────────────────────────────────────────────────────────────────────────

Simulator::Simulator()
{
    for (auto &m : m_pinModes)
        m.store(0);
    for (auto &s : m_pinStates)
        s.store(0);
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

        int nanoId = -1;
        for (auto &c2 : components)
            if (c2.kind == ComponentKind::ArduinoNano)
                nanoId = c2.id;

        m_ledBindings.push_back({c.id, pinNum, nanoId});
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

    // Parse LED_PIN
    std::regex pinRegex(R"(const\s+uint8_t\s+LED_PIN\s*=\s*(\d+);)");
    std::smatch pinMatch;
    if (std::regex_search(code, pinMatch, pinRegex))
    {
        m_ledPin = std::stoi(pinMatch[1]);
    }

    // Parse delays in loop
    std::regex delayRegex(R"(delay\((\d+)\))");
    std::sregex_iterator iter(code.begin(), code.end(), delayRegex);
    std::sregex_iterator end;
    std::vector<int> delays;
    for (; iter != end; ++iter)
    {
        delays.push_back(std::stoi((*iter)[1]));
    }
    if (delays.size() >= 2)
    {
        m_delayHigh = delays[0];
        m_delayLow = delays[1];
    }
    else if (delays.size() == 1)
    {
        m_delayHigh = m_delayLow = delays[0];
    }
}

// ── Circuit validation (unchanged logic) ────────────────────────────────────
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

                    if (c.kind == ComponentKind::ArduinoNano)
                    {
                        for (auto &p : c.pins)
                        {
                            if (p.id == ep.pinId && p.type == PinType::Digital)
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

                    if (c.kind == ComponentKind::ArduinoNano)
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
    g_sim = this;

    // Simulate setup
    api_pinMode(m_ledPin, OUTPUT);

    // Simulate loop
    while (m_running.load())
    {
        api_digitalWrite(m_ledPin, HIGH);
        api_delay(m_delayHigh);
        api_digitalWrite(m_ledPin, LOW);
        api_delay(m_delayLow);
    }

    g_sim = nullptr;
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