#include "cli.h"
#include <iostream>
#include <cstring>
#include <algorithm>

CLIArgs CLI::parse(int argc, char* argv[]) {
    CLIArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.help = true;
        } else if (arg == "-v" || arg == "--verbose") {
            args.verbose = true;
        } else if (arg == "--version") {
            args.version = true;
        } else if (arg == "--complete") {
            // Generate completions (used by bash completion)
            if (i + 1 < argc) {
                print_completions(argv[++i]);
            } else {
                print_completions();
            }
            exit(0);
        } else if (arg[0] != '-') {
            // First non-option argument is the command
            if (args.command.empty()) {
                args.command = arg;
            } else {
                args.command_args.push_back(arg);
            }
        } else {
            // Additional arguments for the command
            args.command_args.push_back(arg);
        }
    }
    
    return args;
}

void CLI::print_help() {
    std::cout << R"(daemon - customization utility
    USAGE:
        daemon [OPTIONS] <COMMAND> [ARGS]
    COMMANDS:
        ttyOpened           Initialize directory history for current TTY
        update <dir>        Record directory in history
        cdBackward          Navigate to previous directory
        cdForward           Navigate to next directory
        print               Display all history entries and pointers
        reset               Clear all history and reset to default
        status              Show current history state
    OPTIONS:
        -h, --help          Show this help message
        -v, --verbose       Enable verbose output
        --version           Show version information
        --complete [cmd]    Print bash completions
    
    EXAMPLES:
        daemon init
        daemon update /home/user
        daemon cdBackward
        daemon cdForward
        daemon print
        )";
    std::cout << std::endl;
}

void CLI::print_version() {
    std::cout << "daemon version 1.0.0" << std::endl;
}

void CLI::print_completions(const std::string& partial_command) {
    const std::string commands[] = {
        "init",
        "update",
        "cdBackward",
        "cdForward",
        "print",
        "reset",
        "status"
    };
    
    if (partial_command.empty()) {
        // Print all commands
        for (const auto& cmd : commands) {
            std::cout << cmd << std::endl;
        }
    } else {
        // Print matching commands
        for (const auto& cmd : commands) {
            if (cmd.find(partial_command) == 0) {
                std::cout << cmd << std::endl;
            }
        }
    }
}
