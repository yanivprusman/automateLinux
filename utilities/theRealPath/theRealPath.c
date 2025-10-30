#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>

char* get_script_dir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        return NULL;
    }
    path[len] = '\0';
    return strdup(dirname(path));
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    // First try as-is
    char *absolute_path = realpath(argv[1], NULL);
    if (!absolute_path) {
        // If that fails, try relative to script dir
        char *script_dir = get_script_dir();
        if (script_dir) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", script_dir, argv[1]);
            absolute_path = realpath(full_path, NULL);
            free(script_dir);
        }
    }
    
    if (!absolute_path) {
        perror("Error resolving path");
        return 1;
    }

    printf("%s\n", absolute_path);
    free(absolute_path);
    return 0;
}