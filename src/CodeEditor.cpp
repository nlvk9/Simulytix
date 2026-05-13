#include "CodeEditor.h"
#include <algorithm>
#include <sstream>
#include <cstring>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::vector<std::string> splitLines(const std::string &s)
{
    std::vector<std::string> lines;
    std::istringstream ss(s);
    std::string line;
    while (std::getline(ss, line))
        lines.push_back(line);
    if (lines.empty())
        lines.push_back("");
    return lines;
}

static std::string joinLines(const std::vector<std::string> &lines)
{
    std::string out;
    for (int i = 0; i < (int)lines.size(); ++i)
    {
        if (i)
            out += '\n';
        out += lines[i];
    }
    return out;
}

// ── Very simple keyword detection ─────────────────────────────────────────────
static bool isKeyword(const std::string &w)
{
    static const char *kw[] = {
        "void", "int", "uint8_t", "uint32_t", "bool", "const", "return",
        "if", "else", "while", "for", "true", "false", "setup", "loop",
        "pinMode", "digitalWrite", "delay", "HIGH", "LOW", "OUTPUT", "INPUT", nullptr};
    for (int i = 0; kw[i]; ++i)
        if (w == kw[i])
            return true;
    return false;
}

static bool isType(const std::string &w)
{
    static const char *types[] = {
        "void", "int", "uint8_t", "uint32_t", "uint16_t", "uint64_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "bool", "float", "double", "char", "const", nullptr};
    for (int i = 0; types[i]; ++i)
        if (w == types[i])
            return true;
    return false;
}

// ── CodeEditor ────────────────────────────────────────────────────────────────

CodeEditor::CodeEditor()
{
    m_code = "const uint8_t LED_PIN = 13;\n\nvoid setup() {\n    pinMode(LED_PIN, OUTPUT);\n}\n\nvoid loop() {\n    digitalWrite(LED_PIN, HIGH);\n    delay(500);\n    digitalWrite(LED_PIN, LOW);\n    delay(500);\n}\n";
    m_lines = splitLines(m_code);
}

CodeEditor::~CodeEditor()
{
    if (m_font)
        TTF_CloseFont(m_font);
    if (m_fontBold)
        TTF_CloseFont(m_fontBold);
}

bool CodeEditor::init()
{
    if (TTF_Init() != 0)
        return false;
    // Try to find a crisp monospace font (VSCode-style fonts first)
    const char *candidates[] = {
        // macOS VSCode fonts
        "/System/Library/Fonts/Supplemental/CascadiaCode.ttf",
        "/System/Library/Fonts/Supplemental/Cascadia Code.ttf",
        "/System/Library/Fonts/Supplemental/Menlo.ttc",
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        // Linux VSCode fonts
        "/usr/share/fonts/truetype/cascadia/CascadiaCode.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        // Fallback
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        nullptr};
    for (int i = 0; candidates[i]; ++i)
    {
        m_font = TTF_OpenFont(candidates[i], 24);
        if (m_font)
            break;
    }
    if (!m_font)
        return false;
    m_lineH = TTF_FontLineSkip(m_font) + 8;
    return true;
}

void CodeEditor::setCode(const std::string &s)
{
    m_code = s;
    m_lines = splitLines(s);
    m_cursorLine = std::min(m_cursorLine, (int)m_lines.size() - 1);
    m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorLine].size());
    parseVariablesFromCode();
}

void CodeEditor::pushUndo()
{
    if (m_undoIndex < (int)m_undoStack.size() - 1)
    {
        m_undoStack.resize(m_undoIndex + 1);
    }
    m_undoStack.push_back(m_code);
    m_undoIndex = m_undoStack.size() - 1;
    if (m_undoStack.size() > 50) // limit
    {
        m_undoStack.erase(m_undoStack.begin());
        m_undoIndex--;
    }
}

void CodeEditor::undo()
{
    if (m_undoIndex > 0)
    {
        m_undoIndex--;
        setCode(m_undoStack[m_undoIndex]);
    }
}

