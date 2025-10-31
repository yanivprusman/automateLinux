#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

char* get_script_dir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        return NULL;
    }
    path[len] = '\0';
    return strdup(dirname(path));
}

char* resolve_path(const char* input_path) {
    char absolute[PATH_MAX];
    char *cwd = getcwd(NULL, 0);
    char *final_path = NULL;
    char tmp[PATH_MAX];
    
    if (!cwd) {
        return NULL;
    }

    // Make path absolute if it's relative
    if (input_path[0] != '/') {
        snprintf(absolute, sizeof(absolute), "%s/%s", cwd, input_path);
    } else {
        strncpy(absolute, input_path, sizeof(absolute) - 1);
        absolute[sizeof(absolute) - 1] = '\0';
    }
    
    free(cwd);

    // Resolve all symlinks in the path
    int max_links = 40;  // Maximum number of symlinks to follow
    strncpy(tmp, absolute, sizeof(tmp));
    
    while (max_links > 0) {
        char *resolved = realpath(tmp, NULL);
        if (resolved) {
            // Found the final target
            final_path = resolved;
            break;
        }
        
        // If error is not due to missing file, break
        if (errno != ENOENT) {
            break;
        }
        
        // Try to read the symlink
        ssize_t len = readlink(tmp, absolute, sizeof(absolute) - 1);
        if (len == -1) {
            // Not a symlink or doesn't exist
            break;
        }
        
        absolute[len] = '\0';
        strncpy(tmp, absolute, sizeof(tmp));
        max_links--;
    }
    
    return final_path;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }

    char *absolute_path = NULL;
    char tmp_path[PATH_MAX];
    const char *input_path = argv[1];
    
    // Strategy 1: Try direct resolution
    absolute_path = resolve_path(input_path);
    
    // Strategy 2: If path is relative and resolution failed, try with script dir
    if (!absolute_path && input_path[0] != '/') {
        char *script_dir = get_script_dir();
        if (script_dir) {
            snprintf(tmp_path, sizeof(tmp_path), "%s/%s", script_dir, input_path);
            absolute_path = resolve_path(tmp_path);
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