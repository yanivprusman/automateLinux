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
    printf("  key<X>               Press and release any letter key (A-Z)\n");
    printf("  key<X>Down           Press any letter key (A-Z)\n");
    printf("  key<X>Up             Release any letter key (A-Z)\n");
    printf("  keycode:N            Send raw key code N (e.g., keycode:30 for KEY_A)\n");
    printf("  period/dot[Down/Up]  Send period/dot key\n");
    printf("  slash[Down/Up]       Send forward slash key\n");
    printf("  minus/dash[Down/Up]  Send minus/dash key\n");
    printf("  space[Down/Up]       Send space key\n");
    printf("  comma[Down/Up]       Send comma key\n");
    printf("  equals/equal[Down/Up] Send equals key\n");
    printf("  semicolon[Down/Up]   Send semicolon key\n");
    printf("  apostrophe/quote[Down/Up] Send apostrophe/quote key\n");
    printf("  backslash[Down/Up]   Send backslash key\n");
    printf("  backspace[Down/Up]   Send backspace key\n");
    printf("  bracket_left[Down/Up] Send left bracket key\n");
    printf("  bracket_right[Down/Up] Send right bracket key\n");
    printf("  backtick/grave[Down/Up] Send backtick/grave key\n");
    printf("  enter[Down/Up]       Send enter key with optional Down/Up\n");
    printf("  numlock[Down/Up]     Toggle numlock or send Down/Up separately\n");
    printf("  syn                  Send sync report\n");
    printf("  Code                 Send Code app signal\n");
    printf("  gnome-terminal-server  Send terminal app signal\n");
    printf("  google-chrome        Send Chrome app signal\n");
}

/* Key mapping table - maps key names to their KEY_* codes */
struct KeyMapping {
    const char *name1;      /* Primary name */
    const char *name2;      /* Alternate name (or NULL) */
    int key_code;           /* KEY_* constant */
};

static const struct KeyMapping KEY_MAP[] = {
    /* Letter keys */
    {"keyA", NULL, KEY_A}, {"keyB", NULL, KEY_B}, {"keyC", NULL, KEY_C},
    {"keyD", NULL, KEY_D}, {"keyE", NULL, KEY_E}, {"keyF", NULL, KEY_F},
    {"keyG", NULL, KEY_G}, {"keyH", NULL, KEY_H}, {"keyI", NULL, KEY_I},
    {"keyJ", NULL, KEY_J}, {"keyK", NULL, KEY_K}, {"keyL", NULL, KEY_L},
    {"keyM", NULL, KEY_M}, {"keyN", NULL, KEY_N}, {"keyO", NULL, KEY_O},
    {"keyP", NULL, KEY_P}, {"keyQ", NULL, KEY_Q}, {"keyR", NULL, KEY_R},
    {"keyS", NULL, KEY_S}, {"keyT", NULL, KEY_T}, {"keyU", NULL, KEY_U},
    {"keyV", NULL, KEY_V}, {"keyW", NULL, KEY_W}, {"keyX", NULL, KEY_X},
    {"keyY", NULL, KEY_Y}, {"keyZ", NULL, KEY_Z},
    /* Symbol keys */
    {"period", "dot", KEY_DOT},
    {"slash", NULL, KEY_SLASH},
    {"minus", "dash", KEY_MINUS},
    {"space", NULL, KEY_SPACE},
    {"comma", NULL, KEY_COMMA},
    {"equals", "equal", KEY_EQUAL},
    {"backspace", NULL, KEY_BACKSPACE},
    {"semicolon", NULL, KEY_SEMICOLON},
    {"apostrophe", "quote", KEY_APOSTROPHE},
    {"backslash", NULL, KEY_BACKSLASH},
    {"bracket_left", "leftbracket", KEY_LEFTBRACE},
    {"bracket_right", "rightbracket", KEY_RIGHTBRACE},
    {"backtick", "grave", KEY_GRAVE},
    {"enter", NULL, KEY_ENTER},
};

static const int KEY_MAP_SIZE = sizeof(KEY_MAP) / sizeof(KEY_MAP[0]);

/* Lookup key code from command string, returns -1 if not found */
static int lookup_key(const char *cmd, const char **suffix) {
    for (int i = 0; i < KEY_MAP_SIZE; i++) {
        const char *name1 = KEY_MAP[i].name1;
        const char *name2 = KEY_MAP[i].name2;
        
        /* Try primary name */
        if (strncmp(cmd, name1, strlen(name1)) == 0) {
            *suffix = cmd + strlen(name1);
            return KEY_MAP[i].key_code;
        }
        /* Try alternate name if present */
        if (name2 && strncmp(cmd, name2, strlen(name2)) == 0) {
            *suffix = cmd + strlen(name2);
            return KEY_MAP[i].key_code;
        }
    }
    return -1;
}

void handle_command(const char *cmd) {
    const char *suffix = NULL;
    int key_code = lookup_key(cmd, &suffix);
    
    /* Try table lookup first */
    if (key_code != -1) {
        if (*suffix == '\0') {
            /* Simple press and release */
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
    
    /* Handle raw keycode commands: keycode:30 */
    if (strncmp(cmd, "keycode:", 8) == 0) {
        int keycode = atoi(cmd + 8);
        if (keycode > 0) {
            send_event(EV_KEY, keycode, 1);
            send_event(EV_KEY, keycode, 0);
            send_event(EV_SYN, SYN_REPORT, 0);
        }
        return;
    }
    
    /* Handle numlock (special case with delay) */
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
    
    /* Handle sync */
    if (strcmp(cmd, "syn") == 0) {
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    
    /* Handle app signals */
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