// ── Variable Tracking ─────────────────────────────────────────────────────────

void CodeEditor::setVariableValue(const std::string &varName, const std::string &value)
{
    if (m_variables.find(varName) != m_variables.end())
    {
        m_variables[varName].value = value;
    }
    else
    {
        Variable var;
        var.name = varName;
        var.value = value;
        m_variables[varName] = var;
    }
}

std::string CodeEditor::getVariableValue(const std::string &varName) const
{
    auto it = m_variables.find(varName);
    if (it != m_variables.end())
        return it->second.value;
    return "";
}

void CodeEditor::parseVariablesFromCode()
{
    m_variables.clear();
    for (int lineIdx = 0; lineIdx < (int)m_lines.size(); ++lineIdx)
    {
        const std::string &line = m_lines[lineIdx];
        // Simple regex-like parsing for variable declarations
        // Pattern: [const] type varName [= value];
        int pos = 0;
        std::string type, varName;

        // Skip leading whitespace
        while (pos < (int)line.size() && std::isspace(line[pos]))
            pos++;

        // Check for 'const'
        if (line.substr(pos).find("const") == 0)
            pos += 5;

        // Skip whitespace
        while (pos < (int)line.size() && std::isspace(line[pos]))
            pos++;

        // Extract type
        int typeStart = pos;
        while (pos < (int)line.size() && (std::isalnum(line[pos]) || line[pos] == '_'))
            pos++;
        type = line.substr(typeStart, pos - typeStart);

        if (!isType(type))
            continue;

        // Skip whitespace
        while (pos < (int)line.size() && std::isspace(line[pos]))
            pos++;

        // Extract variable name
        int nameStart = pos;
        while (pos < (int)line.size() && (std::isalnum(line[pos]) || line[pos] == '_'))
            pos++;
        varName = line.substr(nameStart, pos - nameStart);

        if (!varName.empty())
        {
            Variable var;
            var.name = varName;
            var.type = type;
            var.line = lineIdx;
            var.value = "";

            // Try to extract initial value
            while (pos < (int)line.size() && line[pos] != '=' && line[pos] != ';')
                pos++;
            if (pos < (int)line.size() && line[pos] == '=')
            {
                pos++;
                while (pos < (int)line.size() && std::isspace(line[pos]))
                    pos++;
                int valStart = pos;
                while (pos < (int)line.size() && line[pos] != ';')
                    pos++;
                var.value = line.substr(valStart, pos - valStart);
                // Trim trailing whitespace from value
                while (!var.value.empty() && std::isspace(var.value.back()))
                    var.value.pop_back();
            }

            m_variables[varName] = var;
        }
    }
}

CodeEditor::TokenType CodeEditor::getTokenType(const std::string &word, bool followedByParen)
{
    if (isType(word))
        return TT_TYPE;
    else if (isKeyword(word))
        return TT_KEYWORD;
    else if (followedByParen)
        return TT_FUNCTION;
    else if (m_variables.find(word) != m_variables.end())
        return TT_VARIABLE;
    return TT_UNKNOWN;
}

void CodeEditor::onMouseDown(int x, int y, int button)
{
    if (button != SDL_BUTTON_LEFT)
        return;
    if (x < 0 || x > m_pw)
    {
        m_focused = false;
        return;
    }
    if (y < m_py || y > m_py + m_ph)
    {
        m_focused = false;
        return;
    }

    m_focused = true;

    int btnY = m_py + m_ph - 82;

    if (y >= btnY && y < btnY + 32 && x >= 10 && x < m_pw - 10)
    {
        m_uploadClicked = true;
        return;
    }
    if (y >= btnY + 36 && y < btnY + 36 + 28 && x >= 10 && x < m_pw - 10)
    {
        m_stopClicked = true;
        return;
    }

    // Click into text
    int textTop = m_py + 36;
    int lineIdx = (y - textTop) / m_lineH + m_scrollLine;
    lineIdx = std::max(0, std::min((int)m_lines.size() - 1, lineIdx));
    m_cursorLine = lineIdx;
    m_cursorCol = (int)m_lines[lineIdx].size(); // approximate
}

