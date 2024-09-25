#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

#define mkdir _mkdir
#define PATH_SEPARATOR_C '\\'
#define PATH_SEPARATOR_S "\\"
#else
#include <sys/stat.h>

#include <unistd.h>  // For POSIX mkdir
#define PATH_SEPARATOR_C '/'
#define PATH_SEPARATOR_S "/"
#endif

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long s64;
typedef int s32;
typedef short s16;
typedef char s8;

#define INDENT_SPACE "    "

#define LOG_OK printf(" OK\n")

void panic(const char* msg) {
    printf("\nPANIC: %s\nExiting ..\n", msg);
    exit(1);
}

#define PANIC_MALLOC(msg) panic("Failed to allocate memory (" msg ")")

char* getFilename(char* path) {
    char* lastSlash = strrchr(path, '/');
    if (!lastSlash)
        lastSlash = strrchr(path, '\\');

    if (!lastSlash)
        return path;

    return (char*)(lastSlash + 1);
}

void createDirectory(const char* path) {
    #ifdef _WIN32
    struct _stat st = { 0 };
    #else
    struct stat st = { 0 };
    #endif

    if (stat(path, &st) == -1) {
        #ifdef _WIN32
        if (mkdir(path) != 0)
        #else
        if (mkdir(path, 0700) != 0)
        #endif
            panic("MKDIR failed");
    }
}

void createDirectoryTree(const char* path) {
    char tempPath[1024];
    char* c = NULL;
    u64 len;

    snprintf(tempPath, sizeof(tempPath), "%s", path);
    len = strlen(tempPath);

    for (c = tempPath + 1; c < tempPath + len; c++) {
        if (*c == PATH_SEPARATOR_C) {
            *c = '\0'; // Temporarily null-terminate string

            createDirectory(tempPath);

            *c = PATH_SEPARATOR_C;
        }
    }

    createDirectory(tempPath);
}

void OSPathToSarcPath(char* input, char* output) {
    char* token;
    char* path = strdup(input);
    char* tokens[100];
    int count = 0;

    token = strtok(path, PATH_SEPARATOR_S);
    while (token != NULL) {
        if (strcmp(token, ".") != 0 && strcmp(token, "..") != 0)
            tokens[count++] = token;
        else if (strcmp(token, "..") == 0 && count > 0)
            count--;

        token = strtok(NULL, PATH_SEPARATOR_S);
    }

    if (count > 1) {
        strcpy(output, tokens[count - 2]);
        strcat(output, PATH_SEPARATOR_S);
        strcat(output, tokens[count - 1]);
    }
    else if (count == 1)
        strcpy(output, tokens[0]);
    else
        strcpy(output, ".");

    free(path);
}

#endif
