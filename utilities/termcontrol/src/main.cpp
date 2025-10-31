#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "terminal.hpp"

class TermControl {
private:
    termcontrol::Terminal term;

    struct CommandOption {
        std::string description;
        std::function<int(const std::vector<std::string>&)> handler;
        std::string usage;
    };

    std::map<std::string, CommandOption> commands;

    // Helper functions
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

    void showHelp() {
        std::cout << "Usage: termcontrol [COMMAND] [OPTIONS]\n\n"
                  << "Commands:\n";
        
        for (const auto& cmd : commands) {
            std::cout << "  " << cmd.first << "\n"
                     << "    " << cmd.second.description << "\n"
                     << "    Usage: " << cmd.second.usage << "\n\n";
        }
    }

public:
    TermControl() {
        if (!term.init()) {
            throw std::runtime_error("Failed to initialize terminal");
        }

        // Register commands
        commands["--buffer"] = {
            "Show terminal buffer information",
            [this](const auto& args) { return handleBuffer(args); },
            "termcontrol --buffer"
        };

        commands["--char"] = {
            "Get character information at position",
            [this](const auto& args) { return handleChar(args); },
            "termcontrol --char ROW COL"
        };

        commands["--char-range"] = {
            "Get character information for range",
            [this](const auto& args) { return handleCharRange(args); },
            "termcontrol --char-range START_ROW START_COL END_ROW END_COL"
        };

        commands["--set"] = {
            "Set character at position",
            [this](const auto& args) { return handleSet(args); },
            "termcontrol --set ROW COL CHAR [--color FG BG] [--attr ATTRIBUTES]"
        };

        commands["--set-range"] = {
            "Set text in range",
            [this](const auto& args) { return handleSetRange(args); },
            "termcontrol --set-range ROW COL TEXT [--color FG BG] [--attr ATTRIBUTES]"
        };

        commands["--cursor"] = {
            "Control cursor position",
            [this](const auto& args) { return handleCursor(args); },
            "termcontrol --cursor ROW COL | save | restore"
        };

        commands["--raw"] = {
            "Control raw mode",
            [this](const auto& args) { return handleRaw(args); },
            "termcontrol --raw on|off"
        };

        commands["--reset"] = {
            "Reset terminal",
            [this](const auto& args) { return handleReset(args); },
            "termcontrol --reset"
        };
    }

    int run(int argc, char* argv[]) {
        if (argc < 2) {
            showHelp();
            return 1;
        }

        std::string command = argv[1];
        if (command == "-h" || command == "--help") {
            showHelp();
            return 0;
        }

        auto it = commands.find(command);
        if (it == commands.end()) {
            std::cerr << "Unknown command: " << command << "\n";
            showHelp();
            return 1;
        }

        std::vector<std::string> args;
        for (int i = 2; i < argc; i++) {
            args.push_back(argv[i]);
        }

        return it->second.handler(args);
    }

private:
    int handleBuffer(const std::vector<std::string>& args) {
        auto info = term.getBufferInfo();
        std::cout << "Terminal Buffer Information:\n"
                  << "  Rows: " << info.rows << "\n"
                  << "  Columns: " << info.cols << "\n"
                  << "  Cursor Position: " << info.cursor_row << "," << info.cursor_col << "\n"
                  << "  Raw Mode: " << (info.raw_mode ? "Yes" : "No") << "\n";
        return 0;
    }

    int handleChar(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cerr << "Error: --char requires ROW and COL arguments\n";
            return 1;
        }

        int row = std::stoi(args[0]);
        int col = std::stoi(args[1]);
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

    int handleCharRange(const std::vector<std::string>& args) {
        if (args.size() < 4) {
            std::cerr << "Error: --char-range requires START_ROW START_COL END_ROW END_COL arguments\n";
            return 1;
        }

        int r1 = std::stoi(args[0]);
        int c1 = std::stoi(args[1]);
        int r2 = std::stoi(args[2]);
        int c2 = std::stoi(args[3]);
        
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

    int handleSet(const std::vector<std::string>& args) {
        if (args.size() < 3) {
            std::cerr << "Error: --set requires ROW COL CHAR arguments\n";
            return 1;
        }

        int row = std::stoi(args[0]);
        int col = std::stoi(args[1]);
        char ch = args[2][0];

        termcontrol::Terminal::CharacterInfo info = {0};
        info.ch = ch;
        info.fg_color = -1;
        info.bg_color = -1;

        for (size_t i = 3; i < args.size(); i++) {
            std::string opt = args[i];
            if (opt == "--color" && i + 2 < args.size()) {
                info.fg_color = std::stoi(args[++i]);
                info.bg_color = std::stoi(args[++i]);
            } else if (opt == "--attr" && i + 1 < args.size()) {
                auto attrs = splitString(args[++i]);
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

    int handleSetRange(const std::vector<std::string>& args) {
        if (args.size() < 3) {
            std::cerr << "Error: --set-range requires ROW COL TEXT arguments\n";
            return 1;
        }

        int row = std::stoi(args[0]);
        int col = std::stoi(args[1]);
        std::string text = args[2];

        std::vector<termcontrol::Terminal::CharacterInfo> chars;
        termcontrol::Terminal::CharacterInfo info = {0};
        info.fg_color = -1;
        info.bg_color = -1;

        for (size_t i = 3; i < args.size(); i++) {
            std::string opt = args[i];
            if (opt == "--color" && i + 2 < args.size()) {
                info.fg_color = std::stoi(args[++i]);
                info.bg_color = std::stoi(args[++i]);
            } else if (opt == "--attr" && i + 1 < args.size()) {
                auto attrs = splitString(args[++i]);
                for (const auto& attr : attrs) {
                    if (attr == "bold") info.bold = true;
                    else if (attr == "underline") info.underline = true;
                    else if (attr == "reverse") info.reverse = true;
                    else if (attr == "blink") info.blink = true;
                }
            }
        }

        for (char c : text) {
            info.ch = c;
            chars.push_back(info);
        }

        term.setCharacterRange(row, col, row, col + text.length() - 1, chars);
        return 0;
    }

    int handleCursor(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: --cursor requires position or save/restore argument\n";
            return 1;
        }

        if (args[0] == "save") {
            term.saveCursor();
        } else if (args[0] == "restore") {
            term.restoreCursor();
        } else if (args.size() >= 2) {
            int row = std::stoi(args[0]);
            int col = std::stoi(args[1]);
            term.moveCursor(row, col);
        } else {
            std::cerr << "Error: Invalid cursor command\n";
            return 1;
        }
        return 0;
    }

    int handleRaw(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: --raw requires on/off argument\n";
            return 1;
        }

        bool enable = (args[0] == "on");
        return term.setRawMode(enable) ? 0 : 1;
    }

    int handleReset(const std::vector<std::string>& args) {
        term.resetAttributes();
        return 0;
    }
};

int main(int argc, char* argv[]) {
    try {
        TermControl app;
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}