void CodeEditor::onTextInput(const char *text)
{
    if (!m_focused)
        return;
    pushUndo();
    std::string ins(text);
    auto &line = m_lines[m_cursorLine];
    line.insert(m_cursorCol, ins);
    m_cursorCol += (int)ins.size();
    m_code = joinLines(m_lines);
}

void CodeEditor::onKey(SDL_Keycode sym, SDL_Keymod mod)
{
    if (!m_focused)
        return;
    auto &line = m_lines[m_cursorLine];

    if (sym == SDLK_z && (mod & KMOD_CTRL || mod & KMOD_GUI))
    {
        undo();
        return;
    }

    pushUndo();
    switch (sym)
    {
    case SDLK_RETURN:
    {
        std::string rest = line.substr(m_cursorCol);
        line = line.substr(0, m_cursorCol);
        // Auto-indent
        std::string indent;
        for (char ch : line)
        {
            if (ch == ' ')
                indent += ' ';
            else
                break;
        }
        if (!line.empty() && line.back() == '{')
            indent += "    ";
        m_lines.insert(m_lines.begin() + m_cursorLine + 1, indent + rest);
        m_cursorLine++;
        m_cursorCol = (int)indent.size();
        break;
    }
    case SDLK_BACKSPACE:
        if (m_cursorCol > 0)
        {
            line.erase(m_cursorCol - 1, 1);
            m_cursorCol--;
        }
        else if (m_cursorLine > 0)
        {
            std::string carry = line;
            m_lines.erase(m_lines.begin() + m_cursorLine);
            m_cursorLine--;
            m_cursorCol = (int)m_lines[m_cursorLine].size();
            m_lines[m_cursorLine] += carry;
        }
        break;
    case SDLK_DELETE:
        if (m_cursorCol < (int)line.size())
        {
            line.erase(m_cursorCol, 1);
        }
        else if (m_cursorLine + 1 < (int)m_lines.size())
        {
            m_lines[m_cursorLine] += m_lines[m_cursorLine + 1];
            m_lines.erase(m_lines.begin() + m_cursorLine + 1);
        }
        break;
    case SDLK_LEFT:
        if (m_cursorCol > 0)
            m_cursorCol--;
        else if (m_cursorLine > 0)
        {
            m_cursorLine--;
            m_cursorCol = (int)m_lines[m_cursorLine].size();
        }
        break;
    case SDLK_RIGHT:
        if (m_cursorCol < (int)line.size())
            m_cursorCol++;
        else if (m_cursorLine + 1 < (int)m_lines.size())
        {
            m_cursorLine++;
            m_cursorCol = 0;
        }
        break;
    case SDLK_UP:
        if (m_cursorLine > 0)
        {
            m_cursorLine--;
            m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorLine].size());
        }
        break;
    case SDLK_DOWN:
        if (m_cursorLine + 1 < (int)m_lines.size())
        {
            m_cursorLine++;
            m_cursorCol = std::min(m_cursorCol, (int)m_lines[m_cursorLine].size());
        }
        break;
    case SDLK_TAB:
    {
        std::string spaces = "    ";
        line.insert(m_cursorCol, spaces);
        m_cursorCol += 4;
        break;
    }
    case SDLK_F5:
        m_uploadClicked = true;
        break;
    default:
        break;
    }
    // Clamp cursor
    m_cursorLine = std::max(0, std::min((int)m_lines.size() - 1, m_cursorLine));
    m_cursorCol = std::max(0, std::min((int)m_lines[m_cursorLine].size(), m_cursorCol));
    m_code = joinLines(m_lines);

    // Scroll to keep cursor visible
    int visLines = (m_ph - 36 - 90) / m_lineH;
    if (m_cursorLine < m_scrollLine)
        m_scrollLine = m_cursorLine;
    if (m_cursorLine >= m_scrollLine + visLines)
        m_scrollLine = m_cursorLine - visLines + 1;
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void CodeEditor::renderText(SDL_Renderer *r, TTF_Font *font,
                            const std::string &text, int x, int y,
                            uint8_t R, uint8_t G, uint8_t B)
{
    if (!font || text.empty())
        return;
    SDL_Color col = {R, G, B, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), col);
    if (!surf)
        return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void CodeEditor::drawRect(SDL_Renderer *r, int x, int y, int w, int h,
                          uint8_t R, uint8_t G, uint8_t B, uint8_t A)
{
    SDL_SetRenderDrawColor(r, R, G, B, A);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

void CodeEditor::render(SDL_Renderer *r, int panelX, int panelY, int panelW, int panelH,
                        bool simRunning, const std::string &statusMsg)
{
    m_px = panelX;
    m_py = panelY;
    m_pw = panelW;
    m_ph = panelH;
    if (!m_font)
        return;

    // Background
    drawRect(r, panelX, panelY, panelW, panelH, 28, 28, 38);

    // Header
    drawRect(r, panelX, panelY, panelW, 32, 20, 20, 32);
    renderText(r, m_font, "  sketch.h", panelX + 4, panelY + 8, 160, 200, 255);

    // Line numbers + code
    int textTop = panelY + 36;
    int codeLeft = panelX + 44; // after line numbers
    int visLines = (panelH - 36 - 90) / m_lineH;

    // Clip to text area
    SDL_Rect clip = {panelX, textTop, panelW, panelH - 36 - 90};
    SDL_RenderSetClipRect(r, &clip);

    for (int i = 0; i < visLines; ++i)
    {
        int lineIdx = i + m_scrollLine;
        if (lineIdx >= (int)m_lines.size())
            break;
        int ly = textTop + i * m_lineH;

        // Cursor line highlight
        if (lineIdx == m_cursorLine && m_focused)
            drawRect(r, panelX, ly, panelW, m_lineH, 40, 40, 60);

        // Line number
        std::string ln = std::to_string(lineIdx + 1);
        renderText(r, m_font, ln, panelX + 8, ly, 120, 120, 145);

        // Code with enhanced syntax highlighting
        const std::string &src = m_lines[lineIdx];
        int cx = codeLeft;
        int j = 0;
        while (j < (int)src.size())
        {
            // Comment
            if (j + 1 < (int)src.size() && src[j] == '/' && src[j + 1] == '/')
            {
                renderText(r, m_font, src.substr(j), cx, ly, 100, 140, 100);
                break;
            }
            // String
            if (src[j] == '"')
            {
                int end = (int)src.find('"', j + 1);
                if (end == (int)std::string::npos)
                    end = (int)src.size() - 1;
                renderText(r, m_font, src.substr(j, end - j + 1), cx, ly, 206, 145, 120);
                int charW = 0;
                TTF_SizeText(m_font, src.substr(j, end - j + 1).c_str(), &charW, nullptr);
                cx += charW;
                j = end + 1;
                continue;
            }
            // Word (identifiers, keywords, types, variables, functions)
            if (std::isalpha(src[j]) || src[j] == '_')
            {
                int end = j;
                while (end < (int)src.size() && (std::isalnum(src[end]) || src[end] == '_'))
                    end++;
                std::string word = src.substr(j, end - j);

                // Check if followed by parentheses (function call)
                int nextPos = end;
                while (nextPos < (int)src.size() && std::isspace(src[nextPos]))
                    nextPos++;
                bool followedByParen = (nextPos < (int)src.size() && src[nextPos] == '(');

                TokenType type = getTokenType(word, followedByParen);

                uint8_t wr = 220, wg = 220, wb = 220; // Default: white

                switch (type)
                {
                case TT_TYPE:
                    // Type keywords: Cyan
                    wr = 86;
                    wg = 156;
                    wb = 214;
                    break;
                case TT_KEYWORD:
                    // Other keywords: Magenta/Purple
                    wr = 198;
                    wg = 120;
                    wb = 221;
                    break;
                case TT_FUNCTION:
                    // Function names: Yellow/Orange
                    wr = 220;
                    wg = 198;
                    wb = 120;
                    break;
                case TT_VARIABLE:
                    // Variable names: Light Blue/Cyan
                    wr = 156;
                    wg = 220;
                    wb = 220;
                    break;
                case TT_UNKNOWN:
                default:
                    // Unknown: Default light gray
                    wr = 220;
                    wg = 220;
                    wb = 220;
                    break;
                }

                renderText(r, m_font, word, cx, ly, wr, wg, wb);
                int charW = 0;
                TTF_SizeText(m_font, word.c_str(), &charW, nullptr);
                cx += charW;
                j = end;
                continue;
            }
            // Number
            if (std::isdigit(src[j]))
            {
                int end = j;
                while (end < (int)src.size() && std::isdigit(src[end]))
                    end++;
                std::string num = src.substr(j, end - j);
                renderText(r, m_font, num, cx, ly, 181, 206, 168);
                int charW = 0;
                TTF_SizeText(m_font, num.c_str(), &charW, nullptr);
                cx += charW;
                j = end;
                continue;
            }
            // Single char
            std::string ch(1, src[j]);
            renderText(r, m_font, ch, cx, ly, 200, 200, 200);
            int charW = 0;
            TTF_SizeText(m_font, ch.c_str(), &charW, nullptr);
            cx += charW;
            j++;
        }

        // Draw cursor
        if (lineIdx == m_cursorLine && m_focused)
        {
            std::string before = src.substr(0, m_cursorCol);
            int beforeW = 0;
            if (!before.empty())
                TTF_SizeText(m_font, before.c_str(), &beforeW, nullptr);
            SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
            SDL_RenderDrawLine(r, codeLeft + beforeW, ly, codeLeft + beforeW, ly + m_lineH - 2);
        }
    }

    SDL_RenderSetClipRect(r, nullptr);

    // Divider line between code and buttons
    SDL_SetRenderDrawColor(r, 80, 80, 100, 120);
    SDL_RenderDrawLine(r, panelX + 10, panelY + panelH - 90,
                       panelX + panelW - 10, panelY + panelH - 90);

    // ── Buttons ───────────────────────────────────────────────────────────────
    int btnY = panelY + panelH - 82;
    SDL_Rect uploadRect = {panelX + 10, btnY, panelW - 20, 32};
    SDL_Rect stopRect = {panelX + 10, btnY + 36, panelW - 20, 28};

    drawRect(r, uploadRect.x, uploadRect.y, uploadRect.w, uploadRect.h,
             simRunning ? 42 : 36,
             simRunning ? 100 : 120,
             simRunning ? 42 : 70);
    SDL_SetRenderDrawColor(r, 100, 180, 120, 255);
    SDL_RenderDrawRect(r, &uploadRect);
    renderText(r, m_font,
               simRunning ? "  ▶  Running..." : "  ▶  Upload & Run",
               uploadRect.x + 14, uploadRect.y + 8,
               simRunning ? 90 : 220,
               simRunning ? 220 : 240,
               simRunning ? 90 : 180);

    drawRect(r, stopRect.x, stopRect.y, stopRect.w, stopRect.h, 80, 40, 40);
    SDL_SetRenderDrawColor(r, 160, 80, 80, 255);
    SDL_RenderDrawRect(r, &stopRect);
    renderText(r, m_font, "  ■  Stop", stopRect.x + 14, stopRect.y + 7,
               240, 140, 140);

    // Status bar
    drawRect(r, panelX, panelY + panelH - 14, panelW, 14, 18, 18, 28);
    if (!statusMsg.empty())
    {
        bool isErr = statusMsg.find("Error") != std::string::npos ||
                     statusMsg.find("error") != std::string::npos;
        renderText(r, m_font, " " + statusMsg,
                   panelX + 4, panelY + panelH - 14,
                   isErr ? 220 : 160,
                   isErr ? 90 : 190,
                   isErr ? 90 : 130);
    }
}
