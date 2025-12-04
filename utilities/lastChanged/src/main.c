#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <unistd.h>

#define MAX_FILES 10000
#define MAX_IGNORE 1000
#define MAX_PATH 4096

typedef struct {
    char path[MAX_PATH];
    time_t mtime;
} FileEntry;

FileEntry files[MAX_FILES];
int fileCount = 0;

typedef struct {
    char *pattern;
    int isNegation;
    int isDirectory;
} IgnorePattern;

IgnorePattern ignorePatterns[MAX_IGNORE];
int ignoreCount = 0;

// Normalize and parse .gitignore pattern
void addPattern(const char *line) {
    if (ignoreCount >= MAX_IGNORE) return;
    
    const char *pattern = line;
    int isNegation = 0;
    
    // Handle negation
    if (*pattern == '!') {
        isNegation = 1;
        pattern++;
    }
    
    // Skip if empty after negation
    if (*pattern == '\0') return;
    
    size_t len = strlen(pattern);
    int isDirectory = (pattern[len-1] == '/');
    
    // Allocate and copy pattern
    char *buf = malloc(len + 10); // Extra space for modifications
    strcpy(buf, pattern);
    
    // Remove trailing slash for directory patterns
    if (isDirectory) {
        buf[len-1] = '\0';
    }
    
    ignorePatterns[ignoreCount].pattern = buf;
    ignorePatterns[ignoreCount].isNegation = isNegation;
    ignorePatterns[ignoreCount].isDirectory = isDirectory;
    ignoreCount++;
}

// Check if a path component matches a pattern
int pathMatches(const char *pattern, const char *path, int isDirectory, int checkDir) {
    // For directory patterns, only match if this is a directory
    if (isDirectory && !checkDir) return 0;
    
    // If pattern contains '/', it's anchored - match full path
    if (strchr(pattern, '/')) {
        // Remove leading "./" from path for comparison
        const char *cleanPath = path;
        if (path[0] == '.' && path[1] == '/') cleanPath = path + 2;
        
        return fnmatch(pattern, cleanPath, FNM_PATHNAME) == 0;
    }
    
    // Pattern without '/' matches basename anywhere in tree
    // Try matching against the basename
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;
    
    if (fnmatch(pattern, basename, 0) == 0) return 1;
    
    // Also try matching against each path component
    const char *p = path;
    if (p[0] == '.' && p[1] == '/') p += 2; // skip "./"
    
    while (*p) {
        if (fnmatch(pattern, p, FNM_PATHNAME) == 0) return 1;
        p = strchr(p, '/');
        if (!p) break;
        p++; // skip the '/'
    }
    
    return 0;
}

// Check if path should be ignored
int shouldIgnore(const char *path, int isDir) {
    int ignored = 0;
    
    for (int i = 0; i < ignoreCount; i++) {
        if (pathMatches(ignorePatterns[i].pattern, path, 
                       ignorePatterns[i].isDirectory, isDir)) {
            ignored = !ignorePatterns[i].isNegation;
        }
    }
    
    return ignored;
}

// Read .gitignore
void readGitignore(const char *gitignorePath) {
    FILE *f = fopen(gitignorePath, "r");
    if (!f) return;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip empty lines and comments
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (*trimmed == '\0' || *trimmed == '#') continue;
        
        addPattern(trimmed);
    }
    fclose(f);
}

// Recursively scan directories
void scanDir(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) return;
    
    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) continue;
        
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == -1) continue;
        
        int isDir = S_ISDIR(st.st_mode);
        
        // Check if this path should be ignored
        if (shouldIgnore(path, isDir)) continue;
        
        if (isDir) {
            scanDir(path);
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

// Sort files by modification time descending
int cmpFiles(const void *a, const void *b) {
    FileEntry *fa = (FileEntry*)a;
    FileEntry *fb = (FileEntry*)b;
    if (fb->mtime > fa->mtime) return 1;
    if (fb->mtime < fa->mtime) return -1;
    return 0;
}

// Print last n changed files
void lastChanged(int n) {
    qsort(files, fileCount, sizeof(FileEntry), cmpFiles);
    for (int i = 0; i < n && i < fileCount; ++i) {
        printf("%s\n", files[i].path);
    }
}

int main(int argc, char **argv) {
    int n = 10;
    if (argc > 1) n = atoi(argv[1]);
    
    // Always ignore .git directory
    addPattern(".git/");
    
    readGitignore(".gitignore");
    scanDir(".");
    lastChanged(n);
    
    // Cleanup
    for (int i = 0; i < ignoreCount; i++) {
        free(ignorePatterns[i].pattern);
    }
    
    return 0;
}