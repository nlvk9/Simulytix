/**
 * Simulytix — Arduino Circuit Simulator
 * Main event loop: wires Canvas, CodeEditor, Renderer, and Simulator together.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>

#include "Canvas.h"
#include "CodeEditor.h"
#include "Renderer.h"
#include "Simulator.h"

static const int WIN_W = 1280;
static const int WIN_H = 800;

int main(int /*argc*/, char ** /*argv*/)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow(
        "Simulytix",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    Canvas canvas;
    CodeEditor editor;
    Renderer rend;
    Simulator sim;

    editor.init();
    rend.init(renderer);

    std::string status = "Place components from the sidebar, wire pins, then Upload.";
    float glowPhase = 0.f;

    // Pre-populate with a Nano + LED example
    canvas.addComponent(ComponentKind::ArduinoNano, 200, 100);
    canvas.addComponent(ComponentKind::LED, 420, 180);

    bool running = true;
    SDL_StartTextInput();

    while (running)
    {
        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);

        int sw = rend.sidebarWidth();
        int ew = rend.editorWidth();
        int editorX = winW - ew;

        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            switch (ev.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
            {
                SDL_Keycode sym = ev.key.keysym.sym;
                if (sym == SDLK_ESCAPE)
                {
                    running = false;
                    break;
                }
                editor.onKey(sym, static_cast<SDL_Keymod>(ev.key.keysym.mod));
                canvas.onKey(sym);
                break;
            }

            case SDL_TEXTINPUT:
                editor.onTextInput(ev.text.text);
                break;

            case SDL_MOUSEBUTTONDOWN:
            {
                int mx = ev.button.x, my = ev.button.y;

                bool consumed = rend.onMouseDown(mx, my, winW);
                if (mx < sw)
                {
                    int kind = rend.paletteClickedKind();
                    if (kind >= 0)
                    {
                        // Place near center of canvas
                        float wx = canvas.screenToWorldX((winW - sw - ew) / 2);
                        float wy = canvas.screenToWorldY(winH / 2);
                        canvas.addComponent((ComponentKind)kind,
                                            wx + (float)(rand() % 80 - 40),
                                            wy + (float)(rand() % 80 - 40));
                        rend.clearPaletteClick();
                    }
                    break;
                }

                if (consumed)
                    break;

                // Editor panel
                if (mx >= editorX)
                {
                    editor.onMouseDown(mx - editorX, my, ev.button.button);

                    if (editor.uploadClicked())
                    {
                        editor.clearUploadClick();
                        status = "Starting simulation...";
                        sim.start(editor.code(),
                                  canvas.components(),
                                  canvas.wires());
                        if (sim.state() == SimState::Running)
                            status = "Simulation running.";
                        else
                            status = "Error: " + sim.errorMsg();
                    }

                    if (editor.stopClicked())
                    {
                        editor.clearStopClick();
                        sim.stop();
                        status = "Simulation stopped.";
                    }
                    break;
                }

                // Canvas
                canvas.onMouseDown(mx, my, ev.button.button);
                editor.setFocused(false);
                break;
            }

            case SDL_MOUSEBUTTONUP:
                rend.onMouseUp();
                canvas.onMouseUp(ev.button.x, ev.button.y, ev.button.button);
                break;

            case SDL_MOUSEMOTION:
                if (!rend.onMouseMove(ev.motion.x, winW))
                {
                    canvas.onMouseMove(ev.motion.x, ev.motion.y,
                                       ev.motion.xrel, ev.motion.yrel);
                }
                break;

            case SDL_MOUSEWHEEL:
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                if (mx >= sw && mx < editorX)
                    canvas.onScroll(mx, my, (float)ev.wheel.y);
                break;
            }

            default:
                break;
            }
        }

        glowPhase += 0.06f;
        if (glowPhase > 6.28f)
            glowPhase -= 6.28f;

        rend.drawAll(renderer, canvas, sim, winW, winH, glowPhase);

        // Draw code editor panel
        editor.render(renderer, editorX, 0, ew, winH,
                      sim.state() == SimState::Running, status);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    sim.stop();
    SDL_StopTextInput();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
