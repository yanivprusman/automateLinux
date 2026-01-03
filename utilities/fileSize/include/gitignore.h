#ifndef GITIGNORE_H
#define GITIGNORE_H

#include <stddef.h>

#define MAX_FILES 10000
#define MAX_IGNORE 5000
#define MAX_PATH 4096

typedef struct {
  char path[MAX_PATH];
  long long size;
  long long lines;
} FileEntry;

typedef struct {
  char *pattern;
  int isNegation;
  int isDirectory;
  char baseDir[MAX_PATH];
} IgnorePattern;

extern FileEntry files[MAX_FILES];
extern int fileCount;
extern IgnorePattern ignorePatterns[MAX_IGNORE];
extern int ignoreCount;

// Git utilities
char *findGitRoot(const char *startPath);

// Pattern management
void addPattern(const char *line, const char *baseDir);
int shouldIgnore(const char *path, int isDir);
void readGitignore(const char *gitignorePath, const char *baseDir);

// Directory scanning
long long scanDir(const char *dir, const char *gitRoot);

// File sorting and display
int cmpFiles(const void *a, const void *b);
void print_size(long long size);
void topSizes(int n);

#endif
