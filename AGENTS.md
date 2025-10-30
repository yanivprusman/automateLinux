yaniv@ubuntu:~/coding/automateLinux/evsieve/led$ 1stop 
Error resolving path: No such file or directory

alias 1stop='stopCorsairKeyBoardLogiMouseService.sh'

yaniv@ubuntu:~/coding/automateLinux/evsieve/led$ ls -l ../../symlinks/
total 16
lrwxrwxrwx 1 yaniv yaniv 51 Oct 30 16:45 led -> /home/yaniv/coding/automateLinux/evsieve/led/led.sh
lrwxrwxrwx 1 yaniv yaniv 60 Oct 29 16:38 restartCorsairKeyBoardLogiMouseService.sh -> /home/yaniv/coding/automateLinux/evsieve/services/restart.sh
lrwxrwxrwx 1 yaniv yaniv 60 Oct 30 16:45 sendKeys -> /home/yaniv/coding/automateLinux/utilities/sendKeys/sendKeys
lrwxrwxrwx 1 yaniv yaniv 57 Oct 29 19:30 stopCorsairKeyBoardLogiMouseService.sh -> /home/yaniv/coding/automateLinux/evsieve/services/stop.sh
-rw-rw-r-- 1 yaniv yaniv 66 Oct 27 20:35 symlinks.txt
lrwxrwxrwx 1 yaniv yaniv 66 Oct 30 16:45 theRealPath -> /home/yaniv/coding/automateLinux/utilities/theRealPath/theRealPath
yaniv@ubuntu:~/coding/automateLinux/evsieve/led$ 

/home/yaniv/coding/automateLinux/evsieve/services/stop.sh:
sudo $(theRealPath _sudoStop.sh)

/home/yaniv/coding/automateLinux/utilities/theRealPath/theRealPath.c:
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    char *absolute_path = realpath(argv[1], NULL);
    if (!absolute_path) {
        perror("Error resolving path");
        return 1;
    }

    printf("%s\n", absolute_path);
    free(absolute_path);
    return 0;
}

yaniv@ubuntu:~/coding/automateLinux/evsieve/services$ ls
restart.sh  run.sh  stop.sh  _sudoStop.sh
