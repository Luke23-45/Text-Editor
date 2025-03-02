#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>



struct EditorState {
    std::vector<std::string> lines;
    std::string currentFilePath;
    int cursorX = 0;
    int cursorY = 0;
    bool isSelecting = false;
    int selectionStartX = 0;
    int selectionStartY = 0;
    std::vector<std::vector<std::string>> undoStack;
    std::vector<std::vector<std::string>> redoStack;
    std::string clipboardContent;
    int scrollOffsetY = 0;
    std::string statusMessage;
    bool darkTheme = true;       // Toggle between dark/light themes
    std::string searchQuery;     // For search functionality 
    bool searchActive = false;
};


// SDL Resources (RAII)

struct SDLResources {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    ~SDLResources(){
        if(font) TTF_CloseFont(font);
        if(renderer) SDL_DestroyRenderer(renderer);
        if(window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
};




namespace TextUtils {

    // Save current state (for undo/redo)
    void saveState(EditorState &state) {
        state.undoStack.push_back(state.lines);
        // Clear redo stack on new action.
        state.redoStack.clear();
    }

    void insertText(EditorState &state, const std::string &text) {
        saveState(state);
        if(state.cursorY >= state.lines.size())
            state.lines.push_back("");
        state.lines[state.cursorY].insert(state.cursorX, text);
        state.cursorX += text.size();
    }

    // Insert a newline with auto-indentation (copy leading whitespace)
    void insertNewLine(EditorState &state) {
        saveState(state);
        std::string currentLine = state.lines[state.cursorY];
        std::string indent;
        for (char ch : currentLine) {
            if (ch == ' ' || ch == '\t')
                indent.push_back(ch);
            else
                break;
        }
        std::string newLine = currentLine.substr(state.cursorX);
        state.lines[state.cursorY].erase(state.cursorX);
        state.lines.insert(state.lines.begin() + state.cursorY + 1, indent + newLine);
        state.cursorY++;
        state.cursorX = indent.size();
    }

    // Delete the character before the cursor; if at beginning merge with previous line.
    void deleteCharacter(EditorState &state) {
        if(state.cursorY < state.lines.size() && state.cursorX > 0) {
            saveState(state);
            state.lines[state.cursorY].erase(state.cursorX - 1, 1);
            state.cursorX--;
        } else if(state.cursorY > 0) {
            saveState(state);
            int prevLineLength = state.lines[state.cursorY - 1].size();
            state.lines[state.cursorY - 1] += state.lines[state.cursorY];
            state.lines.erase(state.lines.begin() + state.cursorY);
            state.cursorY--;
            state.cursorX = prevLineLength;
        }
    }

    // Undo the last action
    void undo(EditorState &state) {
        if(!state.undoStack.empty()){
            state.redoStack.push_back(state.lines);
            state.lines = state.undoStack.back();
            state.undoStack.pop_back();
            state.statusMessage = "Undo performed";
        }
    }

    // Redo the last undone action
    void redo(EditorState &state) {
        if(!state.redoStack.empty()){
            state.undoStack.push_back(state.lines);
            state.lines = state.redoStack.back();
            state.redoStack.pop_back();
            state.statusMessage = "Redo performed";
        }
    }

    // Copy text
    void copyText(EditorState &state) {
        if(state.isSelecting) {
            state.clipboardContent = state.lines[state.cursorY];
            state.statusMessage = "Text copied to clipboard";
        }
    }

    // Paste text from the clipboard.
    void pasteText(EditorState &state) {
        if(!state.clipboardContent.empty()){
            saveState(state);
            state.lines[state.cursorY].insert(state.cursorX, state.clipboardContent);
            state.cursorX += state.clipboardContent.size();
            state.statusMessage = "Text pasted from clipboard";
        }
    }

    void moveCursorToLineStart(EditorState &state) {
        state.cursorX = 0;
    }

    void moveCursorToLineEnd(EditorState &state) {
        if(state.cursorY < state.lines.size())
            state.cursorX = state.lines[state.cursorY].size();
    }

