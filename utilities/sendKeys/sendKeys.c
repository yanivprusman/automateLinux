#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>

int fd;
const int codeForCode = 101;
const int codeForGnomeTerminal = 102;
const int codeForGoogleChrome = 103;
const char *DEFAULT_KEYBOARD = "/dev/input/by-id/corsairKeyBoardLogiMouse";

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
    printf("  key<X>               Press and release any letter key (A-Z)\n");
    printf("  key<X>Down           Press any letter key (A-Z)\n");
    printf("  key<X>Up             Release any letter key (A-Z)\n");
    printf("  keycode:N            Send raw key code N (e.g., keycode:30 for KEY_A)\n");
    printf("  period[Down/Up]      Send period key\n");
    printf("  slash[Down/Up]       Send forward slash key\n");
    printf("  minus[Down/Up]       Send minus key\n");
    printf("  space[Down/Up]       Send space key\n");
    printf("  comma[Down/Up]       Send comma key\n");
    printf("  equals[Down/Up]      Send equals key\n");
    printf("  semicolon[Down/Up]   Send semicolon key\n");
    printf("  apostrophe[Down/Up]  Send apostrophe key\n");
    printf("  backslash[Down/Up]   Send backslash key\n");
    printf("  backspace[Down/Up]   Send backspace key\n");
    printf("  bracket_left[Down/Up] Send left bracket key\n");
    printf("  bracket_right[Down/Up] Send right bracket key\n");
    printf("  backtick[Down/Up]    Send backtick key\n");
    printf("  enter[Down/Up]       Send enter key with optional Down/Up\n");
    printf("  numlock[Down/Up]     Toggle numlock or send Down/Up separately\n");
    printf("  syn                  Send sync report\n");
    printf("  Code                 Send Code app signal\n");
    printf("  gnome-terminal-server  Send terminal app signal\n");
    printf("  google-chrome        Send Chrome app signal\n");
}

struct KeyMapping {
    const char *name;
    int key_code;
};

static const struct KeyMapping KEY_MAP[] = {
    {"keyA", KEY_A}, {"keyB", KEY_B}, {"keyC", KEY_C},
    {"keyD", KEY_D}, {"keyE", KEY_E}, {"keyF", KEY_F},
    {"keyG", KEY_G}, {"keyH", KEY_H}, {"keyI", KEY_I},
    {"keyJ", KEY_J}, {"keyK", KEY_K}, {"keyL", KEY_L},
    {"keyM", KEY_M}, {"keyN", KEY_N}, {"keyO", KEY_O},
    {"keyP", KEY_P}, {"keyQ", KEY_Q}, {"keyR", KEY_R},
    {"keyS", KEY_S}, {"keyT", KEY_T}, {"keyU", KEY_U},
    {"keyV", KEY_V}, {"keyW", KEY_W}, {"keyX", KEY_X},
    {"keyY", KEY_Y}, {"keyZ", KEY_Z},
    /* Symbol keys */
    {"period", KEY_DOT},
    {"slash", KEY_SLASH},
    {"minus", KEY_MINUS},
    {"space", KEY_SPACE},
    {"comma", KEY_COMMA},
    {"equals", KEY_EQUAL},
    {"backspace", KEY_BACKSPACE},
    {"semicolon", KEY_SEMICOLON},
    {"apostrophe", KEY_APOSTROPHE},
    {"backslash", KEY_BACKSLASH},
    {"bracket_left", KEY_LEFTBRACE},
    {"bracket_right", KEY_RIGHTBRACE},
    {"backtick", KEY_GRAVE},
    {"enter", KEY_ENTER},
};

static const int KEY_MAP_SIZE = sizeof(KEY_MAP) / sizeof(KEY_MAP[0]);

static int lookup_key(const char *cmd, const char **suffix) {
    for (int i = 0; i < KEY_MAP_SIZE; i++) {
        const char *name = KEY_MAP[i].name;
        if (strncmp(cmd, name, strlen(name)) == 0) {
            *suffix = cmd + strlen(name);
            return KEY_MAP[i].key_code;
        }
    }
    return -1;
}

void handle_command(const char *cmd) {
    const char *suffix = NULL;
    int key_code = lookup_key(cmd, &suffix);
    if (key_code != -1) {
        if (*suffix == '\0') {
            send_event(EV_KEY, key_code, 1);
            send_event(EV_KEY, key_code, 0);
        } else if (strcmp(suffix, "Down") == 0) {
            send_event(EV_KEY, key_code, 1);
        } else if (strcmp(suffix, "Up") == 0) {
            send_event(EV_KEY, key_code, 0);
            send_event(EV_SYN, SYN_REPORT, 0);
        }
        return;
    }
    if (strncmp(cmd, "keycode:", 8) == 0) {
        int keycode = atoi(cmd + 8);
        if (keycode > 0) {
            send_event(EV_KEY, keycode, 1);
            send_event(EV_KEY, keycode, 0);
            send_event(EV_SYN, SYN_REPORT, 0);
        }
        return;
    }
    if (strcmp(cmd, "numlock") == 0) {
        send_event(EV_MSC, MSC_SCAN, 0x45);
        send_event(EV_KEY, KEY_NUMLOCK, 1);
        send_event(EV_SYN, SYN_REPORT, 0);
        usleep(50000);
        send_event(EV_MSC, MSC_SCAN, 0x45);
        send_event(EV_KEY, KEY_NUMLOCK, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    if (strcmp(cmd, "numlockDown") == 0) {
        send_event(EV_MSC, MSC_SCAN, 0x45);
        send_event(EV_KEY, KEY_NUMLOCK, 1);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    if (strcmp(cmd, "numlockUp") == 0) {
        send_event(EV_MSC, MSC_SCAN, 0x45);
        send_event(EV_KEY, KEY_NUMLOCK, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    if (strcmp(cmd, "syn") == 0) {
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    int appCode = isApp(cmd);
    if (appCode) {
        for (int i = 0; i < 3; i++) {
            send_event(EV_MSC, MSC_SCAN, 100);
        }
        send_event(EV_MSC, MSC_SCAN, appCode);
        send_event(EV_SYN, SYN_REPORT, 0);
    }
}

int main(int argc, char *argv[]) {
    const char *keyboard_path = get_keyboard_path();
    int i;
    if (argc < 2) {
        print_usage();
        return 1;
    }
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
            break;  
        }
    }
    fd = open(keyboard_path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open keyboard device: %s\n", keyboard_path);
        return 1;
    }
    for (; i < argc; i++) {
        handle_command(argv[i]);
    }
    close(fd);
    return 0;
}