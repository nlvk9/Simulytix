#pragma once
#include "Component.h"
#include "Wire.h"
#include <vector>
#include <string>

// ── Canvas ────────────────────────────────────────────────────────────────────

struct WireInProgress {
    bool  active   = false;
    int   compId   = -1;
    int   pinIdx   = -1;
    float mouseX   = 0;
    float mouseY   = 0;
};

class Canvas {
public:
    Canvas();

    // Coordinate transforms
    float worldToScreenX(float wx) const { return (wx - m_camX) * m_zoom; }
    float worldToScreenY(float wy) const { return (wy - m_camY) * m_zoom; }
    float screenToWorldX(float sx) const { return sx / m_zoom + m_camX; }
    float screenToWorldY(float sy) const { return sy / m_zoom + m_camY; }

    // Component management
    int  addComponent(ComponentKind kind, float wx, float wy);
    void deleteSelected();

    // Input handling
    void onMouseDown(int sx, int sy, int button);
    void onMouseUp(int sx, int sy, int button);
    void onMouseMove(int sx, int sy, int dx, int dy);
    void onScroll(int x, int y, float delta);
    void onKey(int sym);

    // Accessors for rendering / simulation
    std::vector<Component> &components() { return m_components; }
    std::vector<Wire>      &wires()      { return m_wires; }
    const std::vector<Component> &components() const { return m_components; }
    const std::vector<Wire>      &wires()      const { return m_wires; }

    WireInProgress wip() const { return m_wip; }

    float zoom()  const { return m_zoom; }
    float camX()  const { return m_camX; }
    float camY()  const { return m_camY; }

    // Canvas pixel region (set by main each frame)
    int canvasX = 0, canvasY = 0, canvasW = 800, canvasH = 600;

private:
    // Hit testing
    int  hitComponent(float wx, float wy) const;           // returns index or -1
    bool hitPin(float wx, float wy, int &compIdx, int &pinIdx, float radius = 10.f) const;

    std::vector<Component> m_components;
    std::vector<Wire>      m_wires;

    float m_zoom = 1.f;
    float m_camX = -200.f;
    float m_camY = -100.f;

    // Drag state
    bool  m_panning     = false;
    int   m_dragComp    = -1;   // index into m_components
    float m_dragOffX    = 0, m_dragOffY = 0;

    int   m_selected    = -1;   // selected component index

    WireInProgress m_wip;

    int   m_nextId = 1;
    int   m_nextWireId = 1;

    // Wire colors cycle
    int   m_wireColorIdx = 0;
    static constexpr uint8_t WIRE_COLORS[][3] = {
        {220, 50,  50},
        {50,  180, 50},
        {50,  120, 220},
        {220, 180, 50},
        {180, 50,  220},
        {50,  200, 200},
    };
};