    void moveCursorPageUp(EditorState &state, int visibleLines) {
        state.cursorY = std::max(0, state.cursorY - visibleLines);
        state.scrollOffsetY = std::max(0, state.scrollOffsetY - visibleLines);
    }

    void moveCursorPageDown(EditorState &state, int visibleLines) {
        state.cursorY = std::min((int)state.lines.size()-1, state.cursorY + visibleLines);
        state.scrollOffsetY = std::min((int)state.lines.size() - visibleLines, state.scrollOffsetY + visibleLines);
    }
}


// File Utility Functions

namespace FileUtils {

    void newFile(EditorState &state) {
        state.lines.clear();
        state.lines.push_back("");
        state.currentFilePath.clear();
        state.cursorX = state.cursorY = 0;
        state.statusMessage = "New file created";
    }

    bool openFile(EditorState &state, const std::string &filename) {
        std::ifstream file(filename);
        if(!file.is_open()){
            state.statusMessage = "Error opening file: " + filename;
            return false;
        }
        state.lines.clear();
        std::string line;
        while(std::getline(file, line)) {
            state.lines.push_back(line);
        }
        state.currentFilePath = filename;
        state.cursorX = state.cursorY = 0;
        state.statusMessage = "File opened: " + filename;
        return true;
    }

    bool saveFile(EditorState &state, const std::string &filename = "") {
        std::string path = filename.empty() ? state.currentFilePath : filename;
        if(path.empty()){
            state.statusMessage = "No filename provided for saving.";
            return false;
        }
        std::ofstream file(path);
        if(!file.is_open()){
            state.statusMessage = "Error saving file: " + path;
            return false;
        }
        for (auto &line : state.lines)
            file << line << "\n";
        state.currentFilePath = path;
        state.statusMessage = "File saved: " + path;
        return true;
    }
}


// Rendering Utility Functions

namespace RenderUtils {

    const int STATUS_BAR_HEIGHT = 24;
    const int LINE_HEIGHT = 20;
    const int MARGIN = 10;
    const int LINE_NUMBER_WIDTH = 40;

    // A basic list of C/C++ keywords for highlighting.
    std::vector<std::string> keywords = {
        "int", "return", "if", "else", "for", "while", "struct", "void",
        "#include", "using", "namespace", "std", "class", "break", "continue"
    };

    // Check if a token is a keyword.
    bool isKeyword(const std::string &token) {
        for (const auto &kw : keywords)
            if (token == kw)
                return true;
        return false;
    }

    // Render a single line with rudimentary syntax highlighting.
    void renderLine(SDL_Renderer* renderer, TTF_Font* font, const std::string &line, int x, int y, bool darkTheme) {
        int offsetX = x;
        std::string token;
        for (size_t i = 0; i <= line.size(); i++) {
            char ch = (i < line.size()) ? line[i] : ' '; // force flush at end
            if (std::isalnum(ch) || ch == '_' || ch == '#') {
                token.push_back(ch);
            } else {
                if (!token.empty()) {
                    SDL_Color color;
                    if (isKeyword(token)) {
                        // Highlight keywords in blue-ish color.
                        color = darkTheme ? SDL_Color{0, 200, 255, 255} : SDL_Color{0, 0, 200, 255};
                    } else {
                        color = darkTheme ? SDL_Color{230, 230, 230, 255} : SDL_Color{0, 0, 0, 255};
                    }
                    SDL_Surface* surface = TTF_RenderText_Blended(font, token.c_str(), color);
                    if (surface) {
                        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                        if (texture) {
                            SDL_Rect destRect = { offsetX, y, surface->w, surface->h };
                            SDL_RenderCopy(renderer, texture, nullptr, &destRect);
                            SDL_DestroyTexture(texture);
                        }
                        offsetX += surface->w;
                        SDL_FreeSurface(surface);
                    }
                    token.clear();
                }
                // Render the delimiter character.
                std::string s(1, ch);
                SDL_Color color = darkTheme ? SDL_Color{230, 230, 230, 255} : SDL_Color{0, 0, 0, 255};
                SDL_Surface* surface = TTF_RenderText_Blended(font, s.c_str(), color);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    if (texture) {
                        SDL_Rect destRect = { offsetX, y, surface->w, surface->h };
                        SDL_RenderCopy(renderer, texture, nullptr, &destRect);
                        SDL_DestroyTexture(texture);
                    }
                    offsetX += surface->w;
                    SDL_FreeSurface(surface);
                }
            }
        }
    }

