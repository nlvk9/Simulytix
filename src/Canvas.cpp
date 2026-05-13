#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <Qt>

Canvas::Canvas() {}

int Canvas::addComponent(ComponentKind kind, float wx, float wy)
{
    if (m_gridSnap)
    {
        float grid = gridSize();
        wx = std::round(wx / grid) * grid;
        wy = std::round(wy / grid) * grid;
    }

    int id = m_nextId++;
    switch (kind)
    {
    case ComponentKind::ArduinoNano:
        m_components.push_back(Component::makeArduinoNano(id, wx, wy));
        break;
    case ComponentKind::ArduinoUno:
        m_components.push_back(Component::makeArduinoUno(id, wx, wy));
        break;
    case ComponentKind::LED:
        m_components.push_back(Component::makeLED(id, wx, wy));
        break;
    case ComponentKind::Resistor:
        m_components.push_back(Component::makeResistor(id, wx, wy));
        break;
    case ComponentKind::PushButton:
        m_components.push_back(Component::makePushButton(id, wx, wy));
        break;
    case ComponentKind::Potentiometer:
        m_components.push_back(Component::makePotentiometer(id, wx, wy));
        break;
    case ComponentKind::Servo:
        m_components.push_back(Component::makeServo(id, wx, wy));
        break;
    case ComponentKind::UARTTerminal:
        m_components.push_back(Component::makeUARTTerminal(id, wx, wy));
        break;
    case ComponentKind::I2CDevice:
        m_components.push_back(Component::makeI2CDevice(id, wx, wy));
        break;
    }
    return id;
}

void Canvas::deleteSelected()
{
    if (m_selected < 0 || m_selected >= (int)m_components.size())
        return;
    int id = m_components[m_selected].id;

    // Remove wires referencing this component
    m_wires.erase(std::remove_if(m_wires.begin(), m_wires.end(),
                                 [id](const Wire &w)
                                 { return w.compA == id || w.compB == id; }),
                  m_wires.end());

    m_components.erase(m_components.begin() + m_selected);
    m_selected = -1;
}

// ── Hit testing ───────────────────────────────────────────────────────────────

int Canvas::hitComponent(float wx, float wy) const
{
    for (int i = (int)m_components.size() - 1; i >= 0; --i)
    {
        auto &c = m_components[i];
        if (wx >= c.x && wx <= c.x + c.w &&
            wy >= c.y && wy <= c.y + c.h)
            return i;
    }
    return -1;
}

bool Canvas::hitPin(float wx, float wy, int &compIdx, int &pinIdx, float radius) const
{
    for (int i = 0; i < (int)m_components.size(); ++i)
    {
        auto &c = m_components[i];
        for (int j = 0; j < (int)c.pins.size(); ++j)
        {
            float px = c.x + c.pins[j].localX;
            float py = c.y + c.pins[j].localY;
            float dx = wx - px, dy = wy - py;
            if (std::sqrt(dx * dx + dy * dy) <= radius)
            {
                compIdx = i;
                pinIdx = j;
                return true;
            }
        }
    }
    return false;
}

int Canvas::componentIndexById(int compId) const
{
    for (int i = 0; i < (int)m_components.size(); ++i)
        if (m_components[i].id == compId)
            return i;
    return -1;
}

// ── Mouse input ───────────────────────────────────────────────────────────────

