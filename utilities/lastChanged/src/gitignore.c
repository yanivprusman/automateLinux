#include <gitignore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fnmatch.h>

FileEntry files[MAX_FILES];
int fileCount = 0;

IgnorePattern ignorePatterns[MAX_IGNORE];
int ignoreCount = 0;

char *findGitRoot(const char *startPath) {
    static char gitRoot[MAX_PATH];
    char currentPath[MAX_PATH];
    strncpy(currentPath, startPath, sizeof(currentPath)-1);
    currentPath[sizeof(currentPath)-1] = '\0';

    while (strlen(currentPath) > 1) {
        size_t pathLen = strlen(currentPath);
        if (pathLen + 6 > MAX_PATH) {
            break;
        }
        
        char gitDir[MAX_PATH];
        size_t bufSize = MAX_PATH - pathLen - 1;
        snprintf(gitDir, bufSize + pathLen + 1, "%s/.git", currentPath);
        struct stat st;
        if (stat(gitDir, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(gitRoot, currentPath, sizeof(gitRoot)-1);
            gitRoot[sizeof(gitRoot)-1] = '\0';
            return gitRoot;
        }
        char *lastSlash = strrchr(currentPath, '/');
        if (lastSlash && lastSlash != currentPath) {
            *lastSlash = '\0';
        } else {
            break;
        }
    }
    return NULL;
}

void addPattern(const char *line, const char *baseDir) {
    if (ignoreCount >= MAX_IGNORE) return;

    const char *pattern = line;
    int isNegation = 0;
    if (*pattern == '!') {
        isNegation = 1;
        pattern++;
    }
    if (*pattern == '\0') return;

    while (*pattern == ' ' || *pattern == '\t') pattern++;
    if (*pattern == '\0') return;

    size_t len = strlen(pattern);
    int isDirectory = (pattern[len-1] == '/');
    char *buf = malloc(len + 10);
    strcpy(buf, pattern);
    if (isDirectory) {
        buf[len-1] = '\0';
    }

    ignorePatterns[ignoreCount].pattern = buf;
    ignorePatterns[ignoreCount].isNegation = isNegation;
    ignorePatterns[ignoreCount].isDirectory = isDirectory;
    strncpy(ignorePatterns[ignoreCount].baseDir, baseDir, MAX_PATH-1);
    ignoreCount++;
}

static int pathMatches(const char *pattern, const char *path, int isDirectory, int checkDir) {
    if (isDirectory && !checkDir) return 0;

    if (strchr(pattern, '/')) {
        const char *cleanPath = path;
        if (path[0] == '.' && path[1] == '/') cleanPath = path + 2;
        return fnmatch(pattern, cleanPath, FNM_PATHNAME) == 0;
    }

    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;
    if (fnmatch(pattern, basename, 0) == 0) return 1;

    const char *p = path;
    if (p[0] == '.' && p[1] == '/') p += 2;
    while (*p) {
        if (fnmatch(pattern, p, FNM_PATHNAME) == 0) return 1;
        p = strchr(p, '/');
        if (!p) break;
        p++;
    }
    return 0;
}

static int pathIsUnderDir(const char *path, const char *dir) {
    if (strcmp(dir, ".") == 0) return 1;

    if (strncmp(path, dir, strlen(dir)) == 0) {
        char next = path[strlen(dir)];
        return next == '/' || next == '\0';
    }
    return 0;
}

int shouldIgnore(const char *path, int isDir) {
    int ignored = 0;

    for (int i = 0; i < ignoreCount; i++) {
        const char *baseDir = ignorePatterns[i].baseDir;
        
        if (!pathIsUnderDir(path, baseDir)) {
            continue;
        }
        
        const char *relPath = path;
        if (strcmp(baseDir, ".") != 0) {
            size_t baseDirLen = strlen(baseDir);
            if (strncmp(path, baseDir, baseDirLen) == 0) {
                if (path[baseDirLen] == '/') {
                    relPath = path + baseDirLen + 1;
                } else if (path[baseDirLen] == '\0') {
                    relPath = ".";
                }
            }
        }

        if (pathMatches(ignorePatterns[i].pattern, relPath,
                       ignorePatterns[i].isDirectory, isDir)) {
            ignored = !ignorePatterns[i].isNegation;
        }
    }
    return ignored;
}

void readGitignore(const char *gitignorePath, const char *baseDir) {
    FILE *f = fopen(gitignorePath, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '\0' || *trimmed == '#') continue;
        addPattern(trimmed, baseDir);
    }
    fclose(f);
}

int cmpFiles(const void *a, const void *b) {
    FileEntry *fa = (FileEntry*)a;
    FileEntry *fb = (FileEntry*)b;
    if (fb->mtime > fa->mtime) return 1;
    if (fb->mtime < fa->mtime) return -1;
    return 0;
}

void lastChanged(int n) {
    qsort(files, fileCount, sizeof(FileEntry), cmpFiles);
    for (int i = 0; i < n && i < fileCount; ++i) {
        printf("%s\n", files[i].path);
    }
}
