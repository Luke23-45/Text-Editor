# Text Editor

A functional, text editor built using C++ and SDL2, providing basic text editing capabilities with syntax highlighting, undo/redo, copy/paste, and theme switching.

## Getting Started

### Prerequisites
Ensure you have the following installed:
- C++ Compiler (e.g., g++)
- [SDL2](https://www.libsdl.org/)
- [SDL2_ttf](https://www.libsdl.org/projects/SDL_ttf/)

## Project Structure

- `text.cc`: Main C++ source file. (Note: The file name is incorrect, it should be the source file name of the editor.)
- `Makefile`: Build the project.
- `src/`
    - `include/`
    - `lib/`
### Installation
1. Clone the repository:
    ```bash
    git clone git@github.com:Luke23-45/Text-Editor.git
    ```
2. Navigate to the project directory:
    ```bash
    cd Text-Editor
    ```
3. Compile the code:
    ```bash
    make
    ```
4. Run the executable:
    ```bash
    ./main
    ```
5. In window:
    ```bash
    main.exe
    ```
6. To clean up the build artifacts
    ```bash
    make clean
    ```

## Features
- Basic text editing with cursor movement.
- Syntax highlighting for C/C++ keywords.
- Undo/redo functionality.
- Copy/paste operations.
- File open and save capabilities.
- Theme switching (dark/light).
- Search functionality.
- Line numbers.
- Status bar for messages.
- Scrollable text area.

## Key Controls

| Action            | Key Combination | Description                                  |
| ----------------- | --------------- | -------------------------------------------- |
| Exit application  | `SDL_QUIT` event| Close the window.                               |
| Insert newline    | `RETURN`        | Insert a new line with auto-indentation.       |
| Delete character  | `BACKSPACE`     | Delete the character before the cursor.     |
| Move cursor left  | `LEFT`          | Move the cursor one character to the left.   |
| Move cursor right | `RIGHT`         | Move the cursor one character to the right.  |
| Move cursor up    | `UP`            | Move the cursor one line up.                 |
| Move cursor down  | `DOWN`          | Move the cursor one line down.               |
| Move page up      | `PAGEUP`        | Move the cursor up by a page.                |
| Move page down    | `PAGEDOWN`      | Move the cursor down by a page.              |
| Undo              | `CTRL + Z`      | Undo the last action.                          |
| Redo              | `CTRL + Y`      | Redo the last undone action.                   |
| Save file         | `CTRL + S`      | Save the current file.                       |
| Open file         | `CTRL + O`      | Open a file.                                  |
| New file          | `CTRL + N`      | Create a new file.                           |
| Copy text         | `CTRL + C`      | Copy selected text to the clipboard.         |
| Paste text        | `CTRL + V`      | Paste text from the clipboard.               |
| Toggle theme      | `CTRL + T`      | Switch between dark and light themes.       |
| Search            | `CTRL + F`      | Activate search mode and enter search query. |
| Move to line start| `CTRL + LEFT`   | Move cursor to the start of the current line.|
| Move to line end  | `CTRL + RIGHT`  | Move cursor to the end of the current line.  |

## Code Structure
- `EditorState`: Struct that holds the editor's state, including text lines, cursor position, file path, and other editor-related data.
- `SDLResources`: Struct that manages SDL resources like the window, renderer, and font, ensuring proper cleanup.
- `TextUtils`: Namespace containing functions for text manipulation (inserting, deleting, undoing, redoing, copying, pasting, and cursor movement).
- `FileUtils`: Namespace providing functions for file operations (new, open, save).
- `RenderUtils`: Namespace for rendering functions, including syntax highlighting, cursor rendering, and status bar display.
- `EventUtils`: Namespace for handling SDL events, including keyboard input.
- `main`: Initializes SDL, creates the window and renderer, loads the font, and runs the main event loop.

## Demo Video
Check out the project demo video on YouTube: https://www.youtube.com/watch?v=McOcbGHyAWA
## License

This project is licensed under the MIT License. Feel free to use, modify, and distribute the code.

## Acknowledgements

- SDL2 for graphics rendering.
