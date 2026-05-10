#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class CodeEditor
{
public:
    CodeEditor();
    ~CodeEditor();

    bool init();

    void setCode(const std::string &s);
    std::string code() const { return m_code; }

    // Returns true when Upload was clicked this frame
    bool uploadClicked() const { return m_uploadClicked; }
    void clearUploadClick() { m_uploadClicked = false; }

    // Returns true if Stop was clicked
    bool stopClicked() const { return m_stopClicked; }
    void clearStopClick() { m_stopClicked = false; }

    void onMouseDown(int x, int y, int button);
    void onTextInput(const char *text);
    void onKey(SDL_Keycode sym, SDL_Keymod mod);

    void render(SDL_Renderer *r, int panelX, int panelY, int panelW, int panelH,
                bool simRunning, const std::string &statusMsg);

    void setPanelBounds(int x, int y, int w, int h)
    {
        m_px = x;
        m_py = y;
        m_pw = w;
        m_ph = h;
    }

    void setFocused(bool f) { m_focused = f; }

    void pushUndo();
    void undo();

private:
    void renderText(SDL_Renderer *r, TTF_Font *font,
                    const std::string &text, int x, int y,
                    uint8_t R, uint8_t G, uint8_t B);
    void drawRect(SDL_Renderer *r, int x, int y, int w, int h,
                  uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255);

    std::string m_code;
    std::vector<std::string> m_lines;
    int m_cursorLine = 0;
    int m_cursorCol = 0;
    int m_scrollLine = 0;

    std::vector<std::string> m_undoStack;
    int m_undoIndex = -1;

    bool m_uploadClicked = false;
    bool m_stopClicked = false;
    bool m_focused = false;

    int m_px = 0, m_py = 0, m_pw = 300, m_ph = 600;

    TTF_Font *m_font = nullptr;
    TTF_Font *m_fontBold = nullptr;
    int m_lineH = 16;
};