    // Render the entire editor (text area, line numbers, cursor, and status bar)
    void renderText(SDL_Renderer* renderer, TTF_Font* font, const EditorState &state, int windowWidth, int windowHeight) {
        // Clear with background color.
        if (state.darkTheme)
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        else
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        int startLine = state.scrollOffsetY;
        int maxLines = (windowHeight - STATUS_BAR_HEIGHT - MARGIN) / LINE_HEIGHT;
        // Render each visible line.
        for (int i = 0; i < maxLines && (i + startLine) < state.lines.size(); i++) {
            int y = MARGIN + i * LINE_HEIGHT;
            // Render line number.
            std::string lineNum = std::to_string(i + startLine + 1);
            SDL_Color numColor = state.darkTheme ? SDL_Color{150, 150, 150, 255} : SDL_Color{100, 100, 100, 255};
            SDL_Surface* numSurface = TTF_RenderText_Blended(font, lineNum.c_str(), numColor);
            if (numSurface) {
                SDL_Texture* numTexture = SDL_CreateTextureFromSurface(renderer, numSurface);
                if (numTexture) {
                    SDL_Rect numRect = { MARGIN, y, numSurface->w, numSurface->h };
                    SDL_RenderCopy(renderer, numTexture, nullptr, &numRect);
                    SDL_DestroyTexture(numTexture);
                }
                SDL_FreeSurface(numSurface);
            }
            // Render text with syntax highlighting.
            int textX = MARGIN + LINE_NUMBER_WIDTH;
            renderLine(renderer, font, state.lines[i + startLine], textX, y, state.darkTheme);
        }

        // Render cursor as a vertical line
        int cursorScreenY = MARGIN + (state.cursorY - startLine) * LINE_HEIGHT;
        int cursorScreenX = MARGIN + LINE_NUMBER_WIDTH + state.cursorX * 8;
        SDL_SetRenderDrawColor(renderer, state.darkTheme ? 255 : 0, 0, 0, 255);
        SDL_Rect cursorRect = { cursorScreenX, cursorScreenY, 2, LINE_HEIGHT };
        SDL_RenderFillRect(renderer, &cursorRect);

        // Render status bar.
        SDL_Rect statusBar = { 0, windowHeight - STATUS_BAR_HEIGHT, windowWidth, STATUS_BAR_HEIGHT };
        if (state.darkTheme)
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        else
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderFillRect(renderer, &statusBar);
        // Render status message.
        SDL_Color statusColor = state.darkTheme ? SDL_Color{230, 230, 230, 255} : SDL_Color{0, 0, 0, 255};
        SDL_Surface* statusSurface = TTF_RenderText_Blended(font, state.statusMessage.c_str(), statusColor);
        if (statusSurface) {
            SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(renderer, statusSurface);
            if (statusTexture) {
                SDL_Rect statusRect = { MARGIN, windowHeight - STATUS_BAR_HEIGHT + (STATUS_BAR_HEIGHT - statusSurface->h) / 2,
                                        statusSurface->w, statusSurface->h };
                SDL_RenderCopy(renderer, statusTexture, nullptr, &statusRect);
                SDL_DestroyTexture(statusTexture);
            }
            SDL_FreeSurface(statusSurface);
        }
        SDL_RenderPresent(renderer);
    }
}


// Event Handling Functions

namespace EventUtils {

