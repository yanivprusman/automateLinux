#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>
static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    keep_running = 0;
}

static void write_pidfile(const char *pidfile)
{
    FILE *f = fopen(pidfile, "w");
    if (!f) return;
    fprintf(f, "%d\n", (int)getpid());
    fclose(f);
}

static void remove_pidfile(const char *pidfile)
{
    if (pidfile)
        unlink(pidfile);
}

static void daemonize_and_redirect(const char *logfile)
{
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) {
        /* parent exits */
        exit(0);
    }
    if (setsid() < 0) exit(1);
    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    umask(0);
    chdir("/");

    /* redirect std fds to logfile if provided, otherwise to /dev/null */
    int fdout = open(logfile ? logfile : "/dev/null", O_WRONLY | O_CREAT | O_APPEND, 0640);
    if (fdout >= 0) {
        dup2(fdout, STDOUT_FILENO);
        dup2(fdout, STDERR_FILENO);
        if (fdout > STDERR_FILENO) close(fdout);
    }
}

int main(int argc, char **argv)
{
    int do_daemon = 0;
    const char *pidfile = "/tmp/sendKeysUInput.pid";
    const char *logfile = "/tmp/sendKeysUInput.log";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
            do_daemon = 1;
        } else if (strcmp(argv[i], "--no-log") == 0) {
            logfile = NULL;
        }
    }

    if (do_daemon) {
        daemonize_and_redirect(logfile);
        write_pidfile(pidfile);
    }

    /* install signal handlers */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        if (!do_daemon)
            perror("open /dev/uinput");
        else
            syslog(LOG_ERR, "open /dev/uinput: %s", strerror(errno));
        remove_pidfile(pidfile);
        return 1;
    }

    /* enable key events */
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
        if (!do_daemon)
            perror("ioctl UI_SET_EVBIT EV_KEY");
        else
            syslog(LOG_ERR, "ioctl UI_SET_EVBIT EV_KEY: %s", strerror(errno));
        close(fd);
        remove_pidfile(pidfile);
        return 1;
    }

    for (int k = 0; k < 256; k++) {
        /* ignore individual keybit failures */
        ioctl(fd, UI_SET_KEYBIT, k);
    }

    /* optional capabilities */
    ioctl(fd, UI_SET_EVBIT, EV_REP);
    ioctl(fd, UI_SET_EVBIT, EV_LED);
    ioctl(fd, UI_SET_LEDBIT, LED_NUML);

    struct uinput_user_dev udev;
    memset(&udev, 0, sizeof(udev));
    snprintf(udev.name, UINPUT_MAX_NAME_SIZE, "Virtual Corsair Keyboard");
    udev.id.bustype = BUS_USB;
    udev.id.vendor  = 0x1234;
    udev.id.product = 0x5678;
    udev.id.version = 1;

    if (write(fd, &udev, sizeof(udev)) < 0) {
        if (!do_daemon)
            perror("write udev");
        else
            syslog(LOG_ERR, "write udev: %s", strerror(errno));
        close(fd);
        remove_pidfile(pidfile);
        return 1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        if (!do_daemon)
            perror("ioctl UI_DEV_CREATE");
        else
            syslog(LOG_ERR, "ioctl UI_DEV_CREATE: %s", strerror(errno));
        close(fd);
        remove_pidfile(pidfile);
        return 1;
    }

    if (!do_daemon) {
        printf("uinput device created (fd=%d)\n", fd);
    } else {
        syslog(LOG_INFO, "uinput device created (fd=%d)", fd);
    }

    /* run until signaled */
    while (keep_running) {
        sleep(1);
    }

    if (ioctl(fd, UI_DEV_DESTROY) < 0) {
        if (!do_daemon)
            perror("ioctl UI_DEV_DESTROY");
        else
            syslog(LOG_ERR, "ioctl UI_DEV_DESTROY: %s", strerror(errno));
    }

    close(fd);
    remove_pidfile(pidfile);
    return 0;
}
