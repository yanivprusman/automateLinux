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

} // namespace termcontrol