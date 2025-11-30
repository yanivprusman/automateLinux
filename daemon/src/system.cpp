#include "system.h"

string System::getTty() {
    FILE* tty_pipe = popen("tty", "r");
    string tty;
    if (tty_pipe) {
        char buffer[256] = {0};
        if (fgets(buffer, sizeof(buffer), tty_pipe) != nullptr) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            tty = buffer;
        }
        pclose(tty_pipe);
    }
    return tty;
}

string System::executeBashCommand(const char* cmd) {
    string result;
    string fullCmd = "/bin/bash -c \"" + string(cmd) + "\"";
    FILE* pipe = popen(fullCmd.c_str(), "r");
    
    if (!pipe) {
        perror("popen failed");
        return result;
    }
    
    char buffer[256] = {0};
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}
