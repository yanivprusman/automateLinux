#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>

struct CLIArgs {
    bool verbose = false;
    bool help = false;
    bool version = false;
    std::string command;
    std::vector<std::string> command_args;
};

class CLI {
public:
    static CLIArgs parse(int argc, char* argv[]);
    static void print_help();
    static void print_version();
    static void print_completions(const std::string& partial_command = "");
};

#endif // CLI_H
