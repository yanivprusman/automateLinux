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