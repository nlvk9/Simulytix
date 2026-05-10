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
    for (int y = -rad; y <= rad; ++y)
    {
        int dx = (int)std::sqrt((float)(rad * rad - y * y));
        SDL_RenderDrawLine(r, cx - dx, cy + y, cx + dx, cy + y);
    }
}

void Renderer::drawCircleOutline(SDL_Renderer *r, int cx, int cy, int rad)
{
    int x = rad, y = 0, err = 0;
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

    // Background
    SDL_SetRenderDrawColor(r, 14, 14, 22, 255);
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
    if (gridSpacing < 6)
        gridSpacing *= 4;

    float offX = std::fmod(-c.camX() * zoom, gridSpacing);
    float offY = std::fmod(-c.camY() * zoom, gridSpacing);

    SDL_SetRenderDrawColor(r, 30, 30, 46, 255);
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
        // PCB body
        SDL_SetRenderDrawColor(r, 20, 60, 20, 255);
        SDL_Rect body = {(int)sx, (int)sy, (int)sw, (int)sh};
        SDL_RenderFillRect(r, &body);
        SDL_SetRenderDrawColor(r, 40, 120, 40, 255);
        SDL_RenderDrawRect(r, &body);

        // Label
        renderText(r, "Arduino", (int)sx + (int)(sw / 2) - 24, (int)sy + (int)(sh / 2) - 10,
                   120, 255, 120);
        renderText(r, "  Nano", (int)sx + (int)(sw / 2) - 24, (int)sy + (int)(sh / 2) + 2,
                   120, 255, 120);

        // Pins
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX) + ox;
            float py = canvas.worldToScreenY(comp.y + p.localY);

            // Pin dot
            uint8_t pr = 180, pg = 180, pb = 180;
            if (p.type == PinType::Ground)
            {
                pr = 60;
                pg = 60;
                pb = 60;
            }
            if (p.type == PinType::Power)
            {
                pr = 220;
                pg = 60;
                pb = 60;
            }
            SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
            SDL_Rect pinDot = {(int)px - 3, (int)py - 3, 6, 6};
            SDL_RenderFillRect(r, &pinDot);

            // Pin label (only at reasonable zoom)
            if (zoom > 0.6f && m_fontSm)
            {
                bool leftSide = (p.localX == 0.f);
                int lx = leftSide ? (int)px - 30 : (int)px + 6;
                SDL_Color col = {140, 200, 140, 255};
                SDL_Surface *surf = TTF_RenderText_Blended(m_fontSm, p.label.c_str(), col);
                if (surf)
                {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
                    SDL_Rect dst = {lx, (int)py - surf->h / 2, surf->w, surf->h};
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
        int rad = (int)(std::min(sw, sh) * 0.38f);
        int glow = rad + (int)(30 * zoom);

        if (ledOn)
        {
            float pulse = 0.7f + 0.3f * std::sin(glowPhase);
            for (int gr = glow; gr >= rad + 2; gr -= 2)
            {
                float t = (float)(gr - rad - 2) / (float)(glow - rad - 2);
                uint8_t alpha = (uint8_t)((1.f - t) * pulse * 160.f);
                SDL_SetRenderDrawColor(r, 255, 80, 0, alpha);
                drawCircleOutline(r, cx, cy, gr);
            }
            SDL_SetRenderDrawColor(r, 255, 120, 0, 255);
            fillCircle(r, cx, cy, rad);
            SDL_SetRenderDrawColor(r, 255, 220, 140, 180);
            fillCircle(r, cx - (int)(rad * 0.3f), cy - (int)(rad * 0.3f), rad / 3);
        }
        else
        {
            SDL_SetRenderDrawColor(r, 50, 20, 8, 255);
            fillCircle(r, cx, cy, rad);
        }

        // Outline + leads
        SDL_SetRenderDrawColor(r, 120, 60, 20, 255);
        drawCircleOutline(r, cx, cy, rad + 1);

        // Anode top lead
        float apy = canvas.worldToScreenY(comp.y + comp.pins[0].localY);
        SDL_SetRenderDrawColor(r, 160, 160, 160, 255);
        SDL_RenderDrawLine(r, cx, (int)apy, cx, cy - rad);

        // Cathode bottom lead
        float cpy = canvas.worldToScreenY(comp.y + comp.pins[1].localY);
        SDL_RenderDrawLine(r, cx, cy + rad, cx, (int)cpy);

        // Pin dots
        SDL_SetRenderDrawColor(r, 200, 200, 80, 255);
        SDL_Rect ad = {cx - 4, (int)apy - 4, 8, 8};
        SDL_RenderFillRect(r, &ad);
        SDL_SetRenderDrawColor(r, 80, 80, 200, 255);
        SDL_Rect cd = {cx - 4, (int)cpy - 4, 8, 8};
        SDL_RenderFillRect(r, &cd);

        if (zoom > 0.5f)
        {
            renderText(r, "+", cx - 12, (int)apy - 14, 220, 220, 80);
            renderText(r, "-", cx - 10, (int)cpy + 2, 80, 80, 220);
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

        // Button body
        SDL_SetRenderDrawColor(r, comp.buttonPressed ? 80 : 120,
                               comp.buttonPressed ? 80 : 120,
                               comp.buttonPressed ? 90 : 120,
                               255);
        SDL_Rect body = {cx - w / 2, cy - h / 2, w, h};
        SDL_RenderFillRect(r, &body);

        // Button top (pressed state simulation)
        SDL_SetRenderDrawColor(r, comp.buttonPressed ? 100 : 80,
                               comp.buttonPressed ? 100 : 80,
                               comp.buttonPressed ? 120 : 80,
                               255);
        SDL_Rect top = {cx - w / 2 + 2,
                        cy - h / 2 + 2 + (comp.buttonPressed ? 4 : 0),
                        w - 4,
                        h / 2 - 2};
        SDL_RenderFillRect(r, &top);

        // Label
        renderText(r, "BTN", cx - 12, cy - 5, 220, 220, 220);

        // Pins
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        for (auto &p : comp.pins)
        {
            float px = canvas.worldToScreenX(comp.x + p.localX);
            float py = canvas.worldToScreenY(comp.y + p.localY);
            int endx = (int)px + (p.id == "BTN_A" ? 12 : -12);
            int endy = (int)py;
            SDL_RenderDrawLine(r, (int)px, (int)py, endx, endy);
            fillCircle(r, endx, endy, 5);
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
    const char *labels[] = {"Arduino\n Nano", "LED", "Resistor", "Push\nButton"};
    const int kinds[] = {0, 1, 2, 3};
    const int colors[][3] = {{20, 80, 20}, {80, 40, 10}, {80, 70, 20}, {60, 60, 100}};

    for (int i = 0; i < 4; ++i)
    {
        int by = 36 + i * 72;
        SDL_Rect btn = {8, by, sw - 16, 60};
        SDL_SetRenderDrawColor(r, colors[i][0], colors[i][1], colors[i][2], 255);
        SDL_RenderFillRect(r, &btn);
        SDL_SetRenderDrawColor(r, 80, 120, 80, 255);
        SDL_RenderDrawRect(r, &btn);

        // Icon
        if (i == 0)
        { // Nano
            SDL_SetRenderDrawColor(r, 40, 160, 40, 255);
            SDL_Rect ic = {sw / 2 - 15, by + 8, 30, 42};
            SDL_RenderFillRect(r, &ic);
        }
        else if (i == 1)
        { // LED
            SDL_SetRenderDrawColor(r, 255, 100, 20, 255);
            fillCircle(r, sw / 2, by + 28, 14);
        }
        else
        { // Resistor
            SDL_SetRenderDrawColor(r, 180, 140, 60, 255);
            SDL_Rect ic = {sw / 2 - 8, by + 14, 16, 30};
            SDL_RenderFillRect(r, &ic);
        }

        const char *lbl = (i == 0) ? "Arduino Nano" : (i == 1) ? "LED"
                                                  : (i == 2)   ? "Resistor"
                                                               : "Push Button";
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
