#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <SDL2/SDL.h>

Canvas::Canvas() {}

int Canvas::addComponent(ComponentKind kind, float wx, float wy)
{
    int id = m_nextId++;
    switch (kind)
    {
    case ComponentKind::ArduinoNano:
        m_components.push_back(Component::makeArduinoNano(id, wx, wy));
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

// ── Mouse input ───────────────────────────────────────────────────────────────

void Canvas::onMouseDown(int sx, int sy, int button)
{
    // Only handle events inside canvas region
    if (sx < canvasX || sx > canvasX + canvasW)
        return;
    if (sy < canvasY || sy > canvasY + canvasH)
        return;

    float wx = screenToWorldX(sx - canvasX);
    float wy = screenToWorldY(sy - canvasY);

    if (button == SDL_BUTTON_MIDDLE)
    {
        m_panning = true;
        return;
    }

    if (button == SDL_BUTTON_LEFT)
    {
        // Check pin hit first (start wire)
        int ci = -1, pi = -1;
        if (hitPin(wx, wy, ci, pi))
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

    if (button == SDL_BUTTON_RIGHT)
    {
        // Cancel wire in progress
        m_wip.active = false;
    }
}

void Canvas::onMouseUp(int sx, int sy, int button)
{
    float wx = screenToWorldX(sx - canvasX);
    float wy = screenToWorldY(sy - canvasY);

    if (button == SDL_BUTTON_MIDDLE)
    {
        m_panning = false;
        return;
    }

    if (button == SDL_BUTTON_LEFT)
    {
        m_dragComp = -1;

        if (m_wip.active)
        {
            // Try to complete wire
            int ci = -1, pi = -1;
            if (hitPin(wx, wy, ci, pi))
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
        m_components[m_dragComp].x = wx - m_dragOffX;
        m_components[m_dragComp].y = wy - m_dragOffY;
        return;
    }

    if (m_wip.active)
    {
        m_wip.mouseX = wx;
        m_wip.mouseY = wy;
    }
}

void Canvas::onScroll(int /*x*/, int /*y*/, float delta)
{
    float factor = (delta > 0) ? 1.1f : 0.9f;
    m_zoom = std::max(0.2f, std::min(3.f, m_zoom * factor));
}

void Canvas::onKey(int sym)
{
    if (sym == SDLK_DELETE || sym == SDLK_BACKSPACE)
        deleteSelected();
}
