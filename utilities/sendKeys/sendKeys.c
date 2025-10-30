#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <string.h>
#include <sys/time.h>

int fd;
const int codeForCode = 101;
const int codeForGnomeTerminal = 102;
const int codeForGoogleChrome = 103;

// Default keyboard device path
const char *DEFAULT_KEYBOARD = "/dev/input/by-id/corsairKeyBoardLogiMouse";

// Get keyboard path from environment variable or use default
const char* get_keyboard_path() {
    static char full_path[256];
    const char* env_path = getenv("KEYBOARD_BY_ID");
    
    if (env_path != NULL && *env_path != '\0') {
        snprintf(full_path, sizeof(full_path), "/dev/input/by-id/%s", env_path);
        return full_path;
    }
    return DEFAULT_KEYBOARD;
}

void send_event(int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

void send_key_event(int key, int value) {
    send_event(EV_KEY, key, value);
    send_event(EV_SYN, SYN_REPORT, 0);
}

int isApp(const char *input) {
    if (strcmp(input, "Code") == 0) return codeForCode;
    if (strcmp(input, "gnome-terminal-server") == 0) return codeForGnomeTerminal;
    if (strcmp(input, "google-chrome") == 0) return codeForGoogleChrome;
    return 0;
}

void print_usage() {
    printf("Usage: sendKeys [OPTIONS] COMMANDS...\n");
    printf("Options:\n");
    printf("  -k, --keyboard PATH   Specify keyboard device path\n");
    printf("Commands:\n");
    printf("  keyADown             Press key A\n");
    printf("  keyAUp               Release key A\n");
    printf("  keyA                 Press and release key A\n");
    printf("  numlock              Toggle numlock\n");
    printf("  Code                 Send Code app signal\n");
    printf("  gnome-terminal-server  Send terminal app signal\n");
    printf("  google-chrome        Send Chrome app signal\n");
    printf("  syn                  Send sync report\n");
}

void handle_command(const char *cmd) {
    if (strcmp(cmd, "keyADown") == 0) {
        send_event(EV_KEY, KEY_A, 1);
    } else if (strcmp(cmd, "keyAUp") == 0) {
        send_event(EV_KEY, KEY_A, 0);
    } else if (strcmp(cmd, "keyA") == 0) {
        send_event(EV_KEY, KEY_A, 1);
        send_event(EV_KEY, KEY_A, 0);
    } else if (strcmp(cmd, "numlock") == 0) {
        // Send scan code first (0x45 is the standard scan code for numlock)
        send_event(EV_MSC, MSC_SCAN, 0x45);
        // Send key press
        send_event(EV_KEY, KEY_NUMLOCK, 1);
        // Send sync after press
        send_event(EV_SYN, SYN_REPORT, 0);
        // Send release after a small delay
        usleep(50000); // 50ms delay
        // Send scan code again for release
        send_event(EV_MSC, MSC_SCAN, 0x45);
        // Send key release
        send_event(EV_KEY, KEY_NUMLOCK, 0);
        // Final sync
        send_event(EV_SYN, SYN_REPORT, 0);
    } else if (strcmp(cmd, "syn") == 0) {
        send_event(EV_SYN, SYN_REPORT, 0);
    } else {
        int appCode = isApp(cmd);
        if (appCode) {
            for (int i = 0; i < 3; i++) {
                send_event(EV_MSC, MSC_SCAN, 100);
            }
            send_event(EV_MSC, MSC_SCAN, appCode);
            send_event(EV_SYN, SYN_REPORT, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    const char *keyboard_path = get_keyboard_path();
    int i;

    if (argc < 2) {
        print_usage();
        return 1;
    }

    // Parse options
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keyboard") == 0) {
            if (i + 1 < argc) {
                keyboard_path = argv[++i];
            } else {
                fprintf(stderr, "Error: --keyboard requires a path argument\n");
                return 1;
            }
        } else {
            break;  // Start of commands
        }
    }

    // Open keyboard device
    fd = open(keyboard_path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open keyboard device: %s\n", keyboard_path);
        return 1;
    }

    // Process all commands
    for (; i < argc; i++) {
        handle_command(argv[i]);
    }

    close(fd);
    return 0;
}