#pragma once
#include "Canvas.h"
#include "Simulator.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool init(SDL_Renderer *r);

    void drawAll(SDL_Renderer *r,
                 Canvas &canvas,
                 const Simulator &sim,
                 int winW, int winH,
                 float glowPhase);

    // Sidebar palette — returns ComponentKind if one was clicked, -1 otherwise
    int sidebarWidth() const { return 130; }
    int editorWidth() const { return m_editorWidth; }

    // Which palette item was clicked this frame
    int paletteClickedKind() const { return m_paletteClicked; }
    void clearPaletteClick() { m_paletteClicked = -1; }

    bool onMouseDown(int x, int y, int winW);
    bool onMouseMove(int x, int winW);
    void onMouseUp();

private:
    void drawGrid(SDL_Renderer *r, const Canvas &c, int cw, int ch);
    void drawComponent(SDL_Renderer *r, const Canvas &canvas,
                       const Component &comp, bool selected,
                       bool ledOn, float glowPhase);
    void drawWires(SDL_Renderer *r, const Canvas &canvas);
    void drawWireInProgress(SDL_Renderer *r, const Canvas &canvas);
    void drawSidebar(SDL_Renderer *r, int winH);
    void drawPinLabels(SDL_Renderer *r, const Canvas &canvas, const Component &comp);

    void fillCircle(SDL_Renderer *r, int cx, int cy, int rad);
    void drawCircleOutline(SDL_Renderer *r, int cx, int cy, int rad);
    void renderText(SDL_Renderer *r, const std::string &txt,
                    int x, int y, uint8_t R, uint8_t G, uint8_t B);

    TTF_Font *m_font = nullptr;
    TTF_Font *m_fontSm = nullptr;
    int m_paletteClicked = -1;
    int m_editorWidth = 300;
    bool m_resizingEditor = false;

    // Sidebar button rects
    struct PaletteItem
    {
        int kind;
        SDL_Rect rect;
        const char *label;
    };
    std::vector<PaletteItem> m_palette;
};
