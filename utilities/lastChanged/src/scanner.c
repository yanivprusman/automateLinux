#include <gitignore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

char *findGitRoot(const char *startPath);

void scanDir(const char *dir, const char *gitRoot) {
    DIR *d = opendir(dir);
    if (!d) return;

    // Check if this directory is itself a git repository (nested repo)
    char checkGitDir[MAX_PATH];
    snprintf(checkGitDir, sizeof(checkGitDir), "%s/.git", dir);
    struct stat st_git;
    const char *currentGitRoot = gitRoot;
    
    if (stat(checkGitDir, &st_git) == 0 && S_ISDIR(st_git.st_mode)) {
        // This is a nested git repo, use its root instead
        currentGitRoot = dir;
        // Clear patterns and re-read from this repo's gitignore
        ignoreCount = 0;
        char gitignorePath[MAX_PATH];
        snprintf(gitignorePath, sizeof(gitignorePath), "%s/.gitignore", dir);
        readGitignore(gitignorePath, dir);
        addPattern(".git/", dir);
    } else if (currentGitRoot) {
        // Read .gitignore from current directory if we're in a git repo
        char gitignorePath[MAX_PATH];
        snprintf(gitignorePath, sizeof(gitignorePath), "%s/.gitignore", dir);
        readGitignore(gitignorePath, dir);
    }

    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) continue;

        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1) continue;

        int isDir = S_ISDIR(st.st_mode);

        // Only check gitignore rules if we're in a git repo
        if (currentGitRoot && shouldIgnore(path, isDir)) continue;

        if (isDir) {
            scanDir(path, currentGitRoot);
        } else if (S_ISREG(st.st_mode)) {
            if (fileCount < MAX_FILES) {
                strncpy(files[fileCount].path, path, MAX_PATH-1);
                files[fileCount].mtime = st.st_mtime;
                fileCount++;
            }
        }
    }
    closedir(d);
}
