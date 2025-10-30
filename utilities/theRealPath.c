#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <relative_path>\n", argv[0]);
        return 1;
    }

    char resolved_path[PATH_MAX];
    char temp_path[PATH_MAX];
    char *dir_path;
    
    // Get the absolute path of the current script
    if (realpath(argv[0], temp_path) == NULL) {
        perror("Error resolving current script path");
        return 1;
    }

    // Get the directory of the current script
    dir_path = dirname(temp_path);

    // Create full path by concatenating directory and provided relative path
    snprintf(temp_path, sizeof(temp_path), "%s/%s", dir_path, argv[1]);

    // Resolve the final path
    if (realpath(temp_path, resolved_path) == NULL) {
        perror("Error resolving final path");
        return 1;
    }

    // Output in the format: sudo "$(realpath "path")"
    printf("sudo \"%s\"\n", resolved_path);

    return 0;
}