#pragma once

#include <string>
#include <memory>
#include <functional>
#include <termios.h>

namespace termcontrol {

class Terminal {
public:
    Terminal();
    ~Terminal();

    // Terminal setup
    bool init();
    void cleanup();
    bool setRawMode(bool enable);

    // Screen manipulation
    void clear();
    void clearLine();
    void refresh();

    // Cursor control
    void moveCursor(int row, int col);
    void saveCursor();
    void restoreCursor();
    void getCursorPosition(int& row, int& col);

    // Input handling
    bool readInput(std::function<void(char)> callback);
    
    // Attributes
    void setForegroundColor(int color);
    void setBackgroundColor(int color);
    void setBold(bool enable);
    void setUnderline(bool enable);
    void resetAttributes();

    // Terminal buffer info
    struct TerminalBufferInfo {
        int rows;
        int cols;
        int cursor_row;
        int cursor_col;
        bool raw_mode;
    };
    
    struct CharacterInfo {
        char ch;
        int fg_color;
        int bg_color;
        bool bold;
        bool underline;
        bool reverse;
        bool blink;
    };

    // New buffer and character info methods
    TerminalBufferInfo getBufferInfo();
    CharacterInfo getCharacterInfo(int row, int col);
    void setCharacterInfo(int row, int col, const CharacterInfo& info);
    std::vector<CharacterInfo> getCharacterRange(int start_row, int start_col, int end_row, int end_col);
    void setCharacterRange(int start_row, int start_col, int end_row, int end_col, const std::vector<CharacterInfo>& chars);

private:
    struct termios original_termios;
    bool is_raw_mode;
    void getTerminalSize(int& rows, int& cols);
    
    // Prevent copying
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
};

} // namespace termcontrol