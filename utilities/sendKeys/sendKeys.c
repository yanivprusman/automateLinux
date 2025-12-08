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

static int get_key_code(char letter) {
    /* Convert letter to uppercase for consistency */
    letter = toupper(letter);
    
    /* Check if it's A-Z */
    if (letter >= 'A' && letter <= 'Z') {
        /* Linux input.h defines KEY_A as 30, KEY_B as 48, etc. 
           Map ASCII A-Z to the correct key codes */
        switch (letter) {
            case 'A': return KEY_A;
            case 'B': return KEY_B;
            case 'C': return KEY_C;
            case 'D': return KEY_D;
            case 'E': return KEY_E;
            case 'F': return KEY_F;
            case 'G': return KEY_G;
            case 'H': return KEY_H;
            case 'I': return KEY_I;
            case 'J': return KEY_J;
            case 'K': return KEY_K;
            case 'L': return KEY_L;
            case 'M': return KEY_M;
            case 'N': return KEY_N;
            case 'O': return KEY_O;
            case 'P': return KEY_P;
            case 'Q': return KEY_Q;
            case 'R': return KEY_R;
            case 'S': return KEY_S;
            case 'T': return KEY_T;
            case 'U': return KEY_U;
            case 'V': return KEY_V;
            case 'W': return KEY_W;
            case 'X': return KEY_X;
            case 'Y': return KEY_Y;
            case 'Z': return KEY_Z;
        }
    }
    return -1;  /* Invalid or unsupported key */
}