void Canvas::onMouseDown(int sx, int sy, int button)
{
    // Only handle events inside canvas region (to the right of sidebar)
    if (sx < canvasX || sx > canvasX + canvasW)
        return;
    if (sy < canvasY || sy > canvasY + canvasH)
        return;

    float wx = screenToWorldX(sx - canvasX);
    float wy = screenToWorldY(sy - canvasY);

    if (button == Qt::MiddleButton)
    {
        m_panning = true;
        return;
    }

    if (button == Qt::LeftButton)
    {
        // Check pin hit first (start wire)
        // Use a generous hit radius scaled by zoom so small pins are still clickable
        float hitRadius = std::max(8.f, 10.f / m_zoom);
        int ci = -1, pi = -1;
        if (hitPin(wx, wy, ci, pi, hitRadius))
        {
            m_wip.active = true;
            m_wip.compId = m_components[ci].id;
            m_wip.pinIdx = pi;
            m_wip.mouseX = wx;
            m_wip.mouseY = wy;
            return;
        }

        // Check component hit (drag / select)
        int idx = hitComponent(wx, wy);
        if (idx >= 0)
        {
            m_selected = idx;
            m_dragComp = idx;
            m_dragOffX = wx - m_components[idx].x;
            m_dragOffY = wy - m_components[idx].y;

            // If PushButton, toggle pressed state
            if (m_components[idx].kind == ComponentKind::PushButton)
            {
                m_components[idx].buttonPressed = !m_components[idx].buttonPressed;
            }
        }
        else
        {
            m_selected = -1;
        }
    }

    if (button == Qt::RightButton)
    {
        // Cancel wire in progress
        m_wip.active = false;
    }
}

void Canvas::onMouseUp(int sx, int sy, int button)
{
    float wx = screenToWorldX(sx - canvasX);
    float wy = screenToWorldY(sy - canvasY);

    if (button == Qt::MiddleButton)
    {
        m_panning = false;
        return;
    }

    if (button == Qt::LeftButton)
    {
        m_dragComp = -1;

        if (m_wip.active)
        {
            // Try to complete wire — use same generous radius as press
            float hitRadius = std::max(8.f, 10.f / m_zoom);
            int ci = -1, pi = -1;
            if (hitPin(wx, wy, ci, pi, hitRadius))
            {
                int targetCompId = m_components[ci].id;
                // Don't wire a pin to itself
                if (targetCompId != m_wip.compId || pi != m_wip.pinIdx)
                {
                    // Find source component index
                    int srcIdx = -1;
                    for (int i = 0; i < (int)m_components.size(); ++i)
                        if (m_components[i].id == m_wip.compId)
                        {
                            srcIdx = i;
                            break;
                        }

                    if (srcIdx >= 0)
                    {
                        Wire w;
                        w.id = m_nextWireId++;
                        w.compA = m_wip.compId;
                        w.pinA = m_wip.pinIdx;
                        w.compB = targetCompId;
                        w.pinB = pi;
                        w.complete = true;
                        auto &col = WIRE_COLORS[m_wireColorIdx % 6];
                        w.r = col[0];
                        w.g = col[1];
                        w.b = col[2];
                        m_wireColorIdx++;
                        m_wires.push_back(w);
                    }
                }
            }
            m_wip.active = false;
        }
    }
}

void Canvas::onMouseMove(int sx, int sy, int dx, int dy)
{
    float wx = screenToWorldX(sx - canvasX);
    float wy = screenToWorldY(sy - canvasY);

    if (m_panning)
    {
        m_camX -= dx / m_zoom;
        m_camY -= dy / m_zoom;
        return;
    }

    if (m_dragComp >= 0)
    {
        float newX = wx - m_dragOffX;
        float newY = wy - m_dragOffY;

        if (m_gridSnap)
        {
            float grid = gridSize();
            newX = std::round(newX / grid) * grid;
            newY = std::round(newY / grid) * grid;
        }

        m_components[m_dragComp].x = newX;
        m_components[m_dragComp].y = newY;
        return;
    }

    if (m_wip.active)
    {
        m_wip.mouseX = wx;
        m_wip.mouseY = wy;
    }
}

void Canvas::onScroll(int x, int y, float delta)
{
    // Convert screen coordinates to world coordinates before zoom
    float worldX = screenToWorldX(x - canvasX);
    float worldY = screenToWorldY(y - canvasY);

    float factor = (delta > 0) ? 1.1f : 0.9f;
    float oldZoom = m_zoom;
    m_zoom = std::max(0.2f, std::min(3.f, m_zoom * factor));

    // Adjust camera position to keep the cursor point fixed
    float zoomRatio = m_zoom / oldZoom;
    m_camX = worldX - (worldX - m_camX) / zoomRatio;
    m_camY = worldY - (worldY - m_camY) / zoomRatio;
}

void Canvas::onKey(int sym)
{
    if (sym == Qt::Key_Delete || sym == Qt::Key_Backspace)
        deleteSelected();
}
