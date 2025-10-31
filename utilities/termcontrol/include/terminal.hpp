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

private:
    struct termios original_termios;
    bool is_raw_mode;
    
    // Prevent copying
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
};

} // namespace termcontrol