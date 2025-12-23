#ifndef SENDKEYS_H
#define SENDKEYS_H

#ifdef __cplusplus
extern "C" {
#endif

// Original function - opens/closes keyboard each call
// Used by standalone sendKeys utility
int sendKeys_execute_commands(const char* keyboard_path, int num_commands, char* commands[]);

// Optimized function - uses pre-opened file descriptor
// Used by daemon for maximum performance
void sendKeys_with_fd(int fd, int num_commands, char* commands[]);

#ifdef __cplusplus
}
#endif

#endif // SENDKEYS_H
