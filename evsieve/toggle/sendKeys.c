#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <string.h>
#include <sys/time.h>
int fd;
void send_event(int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

// void sync_events(int fd) {
//     send_event(fd, EV_SYN, SYN_REPORT, 0);
// }

// void send_key(int fd, int keycode, int press) {
//     send_event(fd, EV_KEY, keycode, press);
//     sync_events(fd);
// }

// void tap_key(int fd, int keycode) {
//     send_key(fd, keycode, 1);
//     send_key(fd, keycode, 0);
// }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return 1;
    }
    const char *app = argv[1];
    const char *device = "/dev/input/by-id/corsairKeyBoardLogiMouse";
    fd = open(device, O_WRONLY);
    if (fd < 0) {
        return 1;
    }
    for (int i = 0; i < 3; i++) {
        send_event(EV_MSC, MSC_SCAN, 100); 
    }
    if (strcmp(app, "gnome-terminal-server") == 0) {
        send_event(EV_MSC, MSC_SCAN, 101);
    } else if (strcmp(app, "Code") == 0) {
        send_event(EV_MSC, MSC_SCAN, 102);
    } else if (strcmp(app, "google-chrome") == 0) {
        send_event(EV_MSC, MSC_SCAN, 103);
    } else {
        close(fd);
        return 0;
    }
    send_event(EV_SYN, SYN_REPORT, 0);
    close(fd);
    return 0;
}