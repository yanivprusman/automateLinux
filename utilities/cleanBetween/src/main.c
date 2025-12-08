#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: cleanBetween <filename>\n");
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("File open error");
        return 1;
    }
    char tempFile[] = "tempfile.txt";
    FILE *out = fopen(tempFile, "w");
    if (!out) {
        perror("Temporary file creation error");
        fclose(fp);
        return 1;
    }
    char line[MAX_LINE];
    int braceLevel = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Count opening and closing braces in the line
        for (char *p = line; *p; p++) {
            if (*p == '{') braceLevel++;
            else if (*p == '}') braceLevel--;
        }
        // Check if line is empty (only whitespace)
        int isEmpty = 1;
        for (char *p = line; *p; p++) {
            if (!isspace(*p)) {
                isEmpty = 0;
                break;
            }
        }
        // Write line if not empty or outside braces
        if (!(braceLevel > 0 && isEmpty)) {
            fputs(line, out);
        }
    }
    fclose(fp);
    fclose(out);
    // Replace original file with temp file
    remove(argv[1]);
    rename(tempFile, argv[1]);
    printf("Empty lines inside curly braces removed successfully.\n");
    return 0;
}
