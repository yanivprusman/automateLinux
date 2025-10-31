#include <iostream>
#include "terminal.hpp"
#include "buffer.hpp"

int main() {
    termcontrol::Terminal term;
    
    // Initialize terminal
    if (!term.init()) {
        std::cerr << "Failed to initialize terminal\n";
        return 1;
    }
    
    // Set raw mode for immediate input
    term.setRawMode(true);
    
    // Clear screen
    term.clear();
    
    // Create a message
    term.moveCursor(0, 0);
    term.setBold(true);
    std::cout << "Terminal Control Demo" << std::endl;
    term.setBold(false);
    
    // Show some colors
    term.moveCursor(2, 0);
    for (int i = 0; i < 8; i++) {
        term.setForegroundColor(i);
        std::cout << "Color " << i << " ";
    }
    term.resetAttributes();
    
    // Get cursor position
    int row, col;
    term.moveCursor(4, 0);
    std::cout << "Press any key to exit...";
    term.getCursorPosition(row, col);
    std::cout << "\nCursor at: " << row << "," << col << std::endl;
    
    // Wait for key
    term.readInput([](char c) { return; });
    
    // Cleanup
    term.clear();
    term.setRawMode(false);
    
    return 0;
}