#pragma once

#include <string>
#include <memory>
#include <functional>
#include <termios.h>
#include "buffer.hpp"

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
        // Visible window size
        int rows;
        int cols;
        // Full buffer (including scrollback)
        int buffer_rows;
        int buffer_cols;
        // Cursor position within visible window
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
    // internal buffer that tracks full scrollback/content written through this API
    std::unique_ptr<Buffer> buffer;
    int scrollback_lines = 1000; // reasonable default
    
    // Prevent copying
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
};

} // namespace termcontrol