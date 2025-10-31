#include "terminal.hpp"
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>

namespace termcontrol {

Terminal::Terminal() : is_raw_mode(false) {}

Terminal::~Terminal() {
    cleanup();
}

bool Terminal::init() {
    if (tcgetattr(STDIN_FILENO, &original_termios) < 0) {
        return false;
    }
        // initialize internal buffer with current terminal size + scrollback
        int rows, cols;
        getTerminalSize(rows, cols);
        // buffer rows include visible rows plus scrollback
        buffer.reset(new Buffer(rows + scrollback_lines, cols));
        return true;
}

void Terminal::cleanup() {
    if (is_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    }
}

bool Terminal::setRawMode(bool enable) {
    if (enable == is_raw_mode) return true;

    if (enable) {
        struct termios raw = original_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
            return false;
        }
    } else {
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) < 0) {
            return false;
        }
    }

    is_raw_mode = enable;
    return true;
}

void Terminal::clear() {
    printf("\033[2J");
    printf("\033[H");
    fflush(stdout);
}

void Terminal::clearLine() {
    printf("\033[2K");
    printf("\r");
    fflush(stdout);
}

void Terminal::moveCursor(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
    fflush(stdout);
}

void Terminal::saveCursor() {
    printf("\033[s");
    fflush(stdout);
}

void Terminal::restoreCursor() {
    printf("\033[u");
    fflush(stdout);
}

void Terminal::getCursorPosition(int& row, int& col) {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    
    setRawMode(true);
    
    printf("\033[6n");
    fflush(stdout);
    
    char buf[32];
    int i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    
    if (sscanf(buf, "\033[%d;%dR", &row, &col) != 2) {
        row = col = -1;
    } else {
        row--; col--;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

bool Terminal::readInput(std::function<void(char)> callback) {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        callback(c);
    }
    return true;
}

void Terminal::setForegroundColor(int color) {
    printf("\033[%dm", 30 + color);
    fflush(stdout);
}

void Terminal::setBackgroundColor(int color) {
    printf("\033[%dm", 40 + color);
    fflush(stdout);
}

void Terminal::setBold(bool enable) {
    printf("\033[%dm", enable ? 1 : 22);
    fflush(stdout);
}

void Terminal::setUnderline(bool enable) {
    printf("\033[%dm", enable ? 4 : 24);
    fflush(stdout);
}

void Terminal::resetAttributes() {
    printf("\033[0m");
    fflush(stdout);
}

void Terminal::getTerminalSize(int& rows, int& cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        rows = 24;  // Default fallback
        cols = 80;
    } else {
        rows = ws.ws_row;
        cols = ws.ws_col;
    }
}

Terminal::TerminalBufferInfo Terminal::getBufferInfo() {
    TerminalBufferInfo info;
    
    // Save current state
    saveCursor();
    setRawMode(true);
    
    // Get visible size
    getTerminalSize(info.rows, info.cols);
    
    // Get current position
    getCursorPosition(info.cursor_row, info.cursor_col);
    
    // Move to top-left and query position
    printf("\033[H");
    fflush(stdout);
    
    // Move way up to find the scrollback limit
    printf("\033[9999A\033[6n");
    fflush(stdout);
    
    // Read response
    char buf[32];
    int i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    
    // Parse actual buffer size
    int top_row, tmp_col;
    if (sscanf(buf, "\033[%d;%dR", &top_row, &tmp_col) == 2) {
        info.buffer_rows = abs(top_row); // abs() because some terminals report negative
        info.buffer_cols = info.cols;     // buffer width matches visible width
    } else {
        // Fallback to visible size if query fails
        info.buffer_rows = info.rows;
        info.buffer_cols = info.cols;
    }
    
    // Restore state
    restoreCursor();
    info.raw_mode = is_raw_mode;
    
    return info;
}

Terminal::CharacterInfo Terminal::getCharacterInfo(int row, int col) {
    CharacterInfo info = {0};
    
    // Save current position and attributes
    saveCursor();
    auto orig_attrs = "\033[0m";
    
    // Move to target position
    moveCursor(row, col);
    
    // Query character using ANSI escape sequence
    printf("\033[6n");  // Get cursor position
    fflush(stdout);
    
    // Read response
    char buf[32];
    int i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    
    // Parse character attributes
    // This requires terminal support for SGR parameters
    printf("\033[0m\033[?25l");  // Reset attributes and hide cursor
    fflush(stdout);
    
    printf("\033]10;?\007");  // Query foreground color
    printf("\033]11;?\007");  // Query background color
    fflush(stdout);
    
    // Read character at position
    info.ch = buf[0];  // Simplified - in reality need to handle escape sequences
    
    // Restore position and attributes
    printf("%s", orig_attrs);
    restoreCursor();
    fflush(stdout);
    
    return info;
}

void Terminal::setCharacterInfo(int row, int col, const CharacterInfo& info) {
    saveCursor();
    moveCursor(row, col);
    
    // Set attributes
    if (info.bold) printf("\033[1m");
    if (info.underline) printf("\033[4m");
    if (info.reverse) printf("\033[7m");
    if (info.blink) printf("\033[5m");
    
    // Set colors
    if (info.fg_color >= 0) printf("\033[38;5;%dm", info.fg_color);
    if (info.bg_color >= 0) printf("\033[48;5;%dm", info.bg_color);
    
    // Write character
    printf("%c", info.ch);
    
        // update internal buffer if present (map visible position to buffer's bottom area)
        if (buffer) {
            int br = buffer->getRows();
            int bc = buffer->getCols();
            // place visible rows at the bottom of the buffer
            int vis_rows, vis_cols;
            getTerminalSize(vis_rows, vis_cols);
            int base_row = br - vis_rows; // top row index in buffer corresponding to visible row 0
            int buf_row = base_row + row;
            if (buf_row >= 0 && buf_row < br && col >= 0 && col < bc) {
                // buffer stores chars only; reflect character
                buffer->write(std::string(1, info.ch), buf_row, col);
            }
        }
    
    resetAttributes();
    restoreCursor();
    fflush(stdout);
}

std::vector<Terminal::CharacterInfo> Terminal::getCharacterRange(
    int start_row, int start_col, int end_row, int end_col) {
    
    std::vector<CharacterInfo> chars;
        // If we have an internal buffer, read from it for accurate and fast results
        if (buffer) {
            for (int row = start_row; row <= end_row; row++) {
                for (int col = (row == start_row ? start_col : 0);
                     col <= (row == end_row ? end_col : buffer->getCols() - 1); col++) {
                    CharacterInfo ci = {0};
                    ci.ch = buffer->at(row, col);
                    ci.fg_color = -1;
                    ci.bg_color = -1;
                    chars.push_back(ci);
                }
            }
            return chars;
        }

        for (int row = start_row; row <= end_row; row++) {
            for (int col = (row == start_row ? start_col : 0); 
                 col <= (row == end_row ? end_col : 80); col++) {
                chars.push_back(getCharacterInfo(row, col));
            }
        }
    return chars;
}

void Terminal::setCharacterRange(
    int start_row, int start_col, int end_row, int end_col,
    const std::vector<CharacterInfo>& chars) {
    
    size_t idx = 0;
    for (int row = start_row; row <= end_row; row++) {
        for (int col = (row == start_row ? start_col : 0);
             col <= (row == end_row ? end_col : 80); col++) {
            if (idx < chars.size()) {
                setCharacterInfo(row, col, chars[idx++]);
            }
        }
    }
}

} // namespace termcontrol