    void handleKeyboardInput(SDL_Event &event, EditorState &state) {
        bool ctrl = (SDL_GetModState() & KMOD_CTRL);
        bool shift = (SDL_GetModState() & KMOD_SHIFT);
        switch (event.key.keysym.sym) {
            case SDLK_RETURN:
                TextUtils::insertNewLine(state);
                break;
            case SDLK_BACKSPACE:
                TextUtils::deleteCharacter(state);
                break;
            case SDLK_LEFT:
                if (ctrl)
                    TextUtils::moveCursorToLineStart(state);
                else if (state.cursorX > 0)
                    state.cursorX--;
                break;
            case SDLK_RIGHT:
                if (ctrl)
                    TextUtils::moveCursorToLineEnd(state);
                else if (state.cursorX < state.lines[state.cursorY].size())
                    state.cursorX++;
                break;
            case SDLK_UP:
                if (state.cursorY > 0) {
                    state.cursorY--;
                    state.cursorX = std::min(state.cursorX, (int)state.lines[state.cursorY].size());
                }
                break;
            case SDLK_DOWN:
                if (state.cursorY < state.lines.size() - 1) {
                    state.cursorY++;
                    state.cursorX = std::min(state.cursorX, (int)state.lines[state.cursorY].size());
                }
                break;
            case SDLK_PAGEUP:
                TextUtils::moveCursorPageUp(state, 10);
                break;
            case SDLK_PAGEDOWN:
                TextUtils::moveCursorPageDown(state, 10);
                break;
            case SDLK_z:
                if (ctrl)
                    TextUtils::undo(state);
                break;
            case SDLK_y:
                if (ctrl)
                    TextUtils::redo(state);
                break;
            case SDLK_s:
                if (ctrl)
                    FileUtils::saveFile(state, state.currentFilePath.empty() ? "output.txt" : state.currentFilePath);
                break;
            case SDLK_o:
                if (ctrl) {
                    std::cout << "Enter file name to open: ";
                    std::string filename;
                    std::cin >> filename;
                    FileUtils::openFile(state, filename);
                }
                break;
            case SDLK_n:
                if (ctrl)
                    FileUtils::newFile(state);
                break;
            case SDLK_c:
                if (ctrl)
                    TextUtils::copyText(state);
                break;
            case SDLK_v:
                if (ctrl)
                    TextUtils::pasteText(state);
                break;
            case SDLK_t:
                if (ctrl) {
                    state.darkTheme = !state.darkTheme;
                    state.statusMessage = state.darkTheme ? "Dark theme enabled" : "Light theme enabled";
                }
                break;
            case SDLK_f:
                if (ctrl) {
                    // Activate search mode 
                    state.searchActive = true;
                    std::cout << "Enter search query: ";
                    std::cin >> state.searchQuery;
                    state.statusMessage = "Search activated for: " + state.searchQuery;
                }
                break;
            default:
                break;
        }
    }

    bool processEvents(EditorState &state) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                return false;
            else if (event.type == SDL_KEYDOWN)
                handleKeyboardInput(event, state);
            else if (event.type == SDL_TEXTINPUT)
                TextUtils::insertText(state, event.text.text);
        }
        return true;
    }
}




int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() == -1) {
        std::cerr << "TTF Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDLResources sdl;
    sdl.window = SDL_CreateWindow("Advanced SDL Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);
    if (!sdl.window) {
        std::cerr << "Window creation error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdl.renderer) {
        std::cerr << "Renderer creation error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(sdl.window);
        SDL_Quit();
        return 1;
    }

    sdl.font = TTF_OpenFont("Arial.ttf", 16);
    if (!sdl.font) {
        std::cerr << "Font loading error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(sdl.renderer);
        SDL_DestroyWindow(sdl.window);
        SDL_Quit();
        return 1;
    }

    EditorState editor;
    editor.lines.push_back("");
    editor.statusMessage = "Welcome to Advanced SDL Text Editor";

    SDL_StartTextInput();
    bool running = true;
    while (running) {
        running = EventUtils::processEvents(editor);
        int winW, winH;
        SDL_GetWindowSize(sdl.window, &winW, &winH);
        RenderUtils::renderText(sdl.renderer, sdl.font, editor, winW, winH);
        SDL_Delay(16);  // ~60 FPS
    }
    SDL_StopTextInput();
    return 0;
}
