#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "terminal.hpp"
#include "buffer.hpp"
#include <thread>

class TailWindow {
private:
    termcontrol::Terminal term;
    std::vector<std::string> lines;
    size_t window_size;
    std::string filename;
    bool running;
    int start_row;

public:
    TailWindow(const std::string& file, size_t size = 5) 
        : filename(file), window_size(size), running(false) {
        lines.reserve(window_size);
    }

    bool init() {
        if (!term.init()) {
            std::cerr << "Failed to initialize terminal\n";
            return false;
        }

        // Save initial cursor position
        int col;
        term.getCursorPosition(start_row, col);
        
        // Save current terminal attributes
        if (!term.setRawMode(true)) {
            std::cerr << "Failed to set raw mode\n";
            return false;
        }
        
        // Move down one line from current position
        term.moveCursor(start_row + 1, 0);
        
        // Create space for our window
        for (size_t i = 0; i < window_size; i++) {
            std::cout << std::endl;
        }
        
        return true;
    }

    void updateDisplay() {
        // Move to start of window area
        term.moveCursor(start_row + 1, 0);
        
        // Print all lines
        for (size_t i = 0; i < window_size; i++) {
            term.clearLine();
            if (i < lines.size()) {
                std::cerr << "Debug: Printing line " << i << ": " << lines[i] << std::endl;
                std::cout << lines[i] << std::endl;
            } else {
                std::cout << std::endl;
            }
        }
        
        std::cout.flush();
    }

    void addLine(const std::string& line) {
        std::cerr << "Debug: Adding line: " << line << std::endl;
        
        if (lines.size() >= window_size) {
            lines.erase(lines.begin());
        }
        lines.push_back(line);
        
        updateDisplay();
    }

    void run() {
        running = true;
        std::ifstream file(filename);
        
        if (!file) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return;
        }

        std::cerr << "Debug: Starting to monitor file: " << filename << std::endl;
        std::cerr << "Debug: Window size: " << window_size << std::endl;

        // Read initial content of file
        std::string line;
        while (std::getline(file, line)) {
            std::cerr << "Debug: Initial read line: " << line << std::endl;
            addLine(line);
        }

        // Set up input handling
        auto input_handler = [this](char c) {
            if (c == 3 || c == 'q') { // Ctrl+C or 'q'
                std::cerr << "Debug: Received quit signal" << std::endl;
                running = false;
            }
        };
        
        std::thread input_thread([this, &input_handler]() {
            while (running) {
                term.readInput(input_handler);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

        // Get to end of file
        file.seekg(0, std::ios::end);

        // Reset file position to end for monitoring new content
        file.clear();
        file.seekg(0, std::ios::end);

        // Continue monitoring for new content
        while (running) {
            if (file.peek() != EOF) {
                std::getline(file, line);
                if (!line.empty()) {
                    std::cerr << "Debug: New line detected: " << line << std::endl;
                    addLine(line);
                }
            } else {
                file.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Cleanup
        term.resetAttributes();
        term.setRawMode(false);
        
        // Print final newline to ensure terminal prompt appears correctly
        std::cout << std::endl;
        
        if (input_thread.joinable()) {
            input_thread.join();
        }
    }
};

void showHelp() {
    std::cout << "Usage: tailWindow [-h|--help] [-s|--size NUM] FILENAME\n"
              << "Monitor a file with a fixed-size sliding window display.\n\n"
              << "Options:\n"
              << "  -h, --help       Show this help message\n"
              << "  -s, --size NUM   Set window size (number of lines), default is 5\n\n"
              << "Example:\n"
              << "  tailWindow -s 10 /var/log/syslog\n";
}

bool isPositiveInteger(const std::string& str) {
    if (str.empty()) return false;
    return str.find_first_not_of("0123456789") == std::string::npos;
}

int main(int argc, char* argv[]) {
    std::string filename;
    size_t window_size = 5;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            showHelp();
            return 0;
        } else if (arg == "-s" || arg == "--size") {
            if (i + 1 < argc && isPositiveInteger(argv[i + 1])) {
                window_size = std::stoul(argv[i + 1]);
                i++;
            } else {
                std::cerr << "Error: Window size must be a positive integer\n";
                return 1;
            }
        } else if (filename.empty()) {
            filename = arg;
        } else {
            std::cerr << "Error: Only one file can be monitored at a time\n";
            return 1;
        }
    }

    if (filename.empty()) {
        showHelp();
        return 1;
    }

    TailWindow tail(filename, window_size);
    if (!tail.init()) {
        return 1;
    }

    tail.run();
    return 0;
}