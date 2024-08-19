#ifndef ZLIBPROCESS_H
#define ZLIBPROCESS_H

#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#include "common.h"

typedef struct {
    u8* ptr;
    u32 size;
} DecompressResult;

DecompressResult decompressZlib(u8* zlibBinary, u32 binSize) {
    DecompressResult result;
    result.size = __builtin_bswap32(*(u32*)zlibBinary);

    printf("Alloc buffer (size : %u) ..", result.size);

    result.ptr = (u8*)malloc(result.size);
    if (result.ptr == NULL)
        panic("Mem alloc fail (decompressed buf)");

    LOG_OK;

    ///////////////////////////////////////

    printf("Init ZLIB ..");

    z_stream sInflate;
    sInflate.zalloc = Z_NULL;
    sInflate.zfree = Z_NULL;
    sInflate.opaque = Z_NULL;

    sInflate.avail_in = binSize - sizeof(u32);
    sInflate.next_in = zlibBinary + sizeof(u32);
    sInflate.avail_out = result.size;
    sInflate.next_out = result.ptr;

    LOG_OK;

    ///////////////////////////////////////

    printf("Decompressing ..");

    inflateInit(&sInflate);
    inflate(&sInflate, Z_NO_FLUSH);
    inflateEnd(&sInflate);

    LOG_OK;

    return result;
}

#endif
