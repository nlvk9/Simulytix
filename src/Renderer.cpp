#include "Renderer.h"
#include <cmath>
#include <cstring>
#include <algorithm>

Renderer::Renderer() {}
Renderer::~Renderer()
{
    if (m_font)
        TTF_CloseFont(m_font);
    if (m_fontSm)
        TTF_CloseFont(m_fontSm);
}

bool Renderer::init(SDL_Renderer * /*r*/)
{
    if (TTF_Init() != 0)
        return false;
    const char *candidates[] = {
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/System/Library/Fonts/Monaco.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        nullptr};
    for (int i = 0; candidates[i]; ++i)
    {
        m_font = TTF_OpenFont(candidates[i], 11);
        if (m_font)
            break;
    }
    for (int i = 0; candidates[i]; ++i)
    {
        m_fontSm = TTF_OpenFont(candidates[i], 9);
        if (m_fontSm)
            break;
    }
    return true;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void Renderer::fillCircle(SDL_Renderer *r, int cx, int cy, int rad)
{
    // Improved circle filling using better algorithm
    for (int y = -rad; y <= rad; ++y)
    {
        int x = (int)std::sqrt((float)(rad * rad - y * y));
        SDL_RenderDrawLine(r, cx - x, cy + y, cx + x, cy + y);
    }
}

void Renderer::drawCircleOutline(SDL_Renderer *r, int cx, int cy, int rad, int thickness)
{
    // Draw multiple circles for thickness
    for (int t = 0; t < thickness; ++t)
    {
        int r_inner = rad - t;
        if (r_inner < 0)
            break;

        int x = r_inner, y = 0, err = 0;
        while (x >= y)
        {
            SDL_RenderDrawPoint(r, cx + x, cy + y);
            SDL_RenderDrawPoint(r, cx + y, cy + x);
            SDL_RenderDrawPoint(r, cx - y, cy + x);
            SDL_RenderDrawPoint(r, cx - x, cy + y);
            SDL_RenderDrawPoint(r, cx - x, cy - y);
            SDL_RenderDrawPoint(r, cx - y, cy - x);
            SDL_RenderDrawPoint(r, cx + y, cy - x);
            SDL_RenderDrawPoint(r, cx + x, cy - y);
            y++;
            if (err <= 0)
                err += 2 * y + 1;
            else
            {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }
}

// Draw rounded rectangle
void Renderer::fillRoundedRect(SDL_Renderer *r, int x, int y, int w, int h, int radius)
{
    // Draw main rectangle
    SDL_Rect rect = {x + radius, y, w - 2 * radius, h};
    SDL_RenderFillRect(r, &rect);
    rect = {x, y + radius, w, h - 2 * radius};
    SDL_RenderFillRect(r, &rect);

    // Draw corners
    fillCircle(r, x + radius, y + radius, radius);
    fillCircle(r, x + w - radius, y + radius, radius);
    fillCircle(r, x + radius, y + h - radius, radius);
    fillCircle(r, x + w - radius, y + h - radius, radius);
}

void Renderer::renderText(SDL_Renderer *r, const std::string &txt,
                          int x, int y, uint8_t R, uint8_t G, uint8_t B)
{
    if (!m_font || txt.empty())
        return;
    SDL_Color col = {R, G, B, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(m_font, txt.c_str(), col);
    if (!surf)
        return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

// ── Main draw ─────────────────────────────────────────────────────────────────

void Renderer::drawAll(SDL_Renderer *r, Canvas &canvas,
                       const Simulator &sim, int winW, int winH,
                       float glowPhase)
{
    int sw = sidebarWidth();
    int ew = editorWidth();
    int cw = winW - sw - ew;
    int ch = winH;
    int editorEdgeX = winW - ew;

    canvas.canvasX = sw;
    canvas.canvasY = 0;
    canvas.canvasW = cw;
    canvas.canvasH = ch;

    // Background with gradient
    SDL_SetRenderDrawColor(r, 18, 20, 28, 255); // Dark blue-gray
    SDL_RenderClear(r);

    // Editor resize handle
    SDL_SetRenderDrawColor(r, m_resizingEditor ? 180 : 80,
                           m_resizingEditor ? 180 : 80,
                           m_resizingEditor ? 220 : 100,
                           255);
    SDL_RenderDrawLine(r, editorEdgeX - 1, 0, editorEdgeX - 1, winH);

    // Canvas area clip
    SDL_Rect canvasClip = {sw, 0, cw, ch};
    SDL_RenderSetClipRect(r, &canvasClip);

    drawGrid(r, canvas, cw, ch);
    drawWires(r, canvas);
    drawWireInProgress(r, canvas);

    // Draw components
    auto &comps = canvas.components();
    for (int i = 0; i < (int)comps.size(); ++i)
    {
        bool ledOn = (comps[i].kind == ComponentKind::LED) && sim.ledIsOn(comps[i].id);
        drawComponent(r, canvas, comps[i], false, ledOn, glowPhase);
    }

    SDL_RenderSetClipRect(r, nullptr);

    drawSidebar(r, winH);
}

// ── Grid ──────────────────────────────────────────────────────────────────────

void Renderer::drawGrid(SDL_Renderer *r, const Canvas &c, int cw, int ch)
{
    float zoom = c.zoom();
    float gridSpacing = 20.f * zoom;
    if (gridSpacing < 8)
        gridSpacing *= 2;
    if (gridSpacing < 8)
        gridSpacing *= 2.5f;

    float offX = std::fmod(-c.camX() * zoom, gridSpacing);
    float offY = std::fmod(-c.camY() * zoom, gridSpacing);

    // Subtle grid lines
    SDL_SetRenderDrawColor(r, 40, 45, 55, 255);
    int ox = c.canvasX;

    for (float x = offX; x < cw; x += gridSpacing)
        SDL_RenderDrawLine(r, ox + (int)x, 0, ox + (int)x, ch);
    for (float y = offY; y < ch; y += gridSpacing)
        SDL_RenderDrawLine(r, ox, (int)y, ox + cw, (int)y);
}

// ── Wires ─────────────────────────────────────────────────────────────────────

void Renderer::drawWires(SDL_Renderer *r, const Canvas &canvas)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    int ox = canvas.canvasX;

    for (auto &w : canvas.wires())
    {
        if (!w.complete)
            continue;
        const Component *cA = nullptr, *cB = nullptr;
        for (auto &c : canvas.components())
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

        float ax = canvas.worldToScreenX(cA->x + cA->pins[w.pinA].localX) + ox;
        float ay = canvas.worldToScreenY(cA->y + cA->pins[w.pinA].localY);
        float bx = canvas.worldToScreenX(cB->x + cB->pins[w.pinB].localX) + ox;
        float by = canvas.worldToScreenY(cB->y + cB->pins[w.pinB].localY);

        // Draw wire with a 90-degree elbow
        float midY = (ay + by) * 0.5f;
        SDL_SetRenderDrawColor(r, w.r, w.g, w.b, 255);
        SDL_RenderDrawLine(r, (int)ax, (int)ay, (int)ax, (int)midY);
        SDL_RenderDrawLine(r, (int)ax, (int)midY, (int)bx, (int)midY);
        SDL_RenderDrawLine(r, (int)bx, (int)midY, (int)bx, (int)by);

        // Endpoint dots
        fillCircle(r, (int)ax, (int)ay, 3);
        fillCircle(r, (int)bx, (int)by, 3);
    }
}

void Renderer::drawWireInProgress(SDL_Renderer *r, const Canvas &canvas)
{
    auto wip = canvas.wip();
    if (!wip.active)
        return;

    const Component *src = nullptr;
    for (auto &c : canvas.components())
        if (c.id == wip.compId)
        {
            src = &c;
            break;
        }
    if (!src || wip.pinIdx >= (int)src->pins.size())
        return;

    int ox = canvas.canvasX;
    float ax = canvas.worldToScreenX(src->x + src->pins[wip.pinIdx].localX) + ox;
    float ay = canvas.worldToScreenY(src->y + src->pins[wip.pinIdx].localY);
    float bx = canvas.worldToScreenX(wip.mouseX) + ox;
    float by = canvas.worldToScreenY(wip.mouseY);

    SDL_SetRenderDrawColor(r, 255, 255, 80, 200);
    float midY = (ay + by) * 0.5f;
    SDL_RenderDrawLine(r, (int)ax, (int)ay, (int)ax, (int)midY);
    SDL_RenderDrawLine(r, (int)ax, (int)midY, (int)bx, (int)midY);
    SDL_RenderDrawLine(r, (int)bx, (int)midY, (int)bx, (int)by);
}

// ── Components ────────────────────────────────────────────────────────────────

void Renderer::drawComponent(SDL_Renderer *r, const Canvas &canvas,
                             const Component &comp, bool /*selected*/,
                             bool ledOn, float glowPhase)
{
    float zoom = canvas.zoom();
    int ox = canvas.canvasX;

    float sx = canvas.worldToScreenX(comp.x) + ox;
    float sy = canvas.worldToScreenY(comp.y);
    float sw = comp.w * zoom;
    float sh = comp.h * zoom;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // ── Arduino Nano ──────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::ArduinoNano)
    {
        // PCB body with rounded corners
        SDL_SetRenderDrawColor(r, 30, 70, 30, 255);
        fillRoundedRect(r, (int)sx, (int)sy, (int)sw, (int)sh, 4);

        // PCB border
        SDL_SetRenderDrawColor(r, 60, 140, 60, 255);
        drawCircleOutline(r, (int)sx + 4, (int)sy + 4, 4, 2);
        drawCircleOutline(r, (int)sx + (int)sw - 4, (int)sy + 4, 4, 2);
        drawCircleOutline(r, (int)sx + 4, (int)sy + (int)sh - 4, 4, 2);
        drawCircleOutline(r, (int)sx + (int)sw - 4, (int)sy + (int)sh - 4, 4, 2);

        // Side borders
        SDL_RenderDrawLine(r, (int)sx + 4, (int)sy, (int)sx + (int)sw - 4, (int)sy);
        SDL_RenderDrawLine(r, (int)sx + 4, (int)sy + (int)sh, (int)sx + (int)sw - 4, (int)sy + (int)sh);
        SDL_RenderDrawLine(r, (int)sx, (int)sy + 4, (int)sx, (int)sy + (int)sh - 4);
        SDL_RenderDrawLine(r, (int)sx + (int)sw, (int)sy + 4, (int)sx + (int)sw, (int)sy + (int)sh - 4);

        // Label with better typography
        renderText(r, "Arduino Nano", (int)sx + (int)(sw / 2) - 32, (int)sy + (int)(sh / 2) - 8,
                   140, 255, 140);

        // Pins with better rendering
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            // Pin colors based on type
            uint8_t pr = 200, pg = 200, pb = 200;
            if (p.type == PinType::Ground)
            {
                pr = 80;
                pg = 80;
                pb = 80;
            }
            else if (p.type == PinType::Power)
            {
                pr = 255;
                pg = 100;
                pb = 100;
            }
            else if (p.type == PinType::Digital)
            {
                pr = 100;
                pg = 150;
                pb = 255;
            }

            SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
            fillCircle(r, (int)px, (int)py, 4);

            // Pin label with better positioning
            if (zoom > 0.7f && m_fontSm)
            {
                bool leftSide = (p.localX == 0.f);
                int lx = leftSide ? (int)px - 35 : (int)px + 8;
                int ly = (int)py - 6;
                SDL_Color col = {160, 220, 160, 255};
                SDL_Surface *surf = TTF_RenderText_Blended(m_fontSm, p.label.c_str(), col);
                if (surf)
                {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
                    SDL_Rect dst = {lx, ly, surf->w, surf->h};
                    SDL_RenderCopy(r, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
            }
        }
        return;
    }

    // ── Arduino UNO ───────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::ArduinoUno)
    {
        // PCB body with rounded corners
        SDL_SetRenderDrawColor(r, 30, 70, 30, 255);
        fillRoundedRect(r, (int)sx, (int)sy, (int)sw, (int)sh, 4);

        // PCB border
        SDL_SetRenderDrawColor(r, 60, 140, 60, 255);
        drawCircleOutline(r, (int)sx + 4, (int)sy + 4, 4, 2);
        drawCircleOutline(r, (int)sx + (int)sw - 4, (int)sy + 4, 4, 2);
        drawCircleOutline(r, (int)sx + 4, (int)sy + (int)sh - 4, 4, 2);
        drawCircleOutline(r, (int)sx + (int)sw - 4, (int)sy + (int)sh - 4, 4, 2);

        // Side borders
        SDL_RenderDrawLine(r, (int)sx + 4, (int)sy, (int)sx + (int)sw - 4, (int)sy);
        SDL_RenderDrawLine(r, (int)sx + 4, (int)sy + (int)sh, (int)sx + (int)sw - 4, (int)sy + (int)sh);
        SDL_RenderDrawLine(r, (int)sx, (int)sy + 4, (int)sx, (int)sy + (int)sh - 4);
        SDL_RenderDrawLine(r, (int)sx + (int)sw, (int)sy + 4, (int)sx + (int)sw, (int)sy + (int)sh - 4);

        // Label
        renderText(r, "Arduino UNO", (int)sx + (int)(sw / 2) - 40, (int)sy + (int)(sh / 2) - 8,
                   140, 255, 140);

        // Pins with better rendering
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            // Pin colors based on type
            uint8_t pr = 200, pg = 200, pb = 200;
            if (p.type == PinType::Ground)
            {
                pr = 80;
                pg = 80;
                pb = 80;
            }
            else if (p.type == PinType::Power)
            {
                pr = 255;
                pg = 100;
                pb = 100;
            }
            else if (p.type == PinType::Digital || p.type == PinType::PWM)
            {
                pr = 100;
                pg = 150;
                pb = 255;
            }
            else if (p.type == PinType::Analog)
            {
                pr = 255;
                pg = 200;
                pb = 100;
            }
            else if (p.type == PinType::I2C_SDA || p.type == PinType::I2C_SCL)
            {
                pr = 200;
                pg = 100;
                pb = 255;
            }

            SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
            fillCircle(r, (int)px, (int)py, 4);

            // Pin label
            if (zoom > 0.7f && m_fontSm)
            {
                bool leftSide = (p.localX == 0.f);
                bool bottomSide = (p.localY == comp.h);
                int lx = leftSide ? (int)px - 35 : (int)px + 8;
                int ly = bottomSide ? (int)py + 8 : (int)py - 6;
                SDL_Color col = {160, 220, 160, 255};
                SDL_Surface *surf = TTF_RenderText_Blended(m_fontSm, p.label.c_str(), col);
                if (surf)
                {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
                    SDL_Rect dst = {lx, ly, surf->w, surf->h};
                    SDL_RenderCopy(r, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
            }
        }
        return;
    }

    // ── LED ───────────────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::LED)
    {
        int cx = (int)(sx + sw / 2);
        int cy = (int)(sy + sh / 2);
        int rad = (int)(std::min(sw, sh) * 0.35f);
        int glowRad = rad + (int)(25 * zoom);

        if (ledOn)
        {
            // Multi-layer glow effect for realism
            float pulse = 0.8f + 0.2f * std::sin(glowPhase * 2.0f);

            // Outer glow
            for (int gr = glowRad; gr >= rad + 8; gr -= 1)
            {
                float t = (float)(gr - rad - 8) / (float)(glowRad - rad - 8);
                uint8_t alpha = (uint8_t)((1.f - t * t) * pulse * 120.f);
                SDL_SetRenderDrawColor(r, 255, 140, 60, alpha);
                fillCircle(r, cx, cy, gr);
            }

            // Inner glow
            for (int gr = rad + 8; gr >= rad + 2; gr -= 1)
            {
                float t = (float)(gr - rad - 2) / 6.0f;
                uint8_t alpha = (uint8_t)((1.f - t) * pulse * 200.f);
                SDL_SetRenderDrawColor(r, 255, 180, 80, alpha);
                fillCircle(r, cx, cy, gr);
            }

            // Main LED body
            SDL_SetRenderDrawColor(r, 255, 220, 100, 255);
            fillCircle(r, cx, cy, rad);

            // Highlight
            SDL_SetRenderDrawColor(r, 255, 255, 200, 220);
            fillCircle(r, cx - (int)(rad * 0.25f), cy - (int)(rad * 0.25f), rad / 3);
        }
        else
        {
            // Off state - dim appearance
            SDL_SetRenderDrawColor(r, 40, 20, 10, 255);
            fillCircle(r, cx, cy, rad);

            // Subtle inner shadow
            SDL_SetRenderDrawColor(r, 20, 10, 5, 180);
            fillCircle(r, cx + (int)(rad * 0.2f), cy + (int)(rad * 0.2f), rad / 2);
        }

        // Lens outline
        SDL_SetRenderDrawColor(r, 80, 40, 20, 255);
        drawCircleOutline(r, cx, cy, rad + 1, 2);

        // Leads with better rendering
        float apy = canvas.worldToScreenY(comp.y + comp.pins[0].localY);
        float cpy = canvas.worldToScreenY(comp.y + comp.pins[1].localY);

        SDL_SetRenderDrawColor(r, 120, 120, 120, 255);
        SDL_RenderDrawLine(r, cx, (int)apy, cx, cy - rad);
        SDL_RenderDrawLine(r, cx, cy + rad, cx, (int)cpy);

        // Pin terminals
        SDL_SetRenderDrawColor(r, 220, 220, 100, 255); // Anode (yellow)
        fillCircle(r, cx, (int)apy, 5);
        SDL_SetRenderDrawColor(r, 100, 100, 220, 255); // Cathode (blue)
        fillCircle(r, cx, (int)cpy, 5);

        // Labels
        if (zoom > 0.6f)
        {
            renderText(r, "+", cx - 8, (int)apy - 16, 240, 240, 120);
            renderText(r, "-", cx - 6, (int)cpy + 2, 120, 120, 240);
        }
        return;
    }

    // ── Potentiometer ─────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::Potentiometer)
    {
        int cx = (int)(sx + sw / 2);
        int cy = (int)(sy + sh / 2);

        // Body
        SDL_SetRenderDrawColor(r, 80, 80, 90, 255);
        fillRoundedRect(r, (int)sx, (int)sy, (int)sw, (int)sh, 5);

        // Knob
        SDL_SetRenderDrawColor(r, 120, 120, 130, 255);
        fillCircle(r, cx, cy - 5, 15);

        // Indicator line
        float angle = comp.analogValue * 3.14159f * 1.5f - 3.14159f * 0.75f; // -135 to +135 degrees
        int endX = cx + (int)(cos(angle) * 12);
        int endY = cy - 5 + (int)(sin(angle) * 12);
        SDL_SetRenderDrawColor(r, 200, 200, 210, 255);
        SDL_RenderDrawLine(r, cx, cy - 5, endX, endY);

        // Label
        renderText(r, "POT", cx - 12, cy + 10, 220, 220, 230);

        // Pins
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            SDL_SetRenderDrawColor(r, 180, 180, 190, 255);
            fillCircle(r, (int)px, (int)py, 4);
        }
        return;
    }

    // ── Servo ─────────────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::Servo)
    {
        int cx = (int)(sx + sw / 2);
        int cy = (int)(sy + sh / 2);

        // Body
        SDL_SetRenderDrawColor(r, 100, 100, 110, 255);
        fillRoundedRect(r, (int)sx, (int)sy, (int)sw, (int)sh, 3);

        // Horn (arm)
        float angle = (comp.servoAngle - 90) * 3.14159f / 180.f;
        int hornLength = 20;
        int hornX = cx + (int)(cos(angle) * hornLength);
        int hornY = cy + (int)(sin(angle) * hornLength);

        SDL_SetRenderDrawColor(r, 150, 150, 160, 255);
        SDL_RenderDrawLine(r, cx, cy, hornX, hornY);

        // Horn circle
        fillCircle(r, hornX, hornY, 3);

        // Label
        renderText(r, "SERVO", cx - 18, cy - 8, 220, 220, 230);

        // Pins
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            uint8_t pr = 180, pg = 180, pb = 190;
            if (p.type == PinType::Power)
            {
                pr = 255;
                pg = 100;
                pb = 100;
            }
            else if (p.type == PinType::Ground)
            {
                pr = 80;
                pg = 80;
                pb = 80;
            }

            SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
            fillCircle(r, (int)px, (int)py, 4);

            // Servo pin labels
            if (m_fontSm)
            {
                SDL_Color textColor = {200, 220, 240, 255};
                SDL_Surface *surf = TTF_RenderText_Blended(m_fontSm, p.label.c_str(), textColor);
                if (surf)
                {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
                    int lx = (int)px - surf->w / 2;
                    int ly = (int)py + 8;
                    SDL_Rect dst = {lx, ly, surf->w, surf->h};
                    SDL_RenderCopy(r, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
            }
        }
        return;
    }

    // ── UART Terminal ─────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::UARTTerminal)
    {
        // Terminal window
        SDL_SetRenderDrawColor(r, 40, 40, 50, 255);
        fillRoundedRect(r, (int)sx, (int)sy, (int)sw, (int)sh, 5);

        SDL_SetRenderDrawColor(r, 80, 80, 100, 255);
        drawCircleOutline(r, (int)sx + 5, (int)sy + 5, 5, 2);
        drawCircleOutline(r, (int)sx + (int)sw - 5, (int)sy + 5, 5, 2);
        drawCircleOutline(r, (int)sx + 5, (int)sy + (int)sh - 5, 5, 2);
        drawCircleOutline(r, (int)sx + (int)sw - 5, (int)sy + (int)sh - 5, 5, 2);

        // Terminal content area
        SDL_SetRenderDrawColor(r, 20, 20, 30, 255);
        SDL_Rect content = {(int)sx + 8, (int)sy + 8, (int)sw - 16, (int)sh - 16};
        SDL_RenderFillRect(r, &content);

        // Label
        renderText(r, "UART", (int)sx + 10, (int)sy + 10, 100, 200, 255);

        // Pins
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            uint8_t pr = 100, pg = 200, pb = 255;
            SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
            fillCircle(r, (int)px, (int)py, 4);
        }
        return;
    }

    // ── I2C Device ────────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::I2CDevice)
    {
        // Device body
        SDL_SetRenderDrawColor(r, 60, 60, 80, 255);
        fillRoundedRect(r, (int)sx, (int)sy, (int)sw, (int)sh, 4);

        // Label
        renderText(r, "I2C", (int)sx + (int)(sw / 2) - 12, (int)sy + (int)(sh / 2) - 8,
                   150, 150, 255);

        // Pins
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            uint8_t pr = 150, pg = 150, pb = 255;
            if (p.type == PinType::Power)
            {
                pr = 255;
                pg = 100;
                pb = 100;
            }
            else if (p.type == PinType::Ground)
            {
                pr = 80;
                pg = 80;
                pb = 80;
            }

            SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
            fillCircle(r, (int)px, (int)py, 4);
        }
        return;
    }

    // ── PushButton ────────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::PushButton)
    {
        int cx = (int)(sx + sw / 2);
        int cy = (int)(sy + sh / 2);
        int w = (int)sw;
        int h = (int)sh;

        // Button base
        SDL_SetRenderDrawColor(r, 60, 60, 70, 255);
        fillRoundedRect(r, cx - w / 2, cy - h / 2, w, h, 3);

        // Button top surface with press effect
        int pressOffset = comp.buttonPressed ? 3 : 0;
        SDL_SetRenderDrawColor(r, comp.buttonPressed ? 120 : 160,
                               comp.buttonPressed ? 120 : 160,
                               comp.buttonPressed ? 130 : 170,
                               255);
        fillRoundedRect(r, cx - w / 2 + 3, cy - h / 2 + 3 + pressOffset, w - 6, h / 2 - 3, 2);

        // Button label
        renderText(r, "PUSH", cx - 16, cy - 8 + pressOffset, 40, 40, 50);

        // Pins with better rendering
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            // Lead line
            SDL_SetRenderDrawColor(r, 120, 120, 120, 255);
            int leadDir = (p.id == "BTN_A") ? 1 : -1;
            SDL_RenderDrawLine(r, (int)px, (int)py, (int)px + leadDir * 15, (int)py);

            // Pin terminal
            SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
            fillCircle(r, (int)px + leadDir * 15, (int)py, 4);
        }
        return;
    }

    // ── Resistor ──────────────────────────────────────────────────────────────
    if (comp.kind == ComponentKind::Resistor)
    {
        int cx = (int)(sx + sw / 2);
        float apy = canvas.worldToScreenY(comp.y + comp.pins[0].localY);
        float bpy = canvas.worldToScreenY(comp.y + comp.pins[1].localY);

        // Leads
        SDL_SetRenderDrawColor(r, 160, 160, 160, 255);
        SDL_RenderDrawLine(r, cx, (int)apy, cx, (int)sy + (int)(sh * 0.25f));
        SDL_RenderDrawLine(r, cx, (int)sy + (int)(sh * 0.75f), cx, (int)bpy);

        // Body
        int bw = (int)(sw * 0.9f);
        int bh = (int)(sh * 0.5f);
        int bx = cx - bw / 2;
        int by2 = (int)(sy + sh * 0.25f);
        SDL_SetRenderDrawColor(r, 180, 140, 60, 255);
        SDL_Rect body = {bx, by2, bw, bh};
        SDL_RenderFillRect(r, &body);
        SDL_SetRenderDrawColor(r, 220, 180, 80, 255);
        SDL_RenderDrawRect(r, &body);

        // Color bands
        int bw4 = bw / 5;
        SDL_SetRenderDrawColor(r, 220, 0, 0, 255);
        SDL_Rect b1 = {bx + bw4, by2, 3, bh};
        SDL_RenderFillRect(r, &b1);
        SDL_SetRenderDrawColor(r, 220, 0, 0, 255);
        SDL_Rect b2 = {bx + bw4 * 2, by2, 3, bh};
        SDL_RenderFillRect(r, &b2);
        SDL_SetRenderDrawColor(r, 100, 80, 30, 255);
        SDL_Rect b3 = {bx + bw4 * 3, by2, 3, bh};
        SDL_RenderFillRect(r, &b3);

        // Pin dots
        SDL_SetRenderDrawColor(r, 200, 200, 100, 255);
        SDL_Rect ad = {cx - 4, (int)apy - 4, 8, 8};
        SDL_RenderFillRect(r, &ad);
        SDL_Rect bd = {cx - 4, (int)bpy - 4, 8, 8};
        SDL_RenderFillRect(r, &bd);

        if (zoom > 0.5f)
            renderText(r, "220\xCE\xA9", bx + bw + 2, by2 + bh / 2 - 6, 200, 180, 100);
        return;
    }
}

// ── Sidebar ───────────────────────────────────────────────────────────────────

void Renderer::drawSidebar(SDL_Renderer *r, int winH)
{
    int sw = sidebarWidth();

    // Background
    SDL_SetRenderDrawColor(r, 22, 22, 34, 255);
    SDL_Rect bg = {0, 0, sw, winH};
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 50, 50, 80, 255);
    SDL_RenderDrawLine(r, sw - 1, 0, sw - 1, winH);

    renderText(r, "Components", 8, 10, 140, 140, 180);

    m_palette.clear();
    const char *labels[] = {"Arduino\n Nano", "Arduino\n UNO", "LED", "Resistor", "Push\nButton",
                            "Potent-\niometer", "Servo", "UART\nTerminal", "I2C\nDevice"};
    const int kinds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    const int colors[][3] = {{20, 80, 20}, {20, 80, 20}, {80, 40, 10}, {80, 70, 20}, {60, 60, 100}, {70, 70, 90}, {80, 80, 100}, {50, 50, 70}, {60, 60, 90}};

    for (int i = 0; i < 9; ++i)
    {
        int by = 36 + i * 72;
        SDL_Rect btn = {8, by, sw - 16, 60};
        SDL_SetRenderDrawColor(r, colors[i][0], colors[i][1], colors[i][2], 255);
        SDL_RenderFillRect(r, &btn);
        SDL_SetRenderDrawColor(r, 80, 120, 80, 255);
        SDL_RenderDrawRect(r, &btn);

        // Icons
        if (i == 0 || i == 1)
        { // Arduino boards
            SDL_SetRenderDrawColor(r, 40, 160, 40, 255);
            SDL_Rect ic = {sw / 2 - 15, by + 8, 30, 42};
            SDL_RenderFillRect(r, &ic);
        }
        else if (i == 2)
        { // LED
            SDL_SetRenderDrawColor(r, 255, 100, 20, 255);
            fillCircle(r, sw / 2, by + 28, 14);
        }
        else if (i == 3)
        { // Resistor
            SDL_SetRenderDrawColor(r, 180, 140, 60, 255);
            SDL_Rect ic = {sw / 2 - 8, by + 14, 16, 30};
            SDL_RenderFillRect(r, &ic);
        }
        else if (i == 4)
        { // Push Button
            SDL_SetRenderDrawColor(r, 120, 120, 140, 255);
            SDL_Rect ic = {sw / 2 - 10, by + 15, 20, 20};
            SDL_RenderFillRect(r, &ic);
        }
        else if (i == 5)
        { // Potentiometer
            SDL_SetRenderDrawColor(r, 100, 100, 120, 255);
            fillCircle(r, sw / 2, by + 25, 12);
        }
        else if (i == 6)
        { // Servo
            SDL_SetRenderDrawColor(r, 110, 110, 130, 255);
            SDL_Rect ic = {sw / 2 - 12, by + 10, 24, 35};
            SDL_RenderFillRect(r, &ic);
        }
        else if (i == 7)
        { // UART Terminal
            SDL_SetRenderDrawColor(r, 60, 60, 90, 255);
            SDL_Rect ic = {sw / 2 - 14, by + 12, 28, 20};
            SDL_RenderFillRect(r, &ic);
        }
        else if (i == 8)
        { // I2C Device
            SDL_SetRenderDrawColor(r, 70, 70, 110, 255);
            SDL_Rect ic = {sw / 2 - 12, by + 15, 24, 18};
            SDL_RenderFillRect(r, &ic);
        }

        const char *lbl = (i == 0) ? "Arduino Nano" : (i == 1) ? "Arduino UNO"
                                                  : (i == 2)   ? "LED"
                                                  : (i == 3)   ? "Resistor"
                                                  : (i == 4)   ? "Push Button"
                                                  : (i == 5)   ? "Potentiometer"
                                                  : (i == 6)   ? "Servo"
                                                  : (i == 7)   ? "UART Terminal"
                                                               : "I2C Device";
        renderText(r, lbl, 8, by + 46, 180, 200, 180);
        m_palette.push_back({kinds[i], btn, lbl});
    }

    // Help text
    renderText(r, "Drag to canvas", 4, winH - 80, 80, 80, 110);
    renderText(r, "Click pin->pin", 4, winH - 65, 80, 80, 110);
    renderText(r, "to wire", 4, winH - 50, 80, 80, 110);
    renderText(r, "Del = remove", 4, winH - 35, 80, 80, 110);
    renderText(r, "Scroll = zoom", 4, winH - 20, 80, 80, 110);
}

bool Renderer::onMouseDown(int x, int y, int winW)
{
    m_paletteClicked = -1;
    int handleX = winW - m_editorWidth;
    if (x >= handleX - 6 && x <= handleX + 6)
    {
        m_resizingEditor = true;
        return true;
    }

    for (auto &item : m_palette)
    {
        if (x >= item.rect.x && x < item.rect.x + item.rect.w &&
            y >= item.rect.y && y < item.rect.y + item.rect.h)
        {
            m_paletteClicked = item.kind;
            return true;
        }
    }
    return false;
}

bool Renderer::onMouseMove(int x, int winW)
{
    if (!m_resizingEditor)
        return false;
    int maxWidth = std::max(220, winW - sidebarWidth() - 120);
    m_editorWidth = std::clamp(winW - x, 220, maxWidth);
    return true;
}

void Renderer::onMouseUp()
{
    m_resizingEditor = false;
}
