#include <ctype.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

const int codeForDefault = 101;
const int codeForCode = 102;
const int codeForGnomeTerminal = 103;
const int codeForGoogleChrome = 104;
const char *DEFAULT_KEYBOARD = "/dev/input/by-id/corsairKeyBoardLogiMouse";
const char *DAEMON_SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock";
// static char keyboard_path_buffer[512]; // Not needed in daemon mode for
// sendKeys_execute_commands

// sendEvent now takes fd as an argument
void sendEvent(int fd_local, int type, int code, int value) {
  struct input_event ev;
  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = type;
  ev.code = code;
  ev.value = value;
  if (fd_local >= 0) {
    write(fd_local, &ev, sizeof(ev));
  } else {
    fprintf(stderr, "Error: Invalid file descriptor for sendEvent\n");
  }
}

int isApp(const char *input) {
  if (strcmp(input, "code") == 0)
    return codeForCode;
  if (strcmp(input, "gnome-terminal-server") == 0)
    return codeForGnomeTerminal;
  if (strcmp(input, "google-chrome") == 0)
    return codeForGoogleChrome;
  if (strcmp(input, "DefaultKeyboard") == 0)
    return codeForDefault;
  return 0;
}

struct KeyMapping {
  const char *name;
  int key_code;
};

static const struct KeyMapping KEY_MAP[] = {
    {"keyA", KEY_A},
    {"keyB", KEY_B},
    {"keyC", KEY_C},
    {"keyD", KEY_D},
    {"keyE", KEY_E},
    {"keyF", KEY_F},
    {"keyG", KEY_G},
    {"keyH", KEY_H},
    {"keyI", KEY_I},
    {"keyJ", KEY_J},
    {"keyK", KEY_K},
    {"keyL", KEY_L},
    {"keyM", KEY_M},
    {"keyN", KEY_N},
    {"keyO", KEY_O},
    {"keyP", KEY_P},
    {"keyQ", KEY_Q},
    {"keyR", KEY_R},
    {"keyS", KEY_S},
    {"keyT", KEY_T},
    {"keyU", KEY_U},
    {"keyV", KEY_V},
    {"keyW", KEY_W},
    {"keyX", KEY_X},
    {"keyY", KEY_Y},
    {"keyZ", KEY_Z},
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
    {"keyShift", KEY_LEFTSHIFT}, // Added for daemon integration
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

// handle_command now takes fd as an argument
void handle_command(int fd_local, const char *cmd) {
  const char *suffix = NULL;
  int key_code = lookup_key(cmd, &suffix);
  if (key_code != -1) {
    if (*suffix == '\0') {
      sendEvent(fd_local, EV_KEY, key_code, 1);
      sendEvent(fd_local, EV_KEY, key_code, 0);
      sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    } else if (strcmp(suffix, "Down") == 0) {
      sendEvent(fd_local, EV_KEY, key_code, 1);
      sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    } else if (strcmp(suffix, "Up") == 0) {
      sendEvent(fd_local, EV_KEY, key_code, 0);
      sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    }
    return;
  }
  if (strncmp(cmd, "keycode:", 8) == 0) {
    int keycode = atoi(cmd + 8);
    if (keycode > 0) {
      sendEvent(fd_local, EV_KEY, keycode, 1);
      sendEvent(fd_local, EV_KEY, keycode, 0);
      sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    }
    return;
  }
  if (strcmp(cmd, "numlock") == 0) {
    sendEvent(fd_local, EV_MSC, MSC_SCAN, 0x45);
    sendEvent(fd_local, EV_KEY, KEY_NUMLOCK, 1);
    sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    usleep(50000);
    sendEvent(fd_local, EV_MSC, MSC_SCAN, 0x45);
    sendEvent(fd_local, EV_KEY, KEY_NUMLOCK, 0);
    sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    return;
  }
  if (strcmp(cmd, "numlockDown") == 0) {
    sendEvent(fd_local, EV_MSC, MSC_SCAN, 0x45);
    sendEvent(fd_local, EV_KEY, KEY_NUMLOCK, 1);
    sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    return;
  }
  if (strcmp(cmd, "numlockUp") == 0) {
    sendEvent(fd_local, EV_MSC, MSC_SCAN, 0x45);
    sendEvent(fd_local, EV_KEY, KEY_NUMLOCK, 0);
    sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    return;
  }
  if (strcmp(cmd, "syn") == 0) {
    sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
    return;
  }
  int appCode = isApp(cmd);
  if (appCode) {
    for (int i = 0; i < 3; i++) {
      sendEvent(fd_local, EV_MSC, MSC_SCAN, 100);
    }
    sendEvent(fd_local, EV_MSC, MSC_SCAN, appCode);
    sendEvent(fd_local, EV_SYN, SYN_REPORT, 0);
  }
}

// Public function for daemon integration
// This function takes the keyboard path and an array of command strings.
// It opens the keyboard device, processes commands, and closes the device.
int sendKeys_execute_commands(const char *keyboard_path, int num_commands,
                              char *commands[]) {
  int fd_local = open(keyboard_path, O_WRONLY);
  if (fd_local < 0) {
    fprintf(stderr, "Error: Could not open keyboard device: %s\n",
            keyboard_path);
    return 1;
  }

  for (int i = 0; i < num_commands; i++) {
    handle_command(fd_local, commands[i]);
  }

  close(fd_local);
  return 0;
}

// Optimized function for daemon - uses pre-opened file descriptor
// This avoids the overhead of opening/closing the keyboard device on every call
void sendKeys_with_fd(int fd_local, int num_commands, char *commands[]) {
  if (fd_local < 0) {
    fprintf(stderr, "Error: Invalid file descriptor for sendKeys_with_fd\n");
    return;
  }

  for (int i = 0; i < num_commands; i++) {
    handle_command(fd_local, commands[i]);
  }
}

#ifndef DAEMON_MODE
// Global fd for standalone mode
static int fd;
static char
    keyboard_path_buffer[512]; // Used only in standalone getKeyboardPath

const char *getKeyboardPath() {
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return DEFAULT_KEYBOARD;
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, DAEMON_SOCKET_PATH, sizeof(addr.sun_path) - 1);
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sock);
    return DEFAULT_KEYBOARD;
  }
  const char *command = "{\"command\":\"getKeyboardPath\"}\n";
  if (write(sock, command, strlen(command)) < 0) {
    close(sock);
    return DEFAULT_KEYBOARD;
  }
  char buffer[512];
  ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
  close(sock);
  if (bytes_read <= 0)
    return DEFAULT_KEYBOARD;
  buffer[bytes_read] = '\0';
  if (buffer[bytes_read - 1] == '\n')
    buffer[bytes_read - 1] = '\0';
  strncpy(keyboard_path_buffer, buffer, sizeof(keyboard_path_buffer) - 1);
  keyboard_path_buffer[sizeof(keyboard_path_buffer) - 1] = '\0';
  return keyboard_path_buffer;
}

// Standalone main function
int main(int argc, char *argv[]) {
  const char *keyboard_path = getKeyboardPath();
  int i;
  if (argc < 2) {
    print_usage();
    return 1;
  }
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage();
      return 0;
    } else if (strcmp(argv[i], "-k") == 0 ||
               strcmp(argv[i], "--keyboard") == 0) {
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
  // Call the new shared function
  int num_commands = argc - i;
  char **commands = &argv[i];

  return sendKeys_execute_commands(keyboard_path, num_commands, commands);
}
#endif // DAEMON_MODE