void handle_command(const char *cmd) {
    /* Handle letter key commands in the format key<X>, key<X>Up, key<X>Down */
    if (strncmp(cmd, "key", 3) == 0 && strlen(cmd) >= 4) {
        char letter = cmd[3];
        int key_code = get_key_code(letter);
        
        if (key_code != -1) {
            const char *action = cmd + 4;  /* Points to either '\0' or "Up"/"Down" */
            
            if (*action == '\0') {
                /* Simple press and release */
                send_event(EV_KEY, key_code, 1);
                send_event(EV_KEY, key_code, 0);
            } else if (strcmp(action, "Down") == 0) {
                send_event(EV_KEY, key_code, 1);
            } else if (strcmp(action, "Up") == 0) {
                send_event(EV_KEY, key_code, 0);
            }
            return;
        }
    }
    /* Handle raw keycode commands: keycode:30 */
    else if (strncmp(cmd, "keycode:", 8) == 0) {
        int keycode = atoi(cmd + 8);
        if (keycode > 0) {
            send_event(EV_KEY, keycode, 1);
            send_event(EV_KEY, keycode, 0);
            send_event(EV_SYN, SYN_REPORT, 0);
            return;
        }
    }
    /* Handle special characters with Down/Up support */
    else if (strcmp(cmd, "period") == 0 || strcmp(cmd, "dot") == 0) {
        send_key_event(KEY_DOT, 1);
        send_key_event(KEY_DOT, 0);
        return;
    }
    else if (strcmp(cmd, "periodDown") == 0 || strcmp(cmd, "dotDown") == 0) {
        send_event(EV_KEY, KEY_DOT, 1);
        return;
    }
    else if (strcmp(cmd, "periodUp") == 0 || strcmp(cmd, "dotUp") == 0) {
        send_event(EV_KEY, KEY_DOT, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "slash") == 0) {
        send_key_event(KEY_SLASH, 1);
        send_key_event(KEY_SLASH, 0);
        return;
    }
    else if (strcmp(cmd, "slashDown") == 0) {
        send_event(EV_KEY, KEY_SLASH, 1);
        return;
    }
    else if (strcmp(cmd, "slashUp") == 0) {
        send_event(EV_KEY, KEY_SLASH, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "minus") == 0 || strcmp(cmd, "dash") == 0) {
        send_key_event(KEY_MINUS, 1);
        send_key_event(KEY_MINUS, 0);
        return;
    }
    else if (strcmp(cmd, "minusDown") == 0 || strcmp(cmd, "dashDown") == 0) {
        send_event(EV_KEY, KEY_MINUS, 1);
        return;
    }
    else if (strcmp(cmd, "minusUp") == 0 || strcmp(cmd, "dashUp") == 0) {
        send_event(EV_KEY, KEY_MINUS, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "space") == 0) {
        send_key_event(KEY_SPACE, 1);
        send_key_event(KEY_SPACE, 0);
        return;
    }
    else if (strcmp(cmd, "spaceDown") == 0) {
        send_event(EV_KEY, KEY_SPACE, 1);
        return;
    }
    else if (strcmp(cmd, "spaceUp") == 0) {
        send_event(EV_KEY, KEY_SPACE, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "comma") == 0) {
        send_key_event(KEY_COMMA, 1);
        send_key_event(KEY_COMMA, 0);
        return;
    }
    else if (strcmp(cmd, "commaDown") == 0) {
        send_event(EV_KEY, KEY_COMMA, 1);
        return;
    }
    else if (strcmp(cmd, "commaUp") == 0) {
        send_event(EV_KEY, KEY_COMMA, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "equals") == 0 || strcmp(cmd, "equal") == 0) {
        send_key_event(KEY_EQUAL, 1);
        send_key_event(KEY_EQUAL, 0);
        return;
    }
    else if (strcmp(cmd, "equalsDown") == 0 || strcmp(cmd, "equalDown") == 0) {
        send_event(EV_KEY, KEY_EQUAL, 1);
        return;
    }
    else if (strcmp(cmd, "equalsUp") == 0 || strcmp(cmd, "equalUp") == 0) {
        send_event(EV_KEY, KEY_EQUAL, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "semicolon") == 0) {
        send_key_event(KEY_SEMICOLON, 1);
        send_key_event(KEY_SEMICOLON, 0);
        return;
    }
    else if (strcmp(cmd, "semicolonDown") == 0) {
        send_event(EV_KEY, KEY_SEMICOLON, 1);
        return;
    }
    else if (strcmp(cmd, "semicolonUp") == 0) {
        send_event(EV_KEY, KEY_SEMICOLON, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "apostrophe") == 0 || strcmp(cmd, "quote") == 0) {
        send_key_event(KEY_APOSTROPHE, 1);
        send_key_event(KEY_APOSTROPHE, 0);
        return;
    }
    else if (strcmp(cmd, "apostropheDown") == 0 || strcmp(cmd, "quoteDown") == 0) {
        send_event(EV_KEY, KEY_APOSTROPHE, 1);
        return;
    }
    else if (strcmp(cmd, "apostropheUp") == 0 || strcmp(cmd, "quoteUp") == 0) {
        send_event(EV_KEY, KEY_APOSTROPHE, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "backslash") == 0) {
        send_key_event(KEY_BACKSLASH, 1);
        send_key_event(KEY_BACKSLASH, 0);
        return;
    }
    else if (strcmp(cmd, "backslashDown") == 0) {
        send_event(EV_KEY, KEY_BACKSLASH, 1);
        return;
    }
    else if (strcmp(cmd, "backslashUp") == 0) {
        send_event(EV_KEY, KEY_BACKSLASH, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "bracket_left") == 0 || strcmp(cmd, "leftbracket") == 0) {
        send_key_event(KEY_LEFTBRACE, 1);
        send_key_event(KEY_LEFTBRACE, 0);
        return;
    }
    else if (strcmp(cmd, "bracket_leftDown") == 0 || strcmp(cmd, "leftbracketDown") == 0) {
        send_event(EV_KEY, KEY_LEFTBRACE, 1);
        return;
    }
    else if (strcmp(cmd, "bracket_leftUp") == 0 || strcmp(cmd, "leftbracketUp") == 0) {
        send_event(EV_KEY, KEY_LEFTBRACE, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "bracket_right") == 0 || strcmp(cmd, "rightbracket") == 0) {
        send_key_event(KEY_RIGHTBRACE, 1);
        send_key_event(KEY_RIGHTBRACE, 0);
        return;
    }
    else if (strcmp(cmd, "bracket_rightDown") == 0 || strcmp(cmd, "rightbracketDown") == 0) {
        send_event(EV_KEY, KEY_RIGHTBRACE, 1);
        return;
    }
    else if (strcmp(cmd, "bracket_rightUp") == 0 || strcmp(cmd, "rightbracketUp") == 0) {
        send_event(EV_KEY, KEY_RIGHTBRACE, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "backtick") == 0 || strcmp(cmd, "grave") == 0) {
        send_key_event(KEY_GRAVE, 1);
        send_key_event(KEY_GRAVE, 0);
        return;
    }
    else if (strcmp(cmd, "backtickDown") == 0 || strcmp(cmd, "graveDown") == 0) {
        send_event(EV_KEY, KEY_GRAVE, 1);
        return;
    }
    else if (strcmp(cmd, "backtickUp") == 0 || strcmp(cmd, "graveUp") == 0) {
        send_event(EV_KEY, KEY_GRAVE, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "enter") == 0) {
        send_key_event(KEY_ENTER, 1);
        send_key_event(KEY_ENTER, 0);
        return;
    }
    else if (strcmp(cmd, "enterDown") == 0) {
        send_event(EV_KEY, KEY_ENTER, 1);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "enterUp") == 0) {
        send_event(EV_KEY, KEY_ENTER, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "numlock") == 0) {
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
    } 
    else if (strcmp(cmd, "numlockDown") == 0) {
        send_event(EV_MSC, MSC_SCAN, 0x45);
        send_event(EV_KEY, KEY_NUMLOCK, 1);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "numlockUp") == 0) {
        send_event(EV_MSC, MSC_SCAN, 0x45);
        send_event(EV_KEY, KEY_NUMLOCK, 0);
        send_event(EV_SYN, SYN_REPORT, 0);
        return;
    }
    else if (strcmp(cmd, "syn") == 0) {
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