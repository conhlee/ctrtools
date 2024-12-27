#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>

#include <time.h>

#ifdef _WIN32
    #include <sys/utime.h>
    #define utimbuf _utimbuf
    #define utime _utime
#else
    #include <utime.h>
#endif

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed long s64;
typedef signed int s32;
typedef signed short s16;
typedef signed char s8;

#define TRUE 1
#define FALSE 0

#define INDENT_SPACE "    "

#define LOG_OK printf(" OK\n")

void panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    printf("\nPANIC: %s\n\n", buffer);

    va_end(args);

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

void setFileTimestamp(char* path, u32 timestamp) {
    struct utimbuf lTime;

    lTime.actime = timestamp;
    lTime.modtime = timestamp;

    if (utime(path, &lTime) < 0)
        panic("Failed to set file timestamp");
}

#endif
