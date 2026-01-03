#include <dirent.h>
#include <gitignore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static long long countLines(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return 0;
  long long lines = 0;
  char buffer[8192];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
    for (size_t i = 0; i < bytes; i++) {
      if (buffer[i] == '\n')
        lines++;
    }
  }
  fclose(f);
  return lines;
}

long long scanDir(const char *dir, const char *gitRoot) {
  DIR *d = opendir(dir);
  if (!d)
    return 0;

  // Check if this directory is itself a git repository (nested repo)
  char checkGitDir[MAX_PATH];
  snprintf(checkGitDir, sizeof(checkGitDir), "%s/.git", dir);
  struct stat st_git;
  const char *currentGitRoot = gitRoot;

  // Save current ignore count to restore it later if we add patterns in this
  // scope int initialIgnoreCount = ignoreCount; // Removed as unused earlier

  if (stat(checkGitDir, &st_git) == 0 && S_ISDIR(st_git.st_mode)) {
    // This is a nested git repo, use its root instead
    currentGitRoot = dir;
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

  long long totalSize = 0;
  struct dirent *entry;
  while ((entry = readdir(d))) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

    struct stat st;
    if (lstat(path, &st) == -1)
      continue;

    int isDir = S_ISDIR(st.st_mode);

    // Only check gitignore rules if we're in a git repo
    if (currentGitRoot && shouldIgnore(path, isDir))
      continue;

    if (isDir) {
      long long dirSize = scanDir(path, currentGitRoot);
      totalSize += dirSize;
      // Removed: Directories are no longer added to the files array for output
    } else if (S_ISREG(st.st_mode)) {
      totalSize += st.st_size;
      if (fileCount < MAX_FILES) {
        strncpy(files[fileCount].path, path, MAX_PATH - 1);
        files[fileCount].size = st.st_size;
        files[fileCount].lines = countLines(path);
        fileCount++;
      }
    }
  }
  closedir(d);

  // In a real git-like ignore system, patterns are scoped.
  // This simple implementation just appends to global list.
  // To be truly "like lastChanged", we might not need to pop,
  // but the original lastChanged didn't seem to pop either.

  return totalSize;
}
