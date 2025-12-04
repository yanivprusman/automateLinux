#include "gitignore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int n = 10;
    if (argc > 1) n = atoi(argv[1]);

    char realPath[MAX_PATH];
    if (realpath(".", realPath) == NULL) {
        fprintf(stderr, "Error resolving current directory\n");
        return 1;
    }

    // Find git root (optional)
    char *gitRoot = findGitRoot(realPath);
    
    // If in a git repo, read root .gitignore and add .git pattern
    if (gitRoot) {
        addPattern(".git/", gitRoot);
        char gitignorePath[MAX_PATH];
        snprintf(gitignorePath, sizeof(gitignorePath), "%s/.gitignore", gitRoot);
        readGitignore(gitignorePath, gitRoot);
    }
    
    // Scan from current directory
    scanDir(realPath, gitRoot);
    lastChanged(n);

    // Cleanup
    for (int i = 0; i < ignoreCount; i++) {
        free(ignorePatterns[i].pattern);
    }
    return 0;
}