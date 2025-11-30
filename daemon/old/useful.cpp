#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>

// Helper function to execute bash command and return output
std::string execute_bash_command(const char* cmd) {
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        std::perror("popen failed");
        return result;
    }
    char buffer[256] = {0};
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

// Helper function to get current TTY
std::string get_tty() {
    FILE* tty_pipe = popen("tty", "r");
    std::string tty_opened;
    if (tty_pipe) {
        char buffer[256] = {0};
        if (fgets(buffer, sizeof(buffer), tty_pipe) != nullptr) {
            // Remove trailing newline
            size_t len = std::strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            tty_opened = buffer;
        }
        pclose(tty_pipe);
    }
    return tty_opened;
}

int usefull(int argc, char* argv[]) {
    bool verbose = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-ttyOpened") == 0) {
            verbose = true;
        }
    }
    
    // Get TTY
    std::string tty_opened = get_tty();
    
    if (verbose) {
        std::cout << "TTY: " << tty_opened << std::endl;
    }
    
    return 0;
}

/*
// Alternative: Execute bash command and return output
// Example of how to use execute_bash_command:

const char* cmd = R"(
    /bin/bash <<'EOF'
    . ~/.bashrc
    #echo "Hello World"
    #ls --color=always
    #theRealPath ../
    #echo "${AUTOMATE_LINUX_DATA_DIR}dirHistory/"
EOF
)";

// std::string output = execute_bash_command(cmd);
// std::cout << output;
*/