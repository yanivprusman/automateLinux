#include <iostream>
#include <string>
#include <vector>
#include "terminal.hpp"

void showHelp() {
    std::cout << "Usage: terminfo [OPTIONS]\n"
              << "Get or set terminal buffer and character information.\n\n"
              << "Options:\n"
              << "  -h, --help                Show this help message\n"
              << "  -b, --buffer              Show terminal buffer information\n"
              << "  -c, --char ROW COL        Get character info at position\n"
              << "  -r, --range R1 C1 R2 C2   Get character info for range\n"
              << "  -s, --set ROW COL CHAR    Set character at position\n"
              << "  --color FG BG             Set colors for --set (0-255)\n"
              << "  --attr ATTRIBUTES         Set attributes for --set (bold,underline,reverse,blink)\n\n"
              << "Example:\n"
              << "  terminfo --buffer\n"
              << "  terminfo --char 0 0\n"
              << "  terminfo --range 0 0 5 10\n"
              << "  terminfo --set 1 1 X --color 1 2 --attr bold,underline\n";
}

std::vector<std::string> splitString(const std::string& str, char delim = ',') {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : str) {
        if (c == delim) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    termcontrol::Terminal term;
    if (!term.init()) {
        std::cerr << "Failed to initialize terminal\n";
        return 1;
    }

    if (argc < 2) {
        showHelp();
        return 1;
    }

    std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
        showHelp();
        return 0;
    }

    if (arg == "-b" || arg == "--buffer") {
        auto info = term.getBufferInfo();
        std::cout << "Terminal Buffer Information:\n"
                  << "  Rows: " << info.rows << "\n"
                  << "  Columns: " << info.cols << "\n"
                  << "  Cursor Position: " << info.cursor_row << "," << info.cursor_col << "\n"
                  << "  Raw Mode: " << (info.raw_mode ? "Yes" : "No") << "\n";
        return 0;
    }

    if (arg == "-c" || arg == "--char") {
        if (argc < 4) {
            std::cerr << "Error: --char requires ROW and COL arguments\n";
            return 1;
        }
        int row = std::stoi(argv[2]);
        int col = std::stoi(argv[3]);
        auto info = term.getCharacterInfo(row, col);
        std::cout << "Character Information at " << row << "," << col << ":\n"
                  << "  Character: '" << info.ch << "'\n"
                  << "  FG Color: " << info.fg_color << "\n"
                  << "  BG Color: " << info.bg_color << "\n"
                  << "  Attributes: "
                  << (info.bold ? "bold " : "")
                  << (info.underline ? "underline " : "")
                  << (info.reverse ? "reverse " : "")
                  << (info.blink ? "blink" : "") << "\n";
        return 0;
    }

    if (arg == "-r" || arg == "--range") {
        if (argc < 6) {
            std::cerr << "Error: --range requires R1 C1 R2 C2 arguments\n";
            return 1;
        }
        int r1 = std::stoi(argv[2]);
        int c1 = std::stoi(argv[3]);
        int r2 = std::stoi(argv[4]);
        int c2 = std::stoi(argv[5]);
        auto chars = term.getCharacterRange(r1, c1, r2, c2);
        std::cout << "Characters in range (" << r1 << "," << c1 << ") to (" << r2 << "," << c2 << "):\n";
        int idx = 0;
        for (int row = r1; row <= r2; row++) {
            std::cout << "  Row " << row << ": ";
            for (int col = (row == r1 ? c1 : 0); col <= (row == r2 ? c2 : 80); col++) {
                if (idx < chars.size()) {
                    std::cout << chars[idx++].ch;
                }
            }
            std::cout << "\n";
        }
        return 0;
    }

    if (arg == "-s" || arg == "--set") {
        if (argc < 5) {
            std::cerr << "Error: --set requires ROW COL CHAR arguments\n";
            return 1;
        }
        int row = std::stoi(argv[2]);
        int col = std::stoi(argv[3]);
        char ch = argv[4][0];

        termcontrol::Terminal::CharacterInfo info = {0};
        info.ch = ch;
        info.fg_color = -1;
        info.bg_color = -1;

        for (int i = 5; i < argc; i++) {
            std::string opt = argv[i];
            if (opt == "--color" && i + 2 < argc) {
                info.fg_color = std::stoi(argv[++i]);
                info.bg_color = std::stoi(argv[++i]);
            } else if (opt == "--attr" && i + 1 < argc) {
                auto attrs = splitString(argv[++i]);
                for (const auto& attr : attrs) {
                    if (attr == "bold") info.bold = true;
                    else if (attr == "underline") info.underline = true;
                    else if (attr == "reverse") info.reverse = true;
                    else if (attr == "blink") info.blink = true;
                }
            }
        }

        term.setCharacterInfo(row, col, info);
        return 0;
    }

    std::cerr << "Error: Unknown option: " << arg << "\n";
    showHelp();
    return 1;
}