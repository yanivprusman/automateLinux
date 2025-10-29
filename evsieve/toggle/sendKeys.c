#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <string.h>
#include <sys/time.h>
int fd;
const int codeForCode = 101;
const int codeForGnomeTerminal = 102;
const int codeForGoogleChrome = 103;

void send_event(int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

int isApp(const char *input){
    if (strcmp(input, "Code") == 0) return codeForCode;
    if (strcmp(input, "gnome-terminal-server") == 0) return codeForGnomeTerminal;
    if (strcmp(input, "google-chrome") == 0) return codeForGoogleChrome;
    return 0;
}
// int getCodeFromInput(const char *app) {
//     if (strcmp(app, "Code") == 0) return 102;
//     if (strcmp(app, "gnome-terminal-server") == 0) return 103;
//     if (strcmp(app, "google-chrome") == 0) return 104;
//     if (strcmp(app, "google-chrome") == 0) return 104;
//     return 101;
// }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return 1;
    }
    const char *input = argv[1];
    // const char *device = "/dev/input/by-id/corsairKeyBoardLogiMouse";
    const char *device = "/dev/input/by-id/usb-Corsair_CORSAIR_K100_RGB_Optical-Mechanical_Gaming_Keyboard_502A81D24AAA7CC6-event-kbd";
    fd = open(device, O_WRONLY);
    if (fd < 0) {
        return 1;
    }
    int appCode = isApp(input);
    if (appCode){
        for (int i = 0; i < 3; i++) { send_event(EV_MSC, MSC_SCAN, 100); }
        send_event(EV_MSC, MSC_SCAN, appCode);
        send_event(EV_SYN, SYN_REPORT, 0);
    } else if (strcmp(input, "SYN_REPORT") == 0){
        send_event(EV_SYN, SYN_REPORT, 0);
    } else if (strcmp(input, "CodeADown") == 0){
        send_event(EV_KEY, KEY_A, 1);
    } else if (strcmp(input, "CodeAUP") == 0){
        send_event(EV_KEY, KEY_A, 0);
    } else if (strcmp(input, "CodeADownUp") == 0){
        send_event(EV_KEY, KEY_A, 1);
        send_event(EV_KEY, KEY_A, 0);
    }
    close(fd);
    return 0;
}