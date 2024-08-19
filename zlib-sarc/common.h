#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed long s64;
typedef signed int s32;
typedef signed short s16;
typedef signed char s8;

#define INDENT_SPACE "    "

#define LOG_OK printf(" OK\n")

void panic(const char* msg) {
    printf("\nPANIC: %s\nExiting ..\n", msg);
    exit(1);
}

char* getFilename(char* path) {
    char* lastSlash = strrchr(path, '/');
    if (!lastSlash)
        lastSlash = strrchr(path, '\\');

    if (!lastSlash)
        return path;

    return (char*)(lastSlash + 1);
}

#endif
